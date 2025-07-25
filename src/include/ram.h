#ifndef ISM_RAM_H
#define ISM_RAM_H

#include "wire_harness.h"

namespace insomnia {

// Maybe someone will implement a RAM here.

// memory is in small endian.
template <std::size_t RAMCap>
class RAMCache : public CPUModule {
  enum class CacheState {
    IDLE, LSB_LOAD, LSB_STORE, IFU_FETCH
  };
  struct Request {
    mem_ptr_t address;
    mem_val_t value;
    mptr_diff_t data_len;
    int cycle_remain;
  };
public:
  RAMCache(
    std::shared_ptr<WH_LSB_RAMC> input_lsb,
    std::shared_ptr<WH_IFU_RAMC> input_ifu,
    std::shared_ptr<WH_RAMC_IFU> output_ifu,
    std::shared_ptr<WH_RAMC_LSB> output_lsb
    ) :
  _input_lsb(std::move(input_lsb)), _input_ifu(std::move(input_ifu)),
  _output_ifu(std::move(output_ifu)), _output_lsb(std::move(output_lsb)),
  _mem(),
  _cur_state(CacheState::IDLE), _nxt_state(CacheState::IDLE),
  _cur_req(), _nxt_req() {}

  bool update() override {
    _nxt_state = _cur_state;
    _nxt_req = _cur_req;

    WH_RAMC_IFU nxt_output_ifu = {false, 0};
    WH_RAMC_LSB nxt_output_lsb = {false, 0};

    switch(_cur_state) {
    case CacheState::IDLE: {
      if(_input_lsb->is_load_request && _input_lsb->is_store_request)
        throw std::runtime_error("RAM update: Invalid wire harness");
      if(_input_lsb->is_load_request) {
        _nxt_req.address = _input_lsb->address;
        _nxt_req.data_len = _input_lsb->data_len;
        _nxt_req.cycle_remain = 3;
        _nxt_state = CacheState::LSB_LOAD;
      } else if(_input_lsb->is_store_request) {
        _nxt_req.address = _input_lsb->address;
        _nxt_req.data_len = _input_lsb->data_len;
        _nxt_req.value = _input_lsb->value;
        _nxt_req.cycle_remain = 3;
        _nxt_state = CacheState::LSB_STORE;
      } else if(_input_ifu->is_valid) {
        _nxt_req.address = _input_ifu->pc;
        _nxt_req.data_len = 4; // fixed
        _nxt_req.cycle_remain = 3;
        _nxt_state = CacheState::IFU_FETCH;
      }
    } break;
    case CacheState::LSB_LOAD: {
      if(--_nxt_req.cycle_remain == 0) {
        nxt_output_lsb.is_valid = true;
        nxt_output_lsb.value = read_mem(_cur_req.address, _cur_req.data_len);
        _nxt_state = CacheState::IDLE;
      }
    } break;
    case CacheState::LSB_STORE: {
      if(--_nxt_req.cycle_remain == 0) {
        write_mem(_cur_req.address, _cur_req.data_len, _cur_req.value);
        _nxt_state = CacheState::IDLE;
      }
    } break;
    case CacheState::IFU_FETCH: {
      if(--_nxt_req.cycle_remain == 0) {
        nxt_output_ifu.is_valid = true;
        nxt_output_ifu.instr = static_cast<raw_instr_t>(read_mem(_cur_req.address, _cur_req.data_len));
        _nxt_state = CacheState::IDLE;
      }
    } break;
    }

    bool result_signal = false;

    if(*_output_ifu != nxt_output_ifu) {
      *_output_ifu = nxt_output_ifu;
      result_signal = true;
    }
    if(*_output_lsb != nxt_output_lsb) {
      *_output_lsb = nxt_output_lsb;
      result_signal = true;
    }
    return result_signal;
  }
  void sync() override {
    _cur_state = _nxt_state;
    _cur_req = _nxt_req;
  }

private:
  const std::shared_ptr<const WH_LSB_RAMC> _input_lsb;
  const std::shared_ptr<const WH_IFU_RAMC> _input_ifu;
  const std::shared_ptr<WH_RAMC_IFU> _output_ifu;
  const std::shared_ptr<WH_RAMC_LSB> _output_lsb;
  std::array<uint8_t, RAMCap> _mem;
  CacheState _cur_state, _nxt_state;
  Request _cur_req, _nxt_req;

  mem_val_t read_mem(mem_ptr_t addr, mptr_diff_t data_len) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("RAMCache: Read memory access out of RAM bound");
    mem_val_t val = 0;
    for(size_t i = 0; i < data_len; ++i)
      val |= static_cast<mem_val_t>(_mem[addr + i]) << (i * 8);
    return val;
  }
  void write_mem(mem_ptr_t addr, mptr_diff_t data_len, mem_val_t val) {
    if(addr + data_len > RAMCap)
      throw std::runtime_error("RAMCache: Write memory access out of RAM bound");
    for(size_t i = 0; i < data_len; ++i)
      _mem[addr + i] = static_cast<uint8_t>((val >> (i * 8)) & 0xff);
  }
};

}


#endif // ISM_RAM_H