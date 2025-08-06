#ifndef ISM_REGISTER_FILE_H
#define ISM_REGISTER_FILE_H

#include "wire_harness.h"
#include "common.h"

namespace insomnia {

class RegisterFile : public CPUModule {
  enum class State {
    IDLE, READING
  };
  struct Registers {
    std::array<mem_val_t, RFSize> arr;
    bool repRi = false, repRj = false;
    mem_val_t Vi, Vj;
  };
public:
  RegisterFile(
    std::shared_ptr<const WH_DU_RF> du_input,
    std::shared_ptr<const WH_ROB_RF> rob_input,
    std::shared_ptr<WH_RF_DU> du_output
    ) :
  _du_input(std::move(du_input)), _rob_input(std::move(rob_input)),
  _du_output(std::move(du_output)),
  _cur_regs(), _nxt_regs(),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE) {}
  void sync() override {
    _cur_regs = _nxt_regs;
    _cur_stat = _nxt_stat;
    _cur_regs.arr[0] = 0; // fix x0 = 0
  }
  bool update() override {
    // debug("RF");
    _nxt_regs = _cur_regs;
    _nxt_stat = _cur_stat;

    WH_RF_DU du_output{};

    if(_rob_input->is_valid) {
      _nxt_regs.arr[_rob_input->dst_reg] = _rob_input->value;
      debug("Reg x" + std::to_string(_rob_input->dst_reg) + " is now " + std::to_string(_rob_input->value));
    }

    switch(_nxt_stat) {
    case State::IDLE: {
      if(_du_input->is_valid) {
        if(_du_input->reqRi) {
          _nxt_regs.repRi = true;
          _nxt_regs.Vi = _nxt_regs.arr[_du_input->Ri];
        } else _nxt_regs.repRi = false;
        if(_du_input->reqRj) {
          _nxt_regs.repRj = true;
          _nxt_regs.Vj = _nxt_regs.arr[_du_input->Rj];
        } else _nxt_regs.repRj = false;
        _nxt_stat = State::READING;
      }
    } break;

    case State::READING: {
      du_output = {
        .is_valid = true,
        .repRi = _cur_regs.repRi,
        .repRj = _cur_regs.repRj,
        .Vi = _nxt_regs.Vi,
        .Vj = _nxt_regs.Vj
      };

      _nxt_stat = State::IDLE; // only broadcast for 1 cycle
    } break;
    }

    bool update_signal = false;
    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }
    return update_signal;
  }
  mem_ptr_t get_reg(int i) const {
    if(i == 0) return 0;
    return _cur_regs.arr[i];
  }
private:
  const std::shared_ptr<const WH_DU_RF> _du_input;
  const std::shared_ptr<const WH_ROB_RF> _rob_input;
  const std::shared_ptr<WH_RF_DU> _du_output;
  Registers _cur_regs, _nxt_regs;
  State _cur_stat, _nxt_stat;
};

}

#endif // ISM_REGISTER_FILE_H