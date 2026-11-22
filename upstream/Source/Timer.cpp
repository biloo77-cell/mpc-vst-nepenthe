#include "Timer.h"

/*

Timer is used to detect when playback has stopped, so there's no burst of noise from stored reverb
data when restarting.

It uses a 100-millisecond timeout as a heuristic.

*/

using namespace std::literals::chrono_literals;

bool Timer::boop() {
    started = true;

    const auto now = std::chrono::steady_clock::now();
    const bool timedOut = (now - lastBoop) > 100ms;

    lastBoop = now;
    return timedOut;
}

void Timer::reset() {
    started = false;
}
