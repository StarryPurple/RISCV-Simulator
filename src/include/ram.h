#ifndef ISM_RAM_H
#define ISM_RAM_H

namespace insomnia {

// Maybe someone will implement a RAM here.

template <int RAMCap>
class RAMProxy : public CPUModule {
public:
  bool update() override { return false; }
  void sync() override {}
};

}


#endif // ISM_RAM_H