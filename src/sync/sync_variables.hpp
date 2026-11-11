#pragma once

#include <atomic>

namespace exchange::sync
{
    alignas(64) std::atomic<bool> killSwitch = false;
    alignas(64) std::atomic<bool> isIncomingClosed = false;
    alignas(64) std::atomic<bool> isProcessingClosed = false;
}