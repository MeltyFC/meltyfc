# MeltyFC Recovery Procedures

**This document is for someone panicking at 11pm before an event.**
Read the section that matches your problem. Every command is copy-pasteable.

---

## My bot won't boot / is bricked / does nothing

**You cannot truly brick this board.** The STM32F405's DFU bootloader is in mask ROM —
unerasable, unmodifiable. Recovery always works:

1. **Unplug everything** (battery, USB).
2. **Hold the BOOT button** on the FC (tiny button near the USB connector).
3. While holding BOOT, **plug in USB**. Release BOOT after 2 seconds.
4. The board is now in DFU mode. Your computer should see USB device `0483:df11`.

### Re-flash MeltyFC:
```bash
dfu-util -a 0 -s 0x08000000:leave -D firmware.bin
```

### Restore Betaflight:
```bash
# Download the correct BF hex from:
# https://github.com/betaflight/unified-targets/
dfu-util -a 0 -s 0x08000000:leave -D betaflight_4.x_STM32F405.bin
```

### Full erase (if config is corrupted):
```bash
dfu-util -a 0 -s 0x08000000:mass-erase:force -D firmware.bin
```

### Using STM32CubeProgrammer (GUI alternative):
1. Open STM32CubeProgrammer
2. Connect: USB, DFU mode
3. Download: select .bin or .hex file
4. Verify after download
5. Disconnect

---

## My bot spins on its own / won't stop

1. **Kill power immediately** — pull the battery.
2. This should never happen. If it did, file a bug report with:
   - What you were doing when it started
   - Whether you had USB connected
   - Your ELRS RX failsafe setting (must be "No Pulses")

---

## ELRS receiver setup (MANDATORY before first use)

1. Connect to your EP1 RX via WiFi (power the FC, EP1 creates a WiFi AP).
2. In the ELRS web UI: **Failsafe Mode → No Pulses**.
3. **Never use "Last Position" or "Set Position"** — MeltyFC's LQ failsafe
   handles these, but No Pulses is the correct primary setting.

---

## USB CLI not responding

The CLI uses USB CDC (virtual COM port). If it stops responding:

1. Unplug and replug USB.
2. If that fails, enter DFU mode (hold BOOT + plug USB) and re-flash.
3. CLI is disabled while ARMED — disarm first.

---

## Config is wrong / bot behaves strangely after a firmware update

```
# Connect via USB serial, then:
defaults    # Reset all params to factory
save        # Write to flash
```

If USB CLI is unresponsive, do a full erase + re-flash (see above).

---

## How to check firmware version

```
# Via USB CLI:
version
```

---

## Emergency contacts

- **MeltyFC GitHub Issues**: https://github.com/MeltyFC/meltyfc/issues
- **SPARC safety rules**: https://sparc.tools
