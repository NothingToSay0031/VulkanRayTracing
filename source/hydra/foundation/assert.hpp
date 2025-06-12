#pragma once

#include "foundation/log.hpp"

namespace hydra {

#define RASSERT(condition)              \
  if (!(condition)) {                   \
    rprint(HYDRA_FILELINE("FALSE\n")); \
    HYDRA_DEBUG_BREAK                  \
  }
#if defined(_MSC_VER)
#define RASSERTM(condition, message, ...)                               \
  if (!(condition)) {                                                   \
    rprint(HYDRA_FILELINE(HYDRA_CONCAT(message, "\n")), __VA_ARGS__); \
    HYDRA_DEBUG_BREAK                                                  \
  }
#else
#define RASSERTM(condition, message, ...)                                 \
  if (!(condition)) {                                                     \
    rprint(HYDRA_FILELINE(HYDRA_CONCAT(message, "\n")), ##__VA_ARGS__); \
    HYDRA_DEBUG_BREAK                                                    \
  }
#endif

}  // namespace hydra
