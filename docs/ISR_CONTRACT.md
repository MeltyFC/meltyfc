# MeltyFC — ISR/Main Contract (L10)

Written BEFORE the DMA-complete callbacks exist. Prevents the shared-state
class by contract rather than reviewing it after the fact.

## The Boundary

ISR context (DMA transfer-complete callbacks) and main-loop context share
data through a narrow, documented interface. This contract defines what
crosses that boundary.

## Shared Variables

| Variable | Direction | Type | Atomicity | Owner |
|----------|-----------|------|-----------|-------|
| `telemReady[i]` | ISR→Main | volatile bool | Atomic (8-bit on M4/M7) | ISR sets, main clears |
| `txInProgress` | ISR→Main | volatile bool | Atomic | ISR clears, main sets |
| `ws2812Busy` | ISR→Main | volatile bool | Atomic | ISR clears, main checks |

## Rules (non-negotiable)

### ISR MAY:
- Set/clear a volatile bool flag
- Read (but not write) DMA buffer contents
- Call `__HAL_TIM_DISABLE_IT()` to stop further interrupts on this channel

### ISR MUST NEVER:
- **Allocate memory** (no malloc, no new, no snprintf into dynamic buffers)
- **Format strings** (no printf, no snprintf — these use the stack heavily)
- **Block** (no while loops, no HAL_Delay, no busy-waits)
- **Feed the IWDG** (I-13: watchdog reload at END of main loop ONLY)
- **Call Wire/I2C/SPI functions** (these may block on bus arbitration)
- **Modify DMA configuration** (turnaround is the exception — dshotBidirTurnaround
  is called from ISR context but only reconfigures its own channel)

### Atomicity Guarantee
- All shared variables are 32-bit-aligned or smaller on Cortex-M4/M7
- 32-bit aligned reads/writes are atomic by architecture (no mutex needed)
- `volatile` ensures the compiler doesn't cache or reorder across the boundary
- No multi-word structures cross the boundary

### Flag Protocol
1. ISR sets `telemReady[i] = true` when capture DMA completes
2. Main loop checks `telemReady[i]`, reads the buffer, clears the flag
3. The flag is the ONLY synchronization — no semaphores, no critical sections
4. If main hasn't read before the next ISR fires, the new data overwrites
   (latest-wins — acceptable for telemetry at 2kHz loop vs ~8kHz DShot rate)

## Integration Checklist
When wiring the DMA callbacks:
- [ ] Every shared variable is in this table
- [ ] Every ISR body is <20 lines (audit for bloat)
- [ ] No ISR calls any function not on the MAY list
- [ ] verify.sh grep-gate: `IWDG` must not appear in any ISR handler
- [ ] The main loop's IWDG feed is the LAST thing before the loop restarts
