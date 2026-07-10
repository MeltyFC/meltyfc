// MeltyFC — Unused GPIO Init (C12 / L9)
// Set unused pins to analog mode — prevents noise coupling + saves µA.
// Speed rationale: VERY_HIGH on DShot pins (timing-critical), LOW on everything else.
//
// Each target implements gpioInitUnused() with its specific unused pin list.
// Called once at boot, after clock + peripheral init.

#pragma once

namespace melty {
namespace hal {

// Per-target: configure all unused GPIO pins to analog input (lowest power, no noise)
void gpioInitUnused();

} // namespace hal
} // namespace melty
