//
//  kern_innie.cpp
//  Innie
//
//  Copyright © 2020 cdf. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

#include "kern_innie.hpp"

void INNIE::init() {
    lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
        static_cast<INNIE *>(user)->internalizeDevices();
    }, this);
}

// Any SATA (AHCI) or NVMe device behind PCIe bridges will be seen as internal.
void INNIE::internalizeDevices() {
    WIOKit::findEntryByPrefix("/AppleACPIPlatformExpert", "PC", gIOServicePlane, [](void *user, IORegistryEntry *pciRoot) {
        if (auto pciIterator = pciRoot->getChildIterator(gIODTPlane)) {
            IORegistryEntry *pciEntry = nullptr;
            // Go through PCI entries, finding every bridge.
            while ((pciEntry = OSDynamicCast(IORegistryEntry, pciIterator->getNextObject())) != nullptr) {
                uint32_t code = 0;
                if (!WIOKit::getOSDataValue(pciEntry, "class-code", code))
                    continue;
                if (code == WIOKit::ClassCode::PCIBridge) {
                    if (auto bridgeIterator = IORegistryIterator::iterateOver(pciEntry, gIOServicePlane, kIORegistryIterateRecursively)) {
                        IORegistryEntry *bridgeEntry = nullptr;
                        // Enter bridge, finding every non-built-in SATA or NVMe entry.
                        while ((bridgeEntry = OSDynamicCast(IORegistryEntry, bridgeIterator->getNextObject())) != nullptr) {
                            if (!WIOKit::getOSDataValue(bridgeEntry, "class-code", code))
                                continue;
                            if (code == INNIE::ClassCode::SATADevice || code == INNIE::ClassCode::NVMeDevice) {
                                DBGLOG("innie", "found device");
                                if(bridgeEntry->getProperty("built-in")) {
                                    DBGLOG("innie", "device already internal");
                                    break;
                                }
                                if (auto deviceIterator = IORegistryIterator::iterateOver(bridgeEntry, gIOServicePlane, kIORegistryIterateRecursively | kIORegistryIterateParents)) {
                                    IORegistryEntry *deviceEntry = nullptr;
                                    // Go from device back to first bridge, checking that there is nothing but bridges between them.
                                    while ((deviceEntry = OSDynamicCast(IORegistryEntry, deviceIterator->getNextObject())) != nullptr) {
                                        if (!WIOKit::getOSDataValue(deviceEntry, "class-code", code))
                                            continue;
                                        if (code != WIOKit::ClassCode::PCIBridge)
                                            break;
                                        if (deviceEntry == pciEntry) {
                                            DBGLOG("innie", "making device internal");
                                            if (auto driverIterator = IORegistryIterator::iterateOver(bridgeEntry, gIOServicePlane, kIORegistryIterateRecursively)) {
                                                IORegistryEntry *driverEntry = nullptr;
                                                // Go forward from device, updating properties.
                                                while ((driverEntry = OSDynamicCast(IORegistryEntry, driverIterator->getNextObject())) != nullptr) {
                                                    static_cast<INNIE *>(user)->updateIcon(driverEntry);
                                                    static_cast<INNIE *>(user)->updateInterconnect(driverEntry);
                                                    static_cast<INNIE *>(user)->updateProtocolCharacteristics(driverEntry);
                                                }
                                                driverIterator->release();
                                            }
                                            break;
                                        }
                                    }
                                    deviceIterator->release();
                                }
                            }
                        }
                        bridgeIterator->release();
                    }
                }
            }
            pciIterator->release();
        }
        return true;
    });
}

void INNIE::updateIcon(IORegistryEntry *propEntry) {
    if(propEntry) {
        if(auto icon = propEntry->getProperty("IOMediaIcon")) {
            if(auto dict = OSDynamicCast(OSDictionary, icon)) {
                dict = OSDictionary::withDictionary(dict);
                if(auto res = dict->getObject("IOBundleResourceFile")) {
                    dict->setObject("IOBundleResourceFile", OSString::withCString("Internal.icns"));
                    propEntry->setProperty ("IOMediaIcon", dict);
                }
            }
        }
    }
}

void INNIE::updateInterconnect(IORegistryEntry *propEntry) {
    if(propEntry) {
        if(auto loc = propEntry->getProperty("Physical Interconnect Location"))
            propEntry->setProperty("Physical Interconnect Location", OSString::withCString("Internal"));
    }
}

void INNIE::updateProtocolCharacteristics(IORegistryEntry *propEntry) {
    if(propEntry) {
        if(auto proto = propEntry->getProperty("Protocol Characteristics")) {
            if(auto dict = OSDynamicCast(OSDictionary, proto)) {
                dict = OSDictionary::withDictionary(dict);
                if(auto loc = dict->getObject("Physical Interconnect Location")) {
                    if(auto prop = OSDynamicCast(OSString, loc)) {
                        dict->setObject("Physical Interconnect Location", OSString::withCString("Internal"));
                        propEntry->setProperty("Protocol Characteristics", dict);
                    }
                }
            }
        }
    }
}
