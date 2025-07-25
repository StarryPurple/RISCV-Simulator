#ifndef ISM_ALU_H
#define ISM_ALU_H

#include "common.h"

namespace insomnia {

// Maybe you'll write some more differentiated ALUs?

// (High functional) Common Arithmetic and Logic Unit.
// Can do all kinds of calculation in 1 clock cycle.
class CommonALU : public CPUModule {

public:
  bool update() override { return false; }
  void sync() override {}

};

}

#endif // ISM_ALU_H