# MeltyFC Rocket Invariants (Round 3, 2026-07-06)

- I-1: Application code never writes internal flash
- I-2: No option-byte code is ever linked
- I-3: Motor DMA is non-circular; dead CPU stops frames within one loop period
- I-4: ESC signal-timeout is the measured, documented backstop for dead FC
- I-5: No DMA buffer in CCM — asserted at boot, not assumed
- I-6: Every init step is bounded-time; failure = ERROR blink, never a hang
- I-7: IWDG starts after init, before arming is reachable; arming requires it running
- I-8: Config storage is A/B + CRC + sequence + read-back; power loss at any instant leaves one valid copy
- I-9: Every USB write is guarded; no host = frames dropped, never blocked
- I-10: Recovery procedure exists in docs/RECOVERY.md and has been executed once on real hardware before first event
