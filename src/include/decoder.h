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
    std::shared_ptr<const WH_DU_DEC> input_du,
    std::shared_ptr<WH_DEC_DU> output_du
  ) :
    _input_du(std::move(input_du)),
    _output_du(std::move(output_du)),
    _cur_stat(State::IDLE), _nxt_stat(State::IDLE),
    _cur_regs(), _nxt_regs() {}

  bool update() override {
    _nxt_stat = _cur_stat;
    _nxt_regs = _cur_regs;

    WH_DEC_DU nxt_output_du{};

    switch(_cur_stat) {
    case State::IDLE: {
      handle_request();
    } break;
    // since no fallthrough in Verilog...
    case State::DECODE: {
      if(--_nxt_regs.clk_delay == 0) { // better write nxt_clk = cur_clk - 1
        // _nxt_regs.instr = Instruction(_nxt_regs.raw_instr);
        nxt_output_du.instr = Instruction(_cur_regs.raw_instr);
        nxt_output_du.is_valid = true;
        handle_request();
      }
    } break;
    }

    bool update_signal = false;

    if(*_output_du != nxt_output_du) {
      *_output_du = nxt_output_du;
      update_signal = true;
    }
    return update_signal;
  }
  void sync() override {
    _cur_stat = _nxt_stat;
    _cur_regs = _nxt_regs;
  }

private:
  const std::shared_ptr<const WH_DU_DEC> _input_du;
  const std::shared_ptr<WH_DEC_DU> _output_du;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;

  void handle_request() {
    if(_input_du->is_valid) {
      _nxt_regs.raw_instr = _input_du->raw_instr;
      _nxt_regs.clk_delay = 1;
      _nxt_stat = State::DECODE;
    } else {
      _nxt_stat = State::IDLE;
    }
  }
};

}

#endif // ISM_DECODER_H