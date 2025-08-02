#ifndef ISM_PREDICTOR_H
#define ISM_PREDICTOR_H

#include <unordered_map>

#include "wire_harness.h"
#include "common.h"

namespace insomnia {

class Predictor : public CPUModule {
  enum class State {
    IDLE,
    PREDICTING
  };
  struct Registers {
    // branch history table, for br
    // value is ranged in [0, 3]:
    // 00: strong Not Take, 01: weak Not Take,
    // 10: weak Take, 11: strong Take
    // Ugh maybe I should design a state machine here. Whatever.
    std::unordered_map<mem_ptr_t, uint8_t> bht;
    // return address stack, for br and jmp
    std::unordered_map<mem_ptr_t, mem_ptr_t> ras;

    mem_ptr_t pred_pc;
  };
public:
  Predictor(
    std::shared_ptr<const WH_IFU_PRED> ifu_input,
    std::shared_ptr<const WH_ROB_PRED> rob_input,
    std::shared_ptr<WH_PRED_IFU> ifu_output
  ) :
  _ifu_input(std::move(ifu_input)), _rob_input(std::move(rob_input)),
  _ifu_output(std::move(ifu_output)),
  _cur_regs(), _nxt_regs(),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE) {}
  void sync() override {
    _cur_regs = std::move(_nxt_regs); // to reduce C++ simulation time
    _cur_stat = _nxt_stat;
  }
  bool update() override {
    debug("PRED");
    _nxt_regs = _cur_regs;
    _nxt_stat = _cur_stat;

    WH_PRED_IFU ifu_output{};

    // learn first.
    if(_rob_input->is_valid) {
      mem_ptr_t instr_addr = _rob_input->instr_addr;
      if(_rob_input->is_br) {
        uint8_t &bht_state = _nxt_regs.bht[instr_addr];
        if(_rob_input->is_pred_taken) {
          if(bht_state < 0b11) ++bht_state;
        } else {
          if(bht_state > 0b00) --bht_state;
        }
      }
      _nxt_regs.ras[instr_addr] = _rob_input->real_pc;
    }
    // predict after learning latest result.
    switch(_cur_stat) {
    case State::IDLE: {
      if(_ifu_input->is_valid) {
        mem_ptr_t instr_addr = _ifu_input->instr_addr;
        mem_ptr_t predict_pc = instr_addr + 4;

        if(_ifu_input->is_br) {
          uint8_t bht_state = 0b10;
          if(auto it = _nxt_regs.bht.find(instr_addr); it != _nxt_regs.bht.end()) {
            bht_state = it->second;
          }
          // take predict
          if(bht_state >= 0b10)
            if(auto it = _nxt_regs.ras.find(instr_addr); it != _nxt_regs.ras.end()) {
              predict_pc = it->second;
            }
        } else if(_ifu_input->is_jalr) {
          if(auto it = _nxt_regs.ras.find(instr_addr); it != _nxt_regs.ras.end()) {
            predict_pc = it->second;
          }
        } else {
          throw std::runtime_error("Prediction: invalid instruction type");
        }
        _nxt_regs.pred_pc = predict_pc;
        _nxt_stat = State::PREDICTING;
      }
    } break;
    case State::PREDICTING: {
      ifu_output.is_valid = true;
      ifu_output.pred_pc = _cur_regs.pred_pc;
      _nxt_stat = State::IDLE;
    } break;
    }

    bool update_signal = false;
    if(*_ifu_output != ifu_output) {
      *_ifu_output = ifu_output;
      update_signal = true;
    }
    return update_signal;
  }
private:
  const std::shared_ptr<const WH_IFU_PRED> _ifu_input;
  const std::shared_ptr<const WH_ROB_PRED> _rob_input;
  const std::shared_ptr<WH_PRED_IFU> _ifu_output;
  Registers _cur_regs, _nxt_regs;
  State _cur_stat, _nxt_stat;
};

}

#endif // ISM_PREDICTOR_H