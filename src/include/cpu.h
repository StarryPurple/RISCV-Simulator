#ifndef ISM_CPU_H
#define ISM_CPU_H

#include <algorithm>
#include <iostream>
#include <random>

#include "alu.h"
#include "cdb.h"
#include "miu.h"
// #include "decoder.h"
#include "predictor.h"
#include "wire_harness.h"
#include "dispatch_unit.h"
#include "register_file.h"
#include "reorder_buffer.h"
#include "load_store_buffer.h"
#include "reservation_station.h"
#include "instruction_fetch_unit.h"

namespace insomnia {

class CPU {
  // module type alias
  using MIU  = MemoryInterfaceUnit<RAMSize>;
  // using DEC  = Decoder;
  using IFU  = InstructionFetchUnit<IFUSize>;
  using DU   = DispatchUnit;
  using ROB  = ReorderBuffer<ROBSize>;
  using ALU = CommonALU;
  using LSB  = LoadStoreBuffer<LSBSize>;
  using RS   = ReservationStation<RSSize>;
  using PRED = Predictor;
  using RF   = RegisterFile;
  using CDB  = CommonDataBus;
private:
  mem_ptr_t _pc; // program counter register, isolated from RF.
  clock_t _clk;  // state machine update clock
  std::array<std::shared_ptr<CPUModule>, 10> _modules; // CPU modules array, for traversion

  std::shared_ptr<MIU>  _miu;       // Memory Interface Unit (in contact with RAM)
  // std::shared_ptr<DEC>  _dec;       // Instruction Decoder
  std::shared_ptr<IFU>  _ifu;       // Instruction Fetch Unit
  std::shared_ptr<DU>   _du;        // Dispatch Unit (with Instruction Fetch Unit and Decoder integrated)
  std::shared_ptr<ROB>  _rob;       // ReOrder Buffer
  std::shared_ptr<ALU>  _alu;       // (High functional) Common ALU. Can do all kinds of calculation in 1 clock cycle.
  std::shared_ptr<LSB>  _lsb;       // Load Store Buffer
  std::shared_ptr<RS>   _rs;        // Reservation Station
  std::shared_ptr<PRED> _pred;      // Branch Predictor
  std::shared_ptr<RF>   _rf;        // General Register File
  std::shared_ptr<CDB>  _cdb;       // Common Data Bus

public:
  CPU() : _pc(0), _clk(0) {

    auto wh_miu_ifu   = std::make_shared<WH_MIU_IFU>();
    auto wh_ifu_miu   = std::make_shared<WH_IFU_MIU>();
    auto wh_miu_lsb   = std::make_shared<WH_MIU_LSB>();
    auto wh_lsb_miu   = std::make_shared<WH_LSB_MIU>();
    auto wh_ifu_du    = std::make_shared<WH_IFU_DU>();
    auto wh_ifu_pred  = std::make_shared<WH_IFU_PRED>();
    auto wh_du_ifu    = std::make_shared<WH_DU_IFU>();
    auto wh_pred_ifu  = std::make_shared<WH_PRED_IFU>();
    auto wh_rob_pred  = std::make_shared<WH_ROB_PRED>();
    auto wh_rob_du    = std::make_shared<WH_ROB_DU>();
    auto wh_rob_rf    = std::make_shared<WH_ROB_RF>();
    auto wh_rob_lsb   = std::make_shared<WH_ROB_LSB>();
    auto wh_du_rob    = std::make_shared<WH_DU_ROB>();
    auto wh_cdb_out   = std::make_shared<WH_CDB_OUT>();
    auto wh_lsb_cdb   = std::make_shared<WH_LSB_CDB>();
    auto wh_alu_cdb   = std::make_shared<WH_ALU_CDB>();
    auto wh_flush     = std::make_shared<WH_FLUSH_PIPELINE>();
    auto wh_rf_du     = std::make_shared<WH_RF_DU>();
    auto wh_du_rf     = std::make_shared<WH_DU_RF>();
    auto wh_du_lsb    = std::make_shared<WH_DU_LSB>();
    auto wh_du_rs     = std::make_shared<WH_DU_RS>();
    auto wh_rs_alu    = std::make_shared<WH_RS_ALU>();
    auto wh_alu_rs    = std::make_shared<WH_ALU_RS>();
    auto wh_rs_du     = std::make_shared<WH_RS_DU>();


    _miu = std::make_shared<MIU>(
      wh_lsb_miu,
      wh_ifu_miu,
      wh_flush,
      wh_miu_ifu,
      wh_miu_lsb
    );

    _cdb = std::make_shared<CDB>(
      wh_lsb_cdb,
      wh_alu_cdb,
      wh_cdb_out
    );

    _pred = std::make_shared<PRED>(
      wh_ifu_pred,
      wh_rob_pred,
      wh_pred_ifu
    );

    _rf = std::make_shared<RF>(
      wh_du_rf,
      wh_rob_rf,
      wh_rf_du
    );

    _rob = std::make_shared<ROB>(
      wh_du_rob,
      wh_cdb_out,
      wh_rob_lsb,
      wh_rob_du,
      wh_rob_pred,
      wh_rob_rf,
      wh_flush
    );

    _ifu = std::make_shared<IFU>(
      _pc,
      wh_miu_ifu,
      wh_pred_ifu,
      wh_flush,
      wh_du_ifu,
      wh_ifu_miu,
      wh_ifu_pred,
      wh_ifu_du
    );

    _du = std::make_shared<DU>(
      wh_ifu_du,
      wh_rf_du,
      wh_rob_du,
      wh_cdb_out,
      wh_flush,
      wh_rs_du,
      wh_du_ifu,
      wh_du_rs,
      wh_du_lsb,
      wh_du_rf,
      wh_du_rob
    );

    _alu = std::make_shared<ALU>(
      wh_rs_alu,
      wh_flush,
      wh_alu_cdb,
      wh_alu_rs
    );

    _lsb = std::make_shared<LSB>(
      wh_miu_lsb,
      wh_du_lsb,
      wh_rob_lsb,
      wh_flush,
      wh_cdb_out,
      wh_lsb_miu,
      wh_lsb_cdb
    );

    _rs = std::make_shared<RS>(
      wh_du_rs,
      wh_cdb_out,
      wh_flush,
      wh_alu_rs,
      wh_rs_alu,
      wh_rs_du
    );

    // 3. Populate _modules array in logical execution order
    _modules = {
      _miu,
      _cdb,
      _pred,
      _rf,
      _rob,
      _ifu,
      _du,
      _alu,
      _lsb,
      _rs
    };
    // You can even shuffle the modules here.
    // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device{}()));
  }

  // via std::cin. Pre-assumed the input style.
  void preload_program() {
    mem_ptr_t cur_ptr = 0, diff_ptr = 0;
    raw_instr_t raw_instr = 0;
    int hex_cnt = 0;
    for(char ch = std::cin.get(); ch != EOF; ch = std::cin.get()) {
      if(is_delim(ch)) continue;
      if(ch == '@') {
        cur_ptr = 0;
        for(int i = 0; i < 8; ++i) {
          ch = std::cin.get();
          cur_ptr = (cur_ptr << 4) | hex2dec(ch);
        }
        diff_ptr = 0;
        continue;
      }
      raw_instr = (raw_instr << 4) | hex2dec(ch);
      if(++hex_cnt == 8) {
        raw_instr = ToSmallEndian32_8(raw_instr);
        _miu->preload_program(raw_instr, cur_ptr + diff_ptr);
        hex_cnt = 0;
        raw_instr = 0;
        diff_ptr += 4;
      }
    }
  }
  bool tick() {
    ++_clk;

    debug("Clock cycle " + std::to_string(_clk) + ":++++++++++++++++++");
    for(bool stabilized = false; !stabilized; ) {
      debug("Try update-----------");
      stabilized = true;
      // You can even shuffle the modules here.
      // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device{}()));
      for(auto &module: _modules) {
        bool res = module->update();
        // debug(res ? "Updated" : "Not updated");
        if(res) stabilized = false;
      }
    }
    // You can even shuffle the modules here.
    // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device{}()));
    for(auto &module: _modules)
      module->sync();
    return !_rob->to_terminate();
    debug("Clock cycle " + std::to_string(_clk) + "ends.");
  }

  mem_val_t get_reg(int i) const {
    return _rf->get_reg(i);
  }
};

}

#endif // ISM_CPU_H