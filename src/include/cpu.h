#ifndef ISM_CPU_H
#define ISM_CPU_H

#include <algorithm>
#include <iostream>
#include <random>

#include "alu.h"
#include "ram.h"
#include "decoder.h"
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
  using RAMC = RAMCache<RAMSize>;
  using DEC  = Decoder;
  using IFU  = InstructionFetchUnit<IFUSize>;
  using DU   = DispatchUnit;
  using RoB  = ReorderBuffer<RoBSize>;
  using CALU = CommonALU;
  using LSB  = LoadStoreBuffer<LSBSize>;
  using RS   = ReservationStation<RSSize>;
  using PRED = Predictor;
  using RF   = RegisterFile;

private:
  mem_ptr_t _pc; // program counter register, isolated from RF.
  clock_t _clk;  // state machine update clock
  std::array<std::shared_ptr<CPUModule>, 10> _modules; // CPU modules

public:
  CPU() {
    std::shared_ptr<RAMC> ramc;      // Random Access Memory (or rather, its cache)
    std::shared_ptr<DEC>  dec;       // Instruction Decoder
    std::shared_ptr<IFU>  ifu;       // Instruction Fetch Unit
    std::shared_ptr<DU>   du;        // Dispatch Unit (with Instruction Fetch Unit and Decoder integrated)
    std::shared_ptr<RoB>  rob;       // Reorder Buffer
    std::shared_ptr<CALU> calu;      // (High functional) Common ALU. Can do all kinds of calculation in 1 clock cycle.
    std::shared_ptr<LSB>  lsb;       // Load Store Buffer
    std::shared_ptr<RS>   rs;        // Reservation Station
    std::shared_ptr<PRED> pred;      // Branch Predictor
    std::shared_ptr<RF>   rf;        // General Register File

    // You can even shuffle the modules here.
    // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device()()));
    _modules = {ramc, dec, du, rob, calu, lsb, rs, pred, rf};
  }

  void tick() {
    ++_clk;
    bool stabilized = false;
    while(!stabilized) {
      stabilized = true;
      // You can even shuffle the modules here.
      // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device()()));
      for(auto &module: _modules)
        stabilized &= !module->update();
    }
    // You can even shuffle the modules here.
    // std::shuffle(_modules.begin(), _modules.end(), std::mt19937_64(std::random_device()()));
    for(auto &module: _modules)
      module->sync();
  }

private:
  // via std::cin. Pre-assumed the input style.
  void read_program() {
    /*
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
        _program.emplace(cur_ptr + diff_ptr, raw_instr);
        hex_cnt = 0;
        raw_instr = 0;
        diff_ptr += 4;
      }
    }
    */
  }

};

}

#endif // ISM_CPU_H