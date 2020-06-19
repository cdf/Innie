//
//  kern_start.cpp
//  Innie
//
//  Copyright © 2020 cdf. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_innie.hpp"

static INNIE innie;

const char *bootargOff[] {
    "-innieoff"
};

const char *bootargDebug[] {
    "-inniedbg"
};

const char *bootargBeta[] {
    "-inniebeta"
};

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
    bootargOff,
    1,
    bootargDebug,
    1,
    bootargBeta,
    1,
    KernelVersion::MountainLion,
    KernelVersion::Catalina,
    []() {
       innie.init();
    }
};
