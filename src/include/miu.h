#ifndef ISM_MIU_H
#define ISM_MIU_H

#include "wire_harness.h"

namespace insomnia {

// Maybe someone will implement a RAM here.

// memory is in small endian.
template <std::size_t RAMCap, std::size_t InstrOffset>
class MIU : public CPUModule {
  enum class State {
    IDLE, LSB_LOAD, LSB_STORE, IFU_FETCH
  };
  struct Registers {
    mem_ptr_t addr;
    mem_val_t value;
    mptr_diff_t data_len;
    clock_t clk_delay;
  };
public:
  MIU(
    std::shared_ptr<const WH_LSB_RAMC> input_lsb,
    std::shared_ptr<const WH_IFU_RAMC> input_ifu,
    std::shared_ptr<WH_RAMC_IFU> output_ifu,
    std::shared_ptr<WH_RAMC_LSB> output_lsb
    ) :
  _input_lsb(std::move(input_lsb)), _input_ifu(std::move(input_ifu)),
  _output_ifu(std::move(output_ifu)), _output_lsb(std::move(output_lsb)),
  _mem(),
  _cur_stat(State::IDLE), _nxt_stat(State::IDLE),
  _cur_regs(), _nxt_regs() {}

  bool update() override {
    _nxt_stat = _cur_stat;
    _nxt_regs = _cur_regs;

    WH_RAMC_IFU nxt_output_ifu{};
    WH_RAMC_LSB nxt_output_lsb{};

    switch(_cur_stat) {
    case State::IDLE: {
      handle_request();
    } break;
    case State::LSB_LOAD: {
      if(--_nxt_regs.cycle_remain == 0) {
        nxt_output_lsb.is_valid = true;
        nxt_output_lsb.value = read_mem(_cur_regs.address, _cur_regs.data_len);
        handle_request();
      }
    } break;
    case State::LSB_STORE: {
      if(--_nxt_regs.cycle_remain == 0) {
        write_mem(_cur_regs.address, _cur_regs.data_len, _cur_regs.value);
        handle_request();
      }
    } break;
    case State::IFU_FETCH: {
      if(--_nxt_regs.cycle_remain == 0) {
        nxt_output_ifu.is_valid = true;
        nxt_output_ifu.instr = static_cast<raw_instr_t>(
          read_mem(_cur_regs.address + InstrOffset, _cur_regs.data_len));
        handle_request();
      }
    } break;
    }

    bool update_signal = false;

    if(*_output_ifu != nxt_output_ifu) {
      *_output_ifu = nxt_output_ifu;
      update_signal = true;
    }
    if(*_output_lsb != nxt_output_lsb) {
      *_output_lsb = nxt_output_lsb;
      update_signal = true;
    }
    return update_signal;
  }
  void sync() override {
    _cur_stat = _nxt_stat;
    _cur_regs = _nxt_regs;
  }

  // the offset here does not include InstrOffset
  void preload_instruction(raw_instr_t raw_instr, std::size_t offset) {
    static_assert(std::is_same_v<raw_instr_t, mem_val_t>);
    write_mem(InstrOffset + offset, 4, raw_instr);
  }

private:
  const std::shared_ptr<const WH_LSB_RAMC> _input_lsb;
  const std::shared_ptr<const WH_IFU_RAMC> _input_ifu;
  const std::shared_ptr<WH_RAMC_IFU> _output_ifu;
  const std::shared_ptr<WH_RAMC_LSB> _output_lsb;
  std::array<uint8_t, RAMCap> _mem;
  State _cur_stat, _nxt_stat;
  Registers _cur_regs, _nxt_regs;

  void handle_request() {
    if(_input_lsb->is_load_request && _input_lsb->is_store_request)
      throw std::runtime_error("RAM update: Invalid wire harness");
    if(_input_lsb->is_load_request) {
      _nxt_regs.addr = _input_lsb->addr;
      _nxt_regs.data_len = _input_lsb->data_len;
      _nxt_regs.cycle_remain = 3;
      _nxt_stat = State::LSB_LOAD;
    } else if(_input_lsb->is_store_request) {
      _nxt_regs.addr = _input_lsb->addr;
      _nxt_regs.data_len = _input_lsb->data_len;
      _nxt_regs.value = _input_lsb->value;
      _nxt_regs.cycle_remain = 3;
      _nxt_stat = State::LSB_STORE;
    } else if(_input_ifu->is_valid) {
      _nxt_regs.addr = _input_ifu->pc;
      _nxt_regs.data_len = 4; // fixed
      _nxt_regs.cycle_remain = 3;
      _nxt_stat = State::IFU_FETCH;
    } else {
      _nxt_stat = State::IDLE;
    }
  }
  mem_val_t read_mem(mem_ptr_t addr, mptr_diff_t data_len) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("MIU: Read memory access out of RAM bound");
    mem_val_t val = 0;
    for(size_t i = 0; i < data_len; ++i)
      val |= static_cast<mem_val_t>(_mem[addr + i]) << (i * 8);
    return val;
  }
  void write_mem(mem_ptr_t addr, mptr_diff_t data_len, mem_val_t val) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("MIU: Write memory access out of RAM bound");
    for(size_t i = 0; i < data_len; ++i)
      _mem[addr + i] = static_cast<uint8_t>((val >> (i * 8)) & 0xff);
  }
};

}


#endif // ISM_MIU_H