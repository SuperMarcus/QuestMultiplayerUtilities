#pragma once

#include "utils/logging.hpp"
#include "utils/typedefs.h"
#include "modloader.hpp"

inline std::string to_string(const Il2CppString *a) {
    if (!a) return {};
    return to_utf8(csstrtostr(const_cast<Il2CppString*>(a)));
}

inline std::string operator +(const Il2CppString *a, const std::string& b) {
    return to_string(a) + b;
}

inline std::string operator +(const std::string& a, Il2CppString *b) {
    return a + to_string(b);
}

Logger& getLogger();
const ModInfo& getModInfo();
