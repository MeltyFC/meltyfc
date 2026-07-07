// MeltyFC — DShot300 Motor Driver Implementation
// HAL/LL timer+DMA. Bidirectional GCR decode.
// P2 spike — hardest driver in the project. Prototype FIRST.
//
// DShot300 frame: 16 bits = 11-bit throttle + 1 telem bit + 4-bit CRC
// Bidir: inverted line, GCR-encoded eRPM telemetry from ESC
//
// STUB — implementation in P2.

#include "motors_dshot.hpp"
