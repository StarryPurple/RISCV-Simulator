#ifndef ISM_REGISTER_FILE_H
#define ISM_REGISTER_FILE_H

#include "wire_harness.h"
#include "common.h"

namespace insomnia {

class RegisterFile : public CPUModule {
  using Registers = std::array<mem_val_t, RFSize>;
public:
  RegisterFile(
    std::shared_ptr<const WH_DU_RF> du_input,
    std::shared_ptr<const WH_ROB_RF> rob_input,
    std::shared_ptr<WH_RF_DU> du_output
    ) :
  _du_input(std::move(du_input)), _rob_input(std::move(rob_input)),
  _du_output(std::move(du_output)),
  _cur_regs(), _nxt_regs() {}
  void sync() override {
    _cur_regs = _nxt_regs;
    _cur_regs[0] = 0; // fix x0 = 0
  }
  bool update() override {
    debug("RF update start");
    _nxt_regs = _cur_regs;

    WH_RF_DU du_output{};

    if(_rob_input->is_valid) {
      _nxt_regs[_rob_input->dst_reg] = _rob_input->value;
    }
    if(_du_input->is_valid) {
      du_output.is_valid = true;
      if(_du_input->reqRi) {
        du_output.Vi = _nxt_regs[_du_input->Ri];
      }
      if(_du_input->reqRj) {
        du_output.Vj = _nxt_regs[_du_input->Rj];
      }
    }

    bool update_signal = false;
    if(*_du_output != du_output) {
      *_du_output = du_output;
      update_signal = true;
    }
    debug("RF update end");
    return update_signal;
  }
  mem_ptr_t get_reg(int i) const {
    return _cur_regs[i];
  }
private:
  const std::shared_ptr<const WH_DU_RF> _du_input;
  const std::shared_ptr<const WH_ROB_RF> _rob_input;
  const std::shared_ptr<WH_RF_DU> _du_output;
  Registers _cur_regs, _nxt_regs;
};

}

#endif // ISM_REGISTER_FILE_H