#ifndef ISM_REGISTER_H
#define ISM_REGISTER_H

#include "common.h"

namespace insomnia {

class Register {
public:
  Register() = default;

  sto_val_t value() const { return _val; }

private:
  sto_val_t _val;
  bool _is_general; // general register or Rob register?
  int _index; // 32 general registers / kRobSize RoB registers.
};

}

#endif // ISM_REGISTER_H