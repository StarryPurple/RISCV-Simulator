#ifndef ISM_COMMON_H
#define ISM_COMMON_H

#include <cstdint>
#include <memory> // for std::shared_ptr
#include <array>

namespace insomnia {

/********************* global parameters ************************/

using mem_val_t   = uint32_t; // RV32I 4 bytes
using mem_ptr_t   = uint32_t; // RV32I 4 bytes
using mptr_diff_t = uint8_t;
using clock_t     = uint32_t;
using rob_index_t = uint32_t;
using rf_index_t  = uint8_t;  // 0~31

constexpr std::size_t RAMSize = 1 << 22; // 4KB
constexpr std::size_t IFUSize = 8;
constexpr std::size_t ROBSize = 16;
constexpr std::size_t LSBSize = 16;
constexpr std::size_t RSSize  = 16;
constexpr std::size_t CDBCap  = 16;

constexpr std::size_t RAMInstrOffset = 1 << 21; // 2KB

/********************* module base ********************************/

// CPU circuit module base.
// In verilog, we have input wires and output wires.
// Here we simplify this by combining wires related to one module into one WireHarness object.
// Your class should not include the modules' pointers.
// Instead, your class should include the harness's pointers. For example:
// const std::shared_ptr<WireHarnessAB> outputB; // if you feel to change it, call module_b->update().
// const std::shared_ptr<const WireHarnessCA> inputC;  // if this->update() is called, check it.
// const std::shared_ptr<const WireHarnessDA> inputD;  // if this->update() is called, check it.
// const std::shared_ptr<WireHarnessAD> outputD; // if you feel to change it, call module_d->update().
// And many others.
// The CPU will do something like this every clock cycle:
// while(!stabilized) { stabilized = true; for(module: modules) stabilized &= !module->update(); }
// for(module: modules) module->sync();
class CPUModule {
public:
  CPUModule() = default;
  virtual ~CPUModule() = 0;

  // synchronous sequential logic update. Called once every clock cycle by CPU.
  // You can build a brand-new module object with old fields, and replace the old object with the new one.
  // You may have better implementation than the above one.
  virtual void sync() = 0;

  // combinational logic update. Called once by related module everytime it changed its output wire signals.
  // returns whether this update changes some output WireHarnesses.
  virtual bool update() = 0;
};

}

#endif // ISM_COMMON_H