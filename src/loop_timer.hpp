// MeltyFC — Loop-Time Instrumentation (C10 / L6)
// Tracks min/max/avg loop time in µs. CLI-visible, blackbox-logged.
// Soft event at >80% of budget. ERROR-logged at >150%.
// Pure logic — testable natively.

#pragma once

#include <cstdint>

namespace melty {

struct LoopTimerStats {
    uint32_t minUs;
    uint32_t maxUs;
    uint32_t avgUs;        // Running EMA
    uint32_t overBudget80;  // Count of loops >80% budget
    uint32_t overBudget150; // Count of loops >150% budget (ERROR events)
    uint32_t totalLoops;
};

class LoopTimer {
public:
    void init(uint32_t budgetUs = 500) {  // 2kHz = 500µs budget
        budgetUs_ = budgetUs;
        stats_ = {};
        stats_.minUs = UINT32_MAX;
        lastStartUs_ = 0;
    }

    // Call at the TOP of each loop iteration
    void startLoop(uint32_t nowUs) {
        if (lastStartUs_ != 0) {
            uint32_t elapsed = nowUs - lastStartUs_;  // Wrap-safe unsigned subtraction

            if (elapsed < stats_.minUs) stats_.minUs = elapsed;
            if (elapsed > stats_.maxUs) stats_.maxUs = elapsed;

            // EMA: avg = (7*avg + elapsed) / 8
            if (stats_.totalLoops == 0) {
                stats_.avgUs = elapsed;
            } else {
                stats_.avgUs = (stats_.avgUs * 7 + elapsed) / 8;
            }

            stats_.totalLoops++;

            if (elapsed > budgetUs_ * 80 / 100) stats_.overBudget80++;
            if (elapsed > budgetUs_ * 150 / 100) stats_.overBudget150++;
        }
        lastStartUs_ = nowUs;
    }

    const LoopTimerStats& stats() const { return stats_; }
    uint32_t budgetUs() const { return budgetUs_; }

private:
    uint32_t budgetUs_;
    uint32_t lastStartUs_;
    LoopTimerStats stats_;
};

} // namespace melty
