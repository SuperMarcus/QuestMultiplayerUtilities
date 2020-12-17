#include "multiplayer.hpp"
#include "private.hpp"
#include "modloader.hpp"
#include "utils/logging.hpp"

void InstallHooks();

static ModInfo modInfo;

const Logger& getLogger() {
    static const Logger logger(modInfo);
    return logger;
}

const ModInfo& getModInfo() {
    return modInfo;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
    getLogger().info("Setting up multiplayer tools.");
}

extern "C" void load() {
    getLogger().info("Loading multiplayer tools & patches...");
//    PTRegisterTypes();
//    ParticleTuner::icall_functions::resolve_icalls();
//    PTInstallHooks();
    InstallHooks();
//    PTRegisterUI(modInfo);
    getLogger().info("Finished loading multiplayer tools.");
}
