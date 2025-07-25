#ifndef ISM_DISPATCH_UNIT_H
#define ISM_DISPATCH_UNIT_H

#include "instruction.h"

namespace insomnia {

class DispatchUnit : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};

}

#endif // ISM_DISPATCH_UNIT_H