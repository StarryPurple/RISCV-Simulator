#ifndef ISM_INSTRUCTION_FETCH_UNIT_H
#define ISM_INSTRUCTION_FETCH_UNIT_H

#include "common.h"
#include "instruction.h" // pre-decoding
#include "wire_harness.h"
#include "circular_queue.h"

namespace insomnia {

template <std::size_t BufSize>
class InstructionFetchUnit : public CPUModule {
  enum class State {
    IDLE,
    FETCH_INSTR,
    HANDLE_BR_JMP,
  };
  struct Registers {
    circular_queue<std::pair<raw_instr_t, mem_ptr_t>, IFUSize> instr_queue;
    mem_ptr_t pc; // managed here
    bool is_fetching; // waiting for iFetch reply. do not try to send another request when waiting.
    // clock_t clk_delay;
  };

public:
  InstructionFetchUnit(
    std::shared_ptr<const WH_MIU_IFU> miu_input,
    std::shared_ptr<const WH_PRED_IFU> pred_input,
    std::shared_ptr<WH_IFU_MIU> miu_output,
    std::shared_ptr<WH_IFU_PRED> pred_output,
    std::shared_ptr<WH_IFU_DU> du_output
    ) :
  _miu_input(std::move(miu_input)), _pred_input(std::move(pred_input)),
  _miu_output(std::move(miu_output)), _pred_output(std::move(pred_output)),
  _du_output(std::move(du_output)),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE),
  _cur_regs(), _nxt_regs() {}

  void sync() override {
    _cur_stat = _nxt_stat;
    _cur_regs = _nxt_regs;
  }
  bool update() override {
    _nxt_stat = _cur_stat;
    _nxt_regs = _cur_regs;

    WH_IFU_MIU miu_output{};
    WH_IFU_PRED pred_output{};
    WH_IFU_DU du_output{};

    // always be cautious for prediction failure.
    if(_pred_input->sig_pred_fail) {
      _nxt_regs.pc = _pred_input->pred_fail_pc;
      _nxt_regs.instr_queue.clear();
      _nxt_regs.is_fetching = false;
      // Then fetch new instr
      miu_output.is_valid = true;
      miu_output.pc = _nxt_regs.pc;
      _nxt_stat = State::FETCH_INSTR;
    } else {
      if(_miu_input->is_valid && !_nxt_regs.instr_queue.full()) {
        // fetch instr reply hes came.
        _nxt_regs.instr_queue.emplace(_miu_input->instr, _miu_input->instr_addr);
        _nxt_regs.is_fetching = false;
      }
      switch(_cur_stat) {
      case State::IDLE: {
        try_process(miu_output, pred_output, du_output);
      } break;
      case State::FETCH_INSTR: {
        if(!_nxt_regs.is_fetching) {
          // reply precessed. Do a new fetch.
          _nxt_regs.pc += 4;
          try_process(miu_output, pred_output, du_output);
        }
      } break;
      case State::HANDLE_BR_JMP: {
          if(_pred_input->sig_send_pred) {
            _nxt_regs.pc = _pred_input->pred_pc;
            if(!_nxt_regs.instr_queue.empty()) {
              // not flushed
              du_output.is_valid = true;
              auto [raw_instr, instr_addr] = _nxt_regs.instr_queue.front();
              du_output.raw_instr = raw_instr;
              du_output.instr_addr = instr_addr;
              _nxt_regs.instr_queue.pop();
            }
            try_process(miu_output, pred_output, du_output);
          }
        } break;
      }
    }

    bool update_signal = false;
    if(*_miu_output != miu_output) {
      *_miu_output = miu_output;
      update_signal = true;
    }
    if(*_pred_output != pred_output) {
      *_pred_output = pred_output;
      update_signal = true;
    }
    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }
    return update_signal;
  }

private:
  const std::shared_ptr<const WH_MIU_IFU> _miu_input;
  const std::shared_ptr<const WH_PRED_IFU> _pred_input;
  const std::shared_ptr<WH_IFU_MIU> _miu_output;
  const std::shared_ptr<WH_IFU_PRED> _pred_output;
  const std::shared_ptr<WH_IFU_DU> _du_output;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;

  // what to do when being idle?
  void try_process(WH_IFU_MIU &miu_output, WH_IFU_PRED &pred_output, WH_IFU_DU &du_output) {
    _nxt_stat = State::IDLE;
    if(!_nxt_regs.instr_queue.empty()) {
      auto [raw_instr, instr_addr] = _nxt_regs.instr_queue.front();
      auto instr = Instruction{raw_instr}; // stimulation of pre decoding
      if(instr.is_branch() || instr.is_jump()) {
        pred_output.is_valid = true;
        pred_output.instr_addr = instr_addr;
        _nxt_stat = State::HANDLE_BR_JMP;
      } else {
        du_output.is_valid = true;
        du_output.raw_instr = raw_instr;
        du_output.instr_addr = instr_addr;
        _nxt_regs.instr_queue.pop();
        _nxt_stat = State::IDLE;
      }
    } else if(!_nxt_regs.instr_queue.full() && !_nxt_regs.is_fetching) {
      // fetch one
      miu_output.is_valid = true;
      miu_output.pc = _nxt_regs.pc;
      _nxt_regs.is_fetching = true;
      _nxt_stat = State::FETCH_INSTR;
    }
  }
};

}

#endif // ISM_INSTRUCTION_FETCH_UNIT_H