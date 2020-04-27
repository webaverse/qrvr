#include "openvr/test/out.h"
#include <iostream>

std::ostream &getOut() {
  return std::cerr;
}