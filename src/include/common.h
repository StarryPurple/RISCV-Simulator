#ifndef ISM_COMMON_H
#define ISM_COMMON_H

#include <cstdint>

namespace insomnia {

/********************* global parameters ************************/

using sto_val_t   = uint32_t; // RV32I 4 bytes
using raw_instr_t = uint32_t; // RV32I 4 bytes
using pc_ptr_t    = uint32_t; // RV32I 4 bytes

constexpr int kRoBSize = 64;

}

#endif // ISM_COMMON_H