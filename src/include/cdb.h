#ifndef ISM_CDB_H
#define ISM_CDB_H

#include "wire_harness.h"
#include "common.h"
#include <array>

namespace insomnia {

class CommonDataBus : public CPUModule {
public:
  CommonDataBus(
    std::shared_ptr<const WH_LSB_CDB> lsb_input,
    std::shared_ptr<const WH_ALU_CDB> alu_input,
    std::shared_ptr<WH_CDB_OUT> output
  ) :
  _lsb_input(std::move(lsb_input)), _alu_input(std::move(alu_input)),
  _output(std::move(output)) {}
  void sync() override {}
  bool update() override {
    WH_CDB_OUT output;
    // lsb first. alu second.
    if(_lsb_input->entry.is_valid) {
      _output->entry = _lsb_input->entry;
    } else if(_alu_input->entry.is_valid) {
      _output->entry = _alu_input->entry;
    }

    bool update_signal = false;
    if(*_output != output) {
      *_output = output;
      update_signal = true;
    }
    return update_signal;
  }

private:
  const std::shared_ptr<const WH_LSB_CDB> _lsb_input;
  const std::shared_ptr<const WH_ALU_CDB> _alu_input;
  const std::shared_ptr<WH_CDB_OUT> _output;
};

}

#endif // ISM_CDB_H