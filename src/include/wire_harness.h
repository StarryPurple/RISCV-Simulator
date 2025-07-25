#ifndef ISM_WIRE_HARNESS_H
#define ISM_WIRE_HARNESS_H

#include "common.h"
#include "instruction.h"

namespace insomnia {

// load raw instruction
struct WH_RAMC_IFU {
  bool is_valid;
  raw_instr_t instr;
  auto operator<=>(const WH_RAMC_IFU &) const = default;
};

// instruction load request
struct WH_IFU_RAMC {
  bool is_valid;
  mem_ptr_t pc;
  auto operator<=>(const WH_IFU_RAMC &) const = default;
};

// load data
struct WH_RAMC_LSB {
  bool is_valid;
  mem_val_t value;
  auto operator<=>(const WH_RAMC_LSB &) const = default;
};

// data load request / store data
struct WH_LSB_RAMC {
  bool is_load_request;
  bool is_store_request;
  mem_ptr_t address;
  mem_val_t value;
  mptr_diff_t data_len;
  auto operator<=>(const WH_LSB_RAMC &) const = default;
};

}

#endif // ISM_WIRE_HARNESS_H