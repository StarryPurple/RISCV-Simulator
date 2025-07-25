#ifndef ISM_RAM_H
#define ISM_RAM_H

#include "wire_harness.h"

namespace insomnia {

// Maybe someone will implement a RAM here.

template <std::size_t RAMCap>
class RAMCache : public CPUModule {
public:
  RAMCache(
    std::shared_ptr<WH_LSB_RAMC> input_lsb,
    std::shared_ptr<WH_RAMC_IFU> output_ifu,
    std::shared_ptr<WH_RAMC_LSB> output_lsb
    ) : _mem(),
  _input_lsb(std::move(input_lsb)),
  _output_ifu(std::move(output_ifu)),
  _output_lsb(std::move(output_lsb)) {}

  bool update() override { return false; }
  void sync() override {}

private:
  std::array<char, RAMCap> _mem;
  const std::shared_ptr<const WH_LSB_RAMC> _input_lsb;
  const std::shared_ptr<WH_RAMC_IFU> _output_ifu;
  const std::shared_ptr<WH_RAMC_LSB> _output_lsb;
};

}


#endif // ISM_RAM_H