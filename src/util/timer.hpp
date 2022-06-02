#pragma once

#include <chrono>

struct Timer {
  std::chrono::steady_clock::time_point start;

  Timer() {
    start = std::chrono::high_resolution_clock::now();
  }

  void print_ms() {
    printf("%lldms\n", (std::chrono::high_resolution_clock::now() - start).count() / 1000000);
  }
};