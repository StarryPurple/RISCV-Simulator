#ifndef ISM_WIRE_HARNESS_H
#define ISM_WIRE_HARNESS_H

#include "common.h"
#include "instruction.h"

namespace insomnia {

// load raw instruction
struct WH_RAMC_IFU {
  bool is_valid;
  raw_instr_t instr;
};

// load data
struct WH_RAMC_LSB {
  bool is_valid;
  mem_val_t value;
};

// data load request / store data
struct WH_LSB_RAMC {
  bool is_load_request;
  bool is_store_request;
  mem_ptr_t address;
  mem_val_t value;
  data_len_t data_len;
};

}

#endif // ISM_WIRE_HARNESS_H