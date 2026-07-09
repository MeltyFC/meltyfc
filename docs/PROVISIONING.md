# MeltyFC — Board Provisioning Procedure (L7)

One-time setup per physical board. Done ONCE at the workbench, never in the field.

## Prerequisites
- STM32CubeProgrammer installed
- Board connected via USB (DFU mode) or SWD
- MeltyFC firmware already flashed and verified working

## Steps

### 1. Flash Firmware
```bash
pio run -e <target> -t upload
```

### 2. Verify Boot
- Connect serial terminal (115200)
- `version` — confirm git hash matches expected build
- I-12 boot assert should NOT fire (correct clock = normal boot)

### 3. Set Option Bytes (STM32CubeProgrammer)
I-2 bans option-byte code in the application — these are set by a human tool.

| Option | Value | Why |
|--------|-------|-----|
| BOR Level | Level 2 or 3 | Defined brownout behavior in the 1.7-2.9V twilight |
| IWDG_SW | Software start | Watchdog starts in code (I-7), not from reset |
| nRST_STOP | Enabled | NRST works in Stop mode |

### 4. Record Board Identity
For each board, record in the build log:

```
Board: _______________
Target: _______________
UID: _______________  (CLI: `uid` or read DBGMCU_IDCODE)
Silicon Rev: _______________  (E-05: must be rev V+ for any future 480MHz experiment on H743)
BOR Level: _______________
IWDG Mode: Software
Firmware Hash: _______________
Provisioning Date: _______________
```

### 5. Golden Image Hash
After provisioning, compute and record the firmware binary hash:
```bash
sha256sum .pio/build/<target>/firmware.bin
```
This is the reference for field verification — a fielded board's flash
content can be read back and compared.

### 6. EOL Quick-Test (for board #2..N)
Subset of BENCH_TEST.md for production boards (already provisioned model):
- [ ] Power on, status LED visible
- [ ] `version` matches expected hash
- [ ] I2C scan: 0x18 + 0x19 respond
- [ ] Arm + disarm cycle works
- [ ] TX-off failsafe stops motors

Full BENCH_TEST.md only required for first board of each target type.
