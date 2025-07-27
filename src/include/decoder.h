#ifndef ISM_DECODER_H
#define ISM_DECODER_H

#include "common.h"
#include "wire_harness.h"

namespace insomnia {

class Decoder : public CPUModule {
  enum class State {
    IDLE, DECODE
  };
  struct Registers {
    raw_instr_t raw_instr;
    // Instruction instr;
    clock_t clk_delay;
  };

public:
  Decoder(
    std::shared_ptr<const WH_DU_DEC> du_input,
    std::shared_ptr<WH_DEC_DU> du_output
  ) :
    _du_input(std::move(du_input)),
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

    WH_DEC_DU du_output{};

    switch(_cur_stat) {
    case State::IDLE: {
      try_process();
    } break;
    // since no fallthrough in Verilog...
    case State::DECODE: {
      if(--_nxt_regs.clk_delay == 0) { // better write nxt_clk = cur_clk - 1
        // _nxt_regs.instr = Instruction(_nxt_regs.raw_instr);
        du_output.instr = Instruction(_cur_regs.raw_instr);
        du_output.is_valid = true;
        try_process();
      }
    } break;
    }

    bool update_signal = false;

    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }
    return update_signal;
  }

private:
  const std::shared_ptr<const WH_DU_DEC> _du_input;
  const std::shared_ptr<WH_DEC_DU> _du_output;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;

  void try_process() {
    _nxt_stat = State::IDLE;
    if(_du_input->is_valid) {
      _nxt_regs.raw_instr = _du_input->raw_instr;
      _nxt_regs.clk_delay = 1;
      _nxt_stat = State::DECODE;
    }
  }
};

}

#endif // ISM_DECODER_H