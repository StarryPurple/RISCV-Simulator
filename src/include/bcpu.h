#pragma once

#include <iostream>

#include "common.h"
#include "utility.h"
#include "instruction.h"

namespace insomnia {

class BCPU {

  std::array<uint8_t, RAMSize> _mem{};
  std::array<mem_val_t, RFSize> _regs{};
  uint64_t _clk = 0;
  mem_ptr_t _pc = 0;

  mem_val_t read_mem(mem_ptr_t addr, mptr_diff_t data_len) {
    mem_val_t val = 0;
    for(size_t i = 0; i < data_len; ++i)
      val |= static_cast<mem_val_t>(_mem[addr + i]) << (i * 8);
    // debug("MIU: Read " + std::to_string(val) + " with data len " + std::to_string(data_len) +
    //   " from addr " + std::to_string(addr));
    return val;
  }
  void write_mem(mem_ptr_t addr, mptr_diff_t data_len, mem_val_t val) {
    // debug("MIU: Write " + std::to_string(val) + " with data len " + std::to_string(data_len) +
    //   " to addr " + std::to_string(addr));
    for(size_t i = 0; i < data_len; ++i)
      _mem[addr + i] = static_cast<uint8_t>((val >> (i * 8)) & 0xff);
  }

public:
  void preload_program() {
    mem_ptr_t cur_ptr = 0, diff_ptr = 0;
    raw_instr_t raw_instr = 0;
    int hex_cnt = 0;
    for(char ch = std::cin.get(); ch != EOF && ch != '&'; ch = std::cin.get()) {
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
        write_mem(cur_ptr + diff_ptr, 4, raw_instr);
        hex_cnt = 0;
        raw_instr = 0;
        diff_ptr += 4;
      }
    }
  }

  mem_val_t get_ret() const {
    return _regs[10] & 0xff;
  }

  bool tick() {
    raw_instr_t raw_instr = read_mem(_pc, 4);
    if(raw_instr == 0x0ff00513) return false;
    Instruction instr{raw_instr};
    auto rd = instr.rd();
    auto rs1 = instr.rs1();
    auto rs2 = instr.rs2();
    auto imm = instr.imm();
    switch(instr.type()) {
    case InstrType::LUI:
      _regs[rd] = imm << 12;
      break;
    case InstrType::AUIPC:
      _regs[rd] = _pc + (imm << 12);
      break;
    case InstrType::JAL:
      _regs[rd] = _pc + 4;
      _pc = _pc + imm;
      break;
    case InstrType::JALR:
      _regs[rd] = _pc + 4;
      _pc = (_regs[rs1] + imm) & (~0x1);
      break;
    case InstrType::BEQ:
      if(_regs[rs1] == _regs[rs2]) _pc += imm; else _pc += 4;
      break;
    case InstrType::BNE:
      if(_regs[rs1] != _regs[rs2]) _pc += imm; else _pc += 4;
      break;
    case InstrType::BLT:
      if(static_cast<int32_t>(_regs[rs1]) < static_cast<int32_t>(_regs[rs2])) _pc += imm; else _pc += 4;
      break;
    case InstrType::BGE:
      if(static_cast<int32_t>(_regs[rs1]) >= static_cast<int32_t>(_regs[rs2])) _pc += imm; else _pc += 4;
      break;
    case InstrType::BLTU:
      if(_regs[rs1] < _regs[rs2]) _pc += imm; else _pc += 4;
      break;
    case InstrType::BGEU:
      if(_regs[rs1] >= _regs[rs2]) _pc += imm; else _pc += 4;
      break;
    case InstrType::LB:
      _regs[rd] = sign_extend<mem_val_t, 8>(read_mem(_regs[rs1] + imm, 1));
      break;
    case InstrType::LH:
      _regs[rd] = sign_extend<mem_val_t, 16>(read_mem(_regs[rs1] + imm, 2));
      break;
    case InstrType::LW:
      _regs[rd] = read_mem(_regs[rs1] + imm, 4);
      break;
    case InstrType::LBU:
      _regs[rd] = read_mem(_regs[rs1] + imm, 1);
      break;
    case InstrType::LHU:
      _regs[rd] = read_mem(_regs[rs1] + imm, 2);
      break;
    case InstrType::SB:
      write_mem(_regs[rs1] + imm, 1, _regs[rs2]);
      break;
    case InstrType::SH:
      write_mem(_regs[rs1] + imm, 2, _regs[rs2]);
      break;
    case InstrType::SW:
      write_mem(_regs[rs1] + imm, 4, _regs[rs2]);
      break;
    case InstrType::ADDI:
      _regs[rd] = _regs[rs1] + imm;
      break;
    case InstrType::SLTI:
      _regs[rd] = (static_cast<int32_t>(_regs[rs1]) < imm) ? 1 : 0;
      break;
    case InstrType::SLTIU:
      _regs[rd] = (_regs[rs1] < static_cast<mem_val_t>(imm)) ? 1 : 0;
      break;
    case InstrType::XORI:
      _regs[rd] = _regs[rs1] ^ imm;
      break;
    case InstrType::ORI:
      _regs[rd] = _regs[rs1] | imm;
      break;
    case InstrType::ANDI:
      _regs[rd] = _regs[rs1] & imm;
      break;
    case InstrType::SLLI:
      _regs[rd] = _regs[rs1] << imm;
      break;
    case InstrType::SRLI:
      _regs[rd] = _regs[rs1] >> imm;
      break;
    case InstrType::SRAI:
      _regs[rd] = static_cast<mem_val_t>(static_cast<int32_t>(_regs[rs1]) >> imm);
      break;
    case InstrType::ADD:
      _regs[rd] = _regs[rs1] + _regs[rs2];
      break;
    case InstrType::SUB:
      _regs[rd] = _regs[rs1] - _regs[rs2];
      break;
    case InstrType::SLL:
      _regs[rd] = _regs[rs1] << _regs[rs2];
      break;
    case InstrType::SLT:
      _regs[rd] = (_regs[rs1] < _regs[rs2]) ? 1 : 0;
      break;
    case InstrType::SLTU:
      _regs[rd] = (_regs[rs1] < _regs[rs2]) ? 1 : 0;
      break;
    case InstrType::XOR:
      _regs[rd] = _regs[rs1] ^ _regs[rs2];
      break;
    case InstrType::SRL:
      _regs[rd] = _regs[rs1] >> _regs[rs2];
      break;
    case InstrType::SRA:
      _regs[rd] = static_cast<mem_val_t>(static_cast<int32_t>(_regs[rs1]) >> _regs[rs2]);
      break;
    case InstrType::OR:
      _regs[rd] = _regs[rs1] | _regs[rs2];
      break;
    case InstrType::AND:
      _regs[rd] = _regs[rs1] & _regs[rs2];
      break;
    case InstrType::INVALID:
      throw std::runtime_error("Invalid operation");
    }
    if(!instr.is_br() && !instr.is_jal() && !instr.is_jalr()) _pc += 4;
    _regs[0] = 0;
    return true;
  }
};

}

























