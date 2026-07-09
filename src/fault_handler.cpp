#include <cinttypes>
// MeltyFC — Fault Handler Implementation (R3-2)
// Pure logic: breadcrumb formatting and CFSR decode.

#include "fault_handler.hpp"
#include <cstdio>
#include <cstring>

namespace melty {

static const char* faultTypeName(uint8_t type) {
    switch (type) {
    case 0:
        return "HardFault";
    case 1:
        return "BusFault";
    case 2:
        return "UsageFault";
    case 3:
        return "MemManage";
    default:
        return "Unknown";
    }
}

int decodeCfsr(uint32_t cfsr, char* buf, int bufLen) {
    int pos = 0;
    auto append = [&](const char* s) {
        int len = static_cast<int>(strlen(s));
        if (pos + len + 1 < bufLen) {
            memcpy(buf + pos, s, len);
            pos += len;
            buf[pos++] = ' ';
        }
    };

    // MemManage faults (bits 0-7)
    if (cfsr & (1 << 0))
        append("IACCVIOL");
    if (cfsr & (1 << 1))
        append("DACCVIOL");
    if (cfsr & (1 << 3))
        append("MUNSTKERR");
    if (cfsr & (1 << 4))
        append("MSTKERR");
    if (cfsr & (1 << 5))
        append("MLSPERR");
    if (cfsr & (1 << 7))
        append("MMARVALID");

    // BusFault (bits 8-15)
    if (cfsr & (1 << 8))
        append("IBUSERR");
    if (cfsr & (1 << 9))
        append("PRECISERR");
    if (cfsr & (1 << 10))
        append("IMPRECISERR");
    if (cfsr & (1 << 11))
        append("UNSTKERR");
    if (cfsr & (1 << 12))
        append("STKERR");
    if (cfsr & (1 << 13))
        append("LSPERR");
    if (cfsr & (1 << 15))
        append("BFARVALID");

    // UsageFault (bits 16-31)
    if (cfsr & (1 << 16))
        append("UNDEFINSTR");
    if (cfsr & (1 << 17))
        append("INVSTATE");
    if (cfsr & (1 << 18))
        append("INVPC");
    if (cfsr & (1 << 19))
        append("NOCP");
    if (cfsr & (1 << 24))
        append("UNALIGNED");
    if (cfsr & (1 << 25))
        append("DIVBYZERO");

    if (pos > 0)
        buf[pos - 1] = '\0'; // Remove trailing space
    else
        buf[0] = '\0';
    return pos;
}

int formatFaultDump(const FaultBreadcrumb& fb, char* buf, int bufLen) {
    if (!faultBreadcrumbValid(fb)) {
        return snprintf(buf, bufLen, "No fault recorded.\n");
    }

    char cfsrBuf[256];
    decodeCfsr(fb.cfsr, cfsrBuf, sizeof(cfsrBuf));

    return snprintf(buf, bufLen,
                    "=== FAULT DUMP ===\n"
                    "Type: %s\n"
                    "PC:   0x%08" PRIX32 "\n"
                    "LR:   0x%08" PRIX32 "\n"
                    "CFSR: 0x%08" PRIX32 " [%s]\n"
                    "HFSR: 0x%08" PRIX32 "\n"
                    "MMFAR: 0x%08" PRIX32 "\n"
                    "BFAR: 0x%08" PRIX32 "\n"
                    "Uptime: %lu ms\n",
                    faultTypeName(fb.faultType), fb.pc, fb.lr, fb.cfsr, cfsrBuf, fb.hfsr, fb.mmfar,
                    fb.bfar, (unsigned long)fb.uptime_ms);
}

} // namespace melty
