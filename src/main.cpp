#include "cpu.h"

int main() {
  insomnia::CPU cpu;
  cpu.preload_program();
  while(cpu.tick()) {}
  std::cout << (cpu.get_reg(10) & 0xff) << std::endl;
  return 0;
}
