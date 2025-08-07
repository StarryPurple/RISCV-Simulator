#include "cpu.h"
#include "bcpu.h"
#include <chrono>

int main() {
  // freopen("cpu.log", "w", stdout);
  insomnia::CPU cpu;
  cpu.preload_program();
  // auto beg = std::chrono::high_resolution_clock::now();
  while(cpu.tick()) {}
  // auto end = std::chrono::high_resolution_clock::now();
  // auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count();
  // auto [suc, tot] = cpu.pred_stat();
  // std::cout << "Dur: " << dur / 1000 << "'" << dur % 1000 << "s" << std::endl;
  // std::cout << "Clk tot: " << cpu.cycles() << std::endl;
  // std::cout << "Pred suc/tot: " << suc << '/' << tot << std::endl;
  std::cout << cpu.get_ret() << std::endl;
  return 0;
}