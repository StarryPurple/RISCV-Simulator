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
    circular_queue<std::pair<raw_instr_t, mem_ptr_t>, BufSize> queue; // raw instructions
    mem_ptr_t pc; // managed here
    bool is_fetching; // waiting for iFetch reply. do not try to send another request when waiting.
    // clock_t clk_delay;
    Registers(mem_ptr_t _pc) : pc(_pc), is_fetching(false) {}
  };

public:
  InstructionFetchUnit(
    mem_ptr_t pc,
    std::shared_ptr<const WH_MIU_IFU> miu_input,
    std::shared_ptr<const WH_PRED_IFU> pred_input,
    std::shared_ptr<const WH_FLUSH_CDB> flush_input,
    std::shared_ptr<const WH_DU_IFU> du_input,
    std::shared_ptr<WH_IFU_MIU> miu_output,
    std::shared_ptr<WH_IFU_PRED> pred_output,
    std::shared_ptr<WH_IFU_DU> du_output
    ) :
  _miu_input(std::move(miu_input)), _pred_input(std::move(pred_input)),
  _flush_input(std::move(flush_input)), _du_input(std::move(du_input)),
  _miu_output(std::move(miu_output)), _pred_output(std::move(pred_output)),
  _du_output(std::move(du_output)),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE),
  _cur_regs(pc), _nxt_regs(pc) {}

  void sync() override {
    _cur_stat = _nxt_stat;
    _cur_regs = _nxt_regs;
  }
  bool update() override {
    debug("IFU");
    _nxt_stat = _cur_stat;
    _nxt_regs = _cur_regs;

    WH_IFU_MIU miu_output{};
    WH_IFU_PRED pred_output{};
    WH_IFU_DU du_output{};

    // always be cautious for prediction failure.
    if(_flush_input->is_flush) {
      _nxt_regs.pc = _flush_input->pc;
      _nxt_regs.queue.clear();
      _nxt_regs.is_fetching = false;
      // Then fetch new instr
      miu_output.is_valid = true;
      miu_output.pc = _nxt_regs.pc;
      _nxt_regs.is_fetching = true;
      _nxt_stat = State::FETCH_INSTR;
    } else {
      if(_miu_input->is_valid && !_nxt_regs.queue.full()) {
        // fetch instr reply has came.
        _nxt_regs.queue.emplace(_miu_input->instr, _miu_input->instr_addr);
        _nxt_regs.is_fetching = false;
      }

      if(_du_input->can_accept_req && !_nxt_regs.queue.empty()) {
        auto [raw_instr, instr_addr] = _nxt_regs.queue.front();
        Instruction instr{raw_instr};
        du_output.is_valid = true;
        du_output.raw_instr = raw_instr;
        du_output.instr_addr = instr_addr;
        _nxt_regs.queue.pop();

        if(instr.is_jal()) {
          _nxt_regs.pc = instr_addr + instr.imm();
          _nxt_regs.is_fetching = false;
          _nxt_stat = State::IDLE;
        } else if(instr.is_jalr() || instr.is_br()) {
          pred_output.is_valid = true;
          pred_output.instr_addr = instr_addr;
          pred_output.is_br = instr.is_br();
          pred_output.is_jalr = instr.is_jalr();
          _nxt_regs.pc = instr_addr + 4;
          _nxt_stat = State::HANDLE_BR_JMP;
        } else {
          _nxt_regs.pc = instr_addr + 4;
          _nxt_regs.is_fetching = false;
          _nxt_stat = State::IDLE;
        }
      }
      if(_nxt_stat == State::HANDLE_BR_JMP && _pred_input->is_valid) {
        _nxt_regs.pc = _pred_input->pred_pc;
        _nxt_regs.is_fetching = false;
        _nxt_stat = State::IDLE;
      }
      if(_nxt_stat == State::IDLE && !_nxt_regs.is_fetching && !_nxt_regs.queue.full() && (
        _nxt_regs.queue.empty() || _nxt_regs.queue.front().second != _nxt_regs.pc)) {
        miu_output.is_valid = true;
        miu_output.pc = _nxt_regs.pc;
        _nxt_regs.is_fetching = true;
        _nxt_stat = State::FETCH_INSTR;
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
  const std::shared_ptr<const WH_FLUSH_CDB> _flush_input;
  const std::shared_ptr<const WH_DU_IFU> _du_input;
  const std::shared_ptr<WH_IFU_MIU> _miu_output;
  const std::shared_ptr<WH_IFU_PRED> _pred_output;
  const std::shared_ptr<WH_IFU_DU> _du_output;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;
};

}

#endif // ISM_INSTRUCTION_FETCH_UNIT_H