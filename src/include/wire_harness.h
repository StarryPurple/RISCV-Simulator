#ifndef ISM_WIRE_HARNESS_H
#define ISM_WIRE_HARNESS_H

#include "common.h"
#include "instruction.h"

// Module implementation order:
// Memory Interface Unit
// Decoder
// Instruction Fetch Unit

// output structs be in front of input structs

namespace insomnia {

// load raw instruction
struct WH_MIU_IFU {
  bool is_valid = false;
  raw_instr_t instr{};
  mem_ptr_t instr_addr;
  auto operator<=>(const WH_MIU_IFU &) const = default;
};

// instruction load request
struct WH_IFU_MIU {
  bool is_valid = false;
  mem_ptr_t pc{};
  auto operator<=>(const WH_IFU_MIU &) const = default;
};

// load data
struct WH_MIU_LSB {
  bool is_valid = false;
  mem_val_t value{};
  auto operator<=>(const WH_MIU_LSB &) const = default;
};

// data load request / store data
struct WH_LSB_MIU {
  bool is_load_request = false;
  bool is_store_request = false;
  mem_ptr_t addr{};
  mem_val_t value{};
  mptr_diff_t data_len{};
  auto operator<=>(const WH_LSB_MIU &) const = default;
};

// decoded instruction
struct WH_DEC_DU {
  bool is_valid = false;
  Instruction instr{};
  auto operator<=>(const WH_DEC_DU &) const = default;
};

// raw instruction
struct WH_DU_DEC {
  bool is_valid = false;
  raw_instr_t raw_instr{};
  auto operator<=>(const WH_DU_DEC &) const = default;
};

struct WH_IFU_DU {
  bool is_valid = false;
  raw_instr_t raw_instr;
  mem_ptr_t instr_addr;
  auto operator<=>(const WH_IFU_DU &) const = default;
};

struct WH_IFU_PRED {
  bool is_valid = false;
  mem_ptr_t instr_addr; // for predictor to locate the instruction and predict

  auto operator<=>(const WH_IFU_PRED &) const = default;
};

struct WH_DU_IFU {
  bool is_valid = false;

  auto operator<=>(const WH_DU_IFU &) const = default;
};

struct WH_PRED_IFU {
  bool sig_send_pred = false;
  mem_ptr_t pred_pc;

  bool sig_pred_fail = false; // flush
  mem_ptr_t pred_fail_pc; // the true pc pointer transferred at flush

  auto operator<=>(const WH_PRED_IFU &) const = default;
};

}

#endif // ISM_WIRE_HARNESS_H