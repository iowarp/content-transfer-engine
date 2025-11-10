#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(test_minimal, m) {
  m.doc() = "Minimal test module";
  m.def("hello", []() { return "Hello World!"; });
}
