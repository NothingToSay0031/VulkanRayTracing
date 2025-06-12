#pragma once

#include "foundation/platform.hpp"

namespace hydra {

struct Service {
  virtual void init(void* configuration) {}
  virtual void shutdown() {}

};  // struct Service

#define HYDRA_DECLARE_SERVICE(Type) static Type* instance();

}  // namespace hydra
