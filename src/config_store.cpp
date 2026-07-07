// MeltyFC — Config Store Implementation
// STUB — implementation in P6.

#include "config_store.hpp"
#include "param_registry.h"

namespace melty {

// Parameter registry definition — the single source of truth for all configurable values.
// CLI `list` and future GUI autogenerate from this array.
// STUB — will be fully populated in P6 when the CLI is built.

const ParamDef PARAM_REGISTRY[] = {
    // Populated in P6
};

const size_t PARAM_REGISTRY_SIZE = sizeof(PARAM_REGISTRY) / sizeof(PARAM_REGISTRY[0]);

const ParamDef* findParam(const char* name) {
    (void)name;
    return nullptr;  // STUB
}

float getParamFloat(const ConfigData& cfg, const ParamDef& def) {
    (void)cfg; (void)def;
    return 0.0f;  // STUB
}

bool setParamFloat(ConfigData& cfg, const ParamDef& def, float value) {
    (void)cfg; (void)def; (void)value;
    return false;  // STUB
}

int formatParam(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen) {
    (void)cfg; (void)def; (void)buf; (void)bufLen;
    return 0;  // STUB
}

} // namespace melty
