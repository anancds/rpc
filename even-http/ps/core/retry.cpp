#include <string>
#include <iostream>
#include <functional>
#include <unistd.h>

// https://stackoverflow.com/questions/14367726/generic-retry-mechanism-cannot-use-c11-features

// Minimalistic retry
bool retry(const std::function<bool()>& func, size_t max_attempts, unsigned long retry_interval_usecs) {
  for (size_t attempt{0}; attempt < max_attempts; ++attempt) {
    if (func()) {
      return true;
    }
    usleep(retry_interval_usecs);
  }
  return false;
}

// Ex1: function
int f(std::string const u) {
  std::cout << "f()" << std::endl;
  return false;
}

// Ex2: 'callable' class
struct A {
  bool operator()(std::string const &u, int z) {
    ++m_cnt;
    std::cout << "A::op() " << u << ", " << z << std::endl;

    if (m_cnt > 3) {
      return true;
    }
    return false;
  }

  int m_cnt{0};
};

int main() {
  A a;

  bool const r1 = retry([] { return f("stringparam1"); }, 3, 100);
  bool const r2 = retry([&] { return a("stringparam2", 77); }, 5, 300);
  // Ex 3: lambda
  bool const r3 = retry(
    []() -> bool {
      std::cout << "lambda()" << std::endl;
      return false;
    },
    5, 1000);

  std::cout << "Results: " << r1 << ", " << r2 << ", " << r3 << std::endl;

  return 0;
}