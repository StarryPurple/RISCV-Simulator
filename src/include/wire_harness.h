#ifndef ISM_WIRE_HARNESS_H
#define ISM_WIRE_HARNESS_H

#include "common.h"
#include "instruction.h"

// Module implementation order:
// Memory Interface Unit
// Decoder
// Instruction Fetch Unit
// Reorder Buffer
// Common Data Bus
// Predictor
// Register File
// Load Store Buffer

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

struct WH_IFU_DU {
  bool is_valid = false;
  raw_instr_t raw_instr;
  mem_ptr_t instr_addr;
  auto operator<=>(const WH_IFU_DU &) const = default;
};

struct WH_IFU_PRED {
  bool is_valid = false;
  mem_ptr_t instr_addr; // for predictor to locate the instruction and predict
  bool is_br = false;
  bool is_jalr = false;

  auto operator<=>(const WH_IFU_PRED &) const = default;
};

struct WH_DU_IFU {
  bool can_accept_req = false;

  auto operator<=>(const WH_DU_IFU &) const = default;
};

struct WH_PRED_IFU {
  bool is_valid = false;
  mem_ptr_t pred_pc;

  auto operator<=>(const WH_PRED_IFU &) const = default;
};

struct WH_ROB_PRED {
  bool is_valid = false;
  mem_ptr_t instr_addr;
  bool is_pred_taken;  // false if prediction failed. For predictor learning.
  mem_ptr_t real_pc; // for prediction learning
  bool is_br;

  auto operator<=>(const WH_ROB_PRED &) const = default;
};

struct WH_ROB_DU {
  bool is_alloc_valid = false;
  rob_index_t rob_index;

  auto operator<=>(const WH_ROB_DU &) const = default;
};

struct WH_ROB_RF {
  bool is_valid = false;
  rf_index_t dst_reg;
  mem_val_t value;
  raw_instr_t raw_instr;

  auto operator<=>(const WH_ROB_RF &) const = default;
};

struct WH_ROB_LSB {
  bool is_valid = false;
  rob_index_t rob_index;

  auto operator<=>(const WH_ROB_LSB &) const = default;
};

struct WH_DU_ROB {
  bool is_valid = false;
  raw_instr_t raw_instr;

  bool is_br = false;
  bool is_jalr = false;
  mem_ptr_t instr_addr;
  mem_ptr_t pred_pc;

  bool is_store = false;
  mem_ptr_t store_addr;
  mem_val_t store_value;
  mptr_diff_t data_len;

  bool write_rf;
  uint8_t dst_reg;


  auto operator<=>(const WH_DU_ROB &) const = default;
};


struct CDBEntry {
  bool is_valid = false;
  rob_index_t rob_index;

  // br_jalr pc.
  mem_ptr_t real_pc;

  // bool write_rf;
  // store value/rf value.
  mem_val_t value;

  auto operator<=>(const CDBEntry &) const = default;
};

// broadcaster: ALU, LSB
// listener: ROB, RS, DU
struct WH_CDB_OUT {
  CDBEntry entry;

  auto operator<=>(const WH_CDB_OUT &) const = default;
};

struct WH_LSB_CDB {
  CDBEntry entry;

  auto operator<=>(const WH_LSB_CDB &) const = default;
};

struct WH_ALU_CDB {
  CDBEntry entry;

  auto operator<=>(const WH_ALU_CDB &) const = default;
};

// broadcaster: ROB
// listener: IFU, DU, LSB, RS
struct WH_FLUSH_CDB {
  bool is_flush = false;
  mem_ptr_t pc; // ifu will need it.

  auto operator<=>(const WH_FLUSH_CDB &) const = default;
};

struct WH_RF_DU {
  bool is_valid = false;

  mem_val_t Vi, Vj;

  auto operator<=>(const WH_RF_DU &) const = default;
};

struct WH_DU_RF {
  bool is_valid = false;

  bool reqRi = false, reqRj = false;
  rf_index_t Ri, Rj;

  auto operator<=>(const WH_DU_RF &) const = default;
};

struct WH_DU_LSB {
  bool is_valid = false;

  mptr_diff_t data_len;
  bool is_load = false;
  bool is_store = false;
  bool is_store_data_ready;
  rob_index_t rob_index; // store data no ready
  mem_val_t value; // store data ready

  auto operator<=>(const WH_DU_LSB &) const = default;
};

struct WH_DU_RS {
  bool is_valid = false;
  rob_index_t rob_index = 0;
  InstrType instr_type = InstrType::INVALID;

  bool src1_ready = false;
  mem_val_t src1_value = 0;
  rob_index_t src1_index = 0;

  bool src2_ready = false;
  mem_val_t src2_value = 0;
  rob_index_t src2_index = 0;

  int32_t imm = 0;
  uint8_t dst_reg = 0;

  mem_ptr_t instr_addr = 0;
  bool is_branch = false;
  mem_ptr_t pred_pc = 0;

  auto operator<=>(const WH_DU_RS &) const = default;
};

struct WH_RS_ALU {
  bool is_valid = false;
  rob_index_t rob_index = 0;
  InstrType instr_type = InstrType::INVALID;
  mem_val_t src1_value = 0;
  mem_val_t src2_value = 0;
  int32_t imm = 0;
  uint8_t dst_reg = 0;
  mem_ptr_t instr_addr = 0;

  bool is_branch = false;
  mem_ptr_t pred_pc = 0;

  auto operator<=>(const WH_RS_ALU &) const = default;
};

struct WH_ALU_RS {
  bool can_accept_instr = false;
  auto operator<=>(const WH_ALU_RS &) const = default;
};

struct WH_RS_DU {
  bool can_accept_instr = false;
  auto operator<=>(const WH_RS_DU &) const = default;
};












}

#endif // ISM_WIRE_HARNESS_H