// MeltyFC — Loop-Time Instrumentation (C10 / L6 / C7)
// Tracks BOTH period (start-to-start) and execution (start-to-end) in µs.
// Period = loop rate. Execution = CPU time consumed. Difference = headroom.
// Soft event at >80% of budget. ERROR-logged at >150%.
// Pure logic — testable natively.

#pragma once

#include <cstdint>

namespace melty {

struct LoopTimerStats {
    // Period: start-of-loop N to start-of-loop N+1
    uint32_t minUs;
    uint32_t maxUs;
    uint32_t avgUs;         // Running EMA

    // C7: Execution: start-of-loop to end-of-work (before idle/wait)
    uint32_t execMinUs;
    uint32_t execMaxUs;
    uint32_t execAvgUs;     // Running EMA

    uint32_t overBudget80;  // Count of loops >80% budget (period)
    uint32_t overBudget150; // Count of loops >150% budget (ERROR events)
    uint32_t totalLoops;
};

class LoopTimer {
public:
    void init(uint32_t budgetUs = 500) {  // 2kHz = 500µs budget
        budgetUs_ = budgetUs;
        stats_ = {};
        stats_.minUs = UINT32_MAX;
        stats_.execMinUs = UINT32_MAX;
        lastStartUs_ = 0;
        currentStartUs_ = 0;
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
        currentStartUs_ = nowUs;
    }

    // C7: Call at the END of the loop body (before idle/wait/yield).
    // Captures execution time = time spent doing actual work this iteration.
    void endLoop(uint32_t nowUs) {
        if (currentStartUs_ == 0) return;  // No matching startLoop yet

        uint32_t execTime = nowUs - currentStartUs_;  // Wrap-safe

        if (execTime < stats_.execMinUs) stats_.execMinUs = execTime;
        if (execTime > stats_.execMaxUs) stats_.execMaxUs = execTime;

        // EMA for execution time
        if (stats_.totalLoops <= 1 && stats_.execAvgUs == 0) {
            stats_.execAvgUs = execTime;
        } else {
            stats_.execAvgUs = (stats_.execAvgUs * 7 + execTime) / 8;
        }

        currentStartUs_ = 0;  // Prevent double-end
    }

    const LoopTimerStats& stats() const { return stats_; }
    uint32_t budgetUs() const { return budgetUs_; }

    // C7: Headroom = budget minus average execution time (µs of slack per loop)
    uint32_t headroomUs() const {
        if (stats_.execAvgUs >= budgetUs_) return 0;
        return budgetUs_ - stats_.execAvgUs;
    }

private:
    uint32_t budgetUs_;
    uint32_t lastStartUs_;
    uint32_t currentStartUs_;
    LoopTimerStats stats_;
};

} // namespace melty
