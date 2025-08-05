#include "cpu.h"
#include "bcpu.h"

int main() {
  insomnia::CPU cpu;
  cpu.preload_program();
  while(cpu.tick()) {}
  std::cout << cpu.get_ret() << std::endl;
  return 0;
}