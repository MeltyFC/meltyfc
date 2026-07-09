// MeltyFC — Flash Storage HAL Contract
// Each chip family implements SPI flash or SDMMC storage access.
//
// R7-5: The write sequence MUST include readback-verify:
//   1. Write new config to the write slot
//   2. Read it back and CRC-verify (the existing CRC routine)
//   3. Only THEN invalidate the old slot
//   If readback fails → old slot is still valid, no data loss.
//
// The SPI busy-poll MUST have an explicit timeout (not unbounded).
// Timeout → return error, caller retries or enters ERROR state.

#pragma once

#include <cstdint>
#include <cstdbool>

namespace melty {
namespace hal {

// Initialize the flash storage peripheral (SPI flash or SDMMC).
// Returns true if the device is detected and ready.
bool flashInit();

// Read `len` bytes from flash at `addr` into `buf`.
// Returns true on success, false on timeout or bus error.
bool flashRead(uint32_t addr, void* buf, uint32_t len);

// Write `len` bytes from `buf` to flash at `addr`.
// Handles page-program commands and busy-wait with timeout.
// Returns true on success, false on timeout or verify failure.
bool flashWrite(uint32_t addr, const void* buf, uint32_t len);

// Erase a sector at `addr`. Flash-specific (4KB typical for SPI NOR).
// Returns true on success, false on timeout.
bool flashEraseSector(uint32_t addr);

// Check if flash is busy (write/erase in progress).
bool flashBusy();

// Read the JEDEC ID (for device identification at init).
// Returns 0 if not available.
uint32_t flashReadJedecId();

// ============================================================================
// R7-5: Config save sequence (implemented by core, calls HAL primitives):
//
//   ConfigSlotHeader newHeader = buildSlotHeader(nextSeq);
//   int writeSlot = pickWriteSlot(slot0, slot1);
//   uint32_t writeAddr = slotAddress(writeSlot);
//
//   flashEraseSector(writeAddr);
//   flashWrite(writeAddr, &newHeader, sizeof(newHeader));
//   flashWrite(writeAddr + sizeof(newHeader), &newData, sizeof(newData));
//
//   // READBACK-VERIFY before invalidating old slot:
//   ConfigData readback;
//   flashRead(writeAddr + sizeof(newHeader), &readback, sizeof(readback));
//   if (computeConfigCrc(readback) != readback.crc32) {
//       // Write failed — old slot still valid, no data loss
//       return false;
//   }
//
//   // Only now is it safe to consider the write committed.
//   // Old slot is NOT explicitly invalidated — it just has a lower sequence.
// ============================================================================

} // namespace hal
} // namespace melty
