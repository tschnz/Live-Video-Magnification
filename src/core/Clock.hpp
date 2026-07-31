#pragma once

#include <chrono>

namespace livim {

using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point;

inline Timestamp now() { return Clock::now(); }

} // namespace livim
