# MeltyFC Deep Findings — Round 2 (Adversarial Pass, 2026-07-06)
# See MeltyFC_Deep_Findings_Round2.md for full content

## S-TIER (System Killers)
- S1: Watchdog vs internal-flash erase = guaranteed reset during save
- S2: Creep/tank mode unimplementable — DShot has no reverse
- S3: USB CDC blocking writes with no host = loop stall
- S4: I2C bus budget — two H3LIS reads eat the entire loop

## A-TIER (Wrong Behavior in Field)
- A1: Config THROTTLE_SPIN_MAX > THROTTLE_CAP = reversed translation
- A2: Degrees vs radians boundary undefined
- A3: Low-speed mode has no R_ICM parameter
- A4: Channel-map collisions legal
- A5: Bidir DShot EDT handshake + 17th buffer slot + line turnaround
- A6: Hit detection false-fires on every spin-up
- A7: Inversion × creep interaction unhandled
- A8: NUM_DRIVE_MOTORS=1 is legal and broken

## Rulings
- Config moves to SPI flash (deletes S1 class entirely)
- Creep descoped to forward-only for v1
- USB writes guarded with availableForWrite check
- validateConfig() cross-param hook required
