#pragma once

#include <atomic>

namespace exchange::sync
{
    alignas(64) inline std::atomic<bool> killSwitch = false;
    alignas(64) inline std::atomic<bool> isIncomingClosed = false;
    alignas(64) inline std::atomic<bool> isProcessingClosed = false;
}