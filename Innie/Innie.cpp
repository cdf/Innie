//
//  Innie.cpp
//  Innie
//
//  Copyright © 2021 cdf. All rights reserved.
//

#include <IOKit/IOLib.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/IODeviceTreeSupport.h>

#include "Innie.hpp"

OSDefineMetaClassAndStructors(Innie, IOService)

bool Innie::init(OSDictionary *dict) {
    if (!IOService::init())
        return false;
    
    return true;
}

void Innie::free(void) {
    IOService::free();
}

IOService *Innie::probe(IOService *provider, SInt32 *score) {
    if (IOService::probe(provider, score)==0)
        return 0;
    
    return this;
}

bool Innie::start(IOService *provider) {
    DBGLOG("starting\n");
    
    if (!IOService::start(provider))
        return false;
    
    processRoot();
    IOService::registerService();
    return true;
}

void Innie::stop(IOService *provider) {
    IOService::stop(provider);
}


void Innie::processRoot() {
    if (auto entry = IORegistryEntry::fromPath("/", gIODTPlane)) {
        IORegistryEntry *pciRoot = nullptr;
        bool ready = false, found = false;
        size_t repeat = 0;
        do {
            if (auto iterator = entry->getChildIterator(gIODTPlane)) {
                while ((pciRoot = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
                    const char *name = pciRoot->getName();
                    if (name && !strncmp("PC", name, 2)) {
                        if (ready) {
                            DBGLOG("found PCI root %s", pciRoot->getName());
                            found = true;
                            while (OSDynamicCast(OSBoolean, pciRoot->getProperty("IOPCIConfigured")) != kOSBooleanTrue) {
                                DBGLOG("waiting for PCI root to be configured");
                                IOSleep(10);
                            }
                            recurseBridge(pciRoot);
                        } else {
                            IOSleep(1000); // Wait for other roots
                            ready = true;
                            break;
                        }
                    }
                }
                iterator->release();
            }
        } while (repeat++ < 0x10000000 && !found);
        entry->release();
    }
}

void Innie::recurseBridge(IORegistryEntry *entry) {
    if (auto iterator = entry->getChildIterator(gIODTPlane)) {
        IORegistryEntry *childEntry = nullptr;
        
        // Go through child entries of bridge, finding every other bridge and every SATA and NVMe device
        while ((childEntry = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
            uint32_t code = 0;
            if (auto class_code = childEntry->getProperty("class-code")) {
                if (auto codeData = OSDynamicCast(OSData, class_code)) {
                    code=*(uint32_t*)codeData->getBytesNoCopy();
                    if (code == classCode::SATADevice || code == classCode::NVMeDevice){
                        DBGLOG("found device %s", childEntry->getName());
                        if (auto built_in = childEntry->getProperty("built-in")) {
                            DBGLOG("device is already built-in");
                        } else {
                            internalizeDevice(childEntry);
                        }
                        break;
                    }
                    if (code == classCode::PCIBridge) {
                        DBGLOG("found bridge %s", childEntry->getName());
                        while (OSDynamicCast(OSBoolean, childEntry->getProperty("IOPCIConfigured")) != kOSBooleanTrue) {
                            DBGLOG("waiting for PCI bridge to be configured");
                            IOSleep(10);
                        }
                        recurseBridge(childEntry);
                    }
                }
            }
        }
        iterator->release();
    }
}
       
void Innie::internalizeDevice(IORegistryEntry *entry) {
    DBGLOG("adding built-in property");
    setBuiltIn(entry);
    
    while (OSDynamicCast(OSBoolean, entry->getProperty("IOPCIResourced")) != kOSBooleanTrue) {
        DBGLOG("waiting for device to be resourced");
        IOSleep(10);
    }
    
    // Proceed to update other properties
    if (auto driverIterator = IORegistryIterator::iterateOver(entry, gIOServicePlane, kIORegistryIterateRecursively)) {
        IORegistryEntry *driverEntry = nullptr;
        while ((driverEntry = OSDynamicCast(IORegistryEntry, driverIterator->getNextObject())) != nullptr) {
            DBGLOG("updating other properties");
            updateOtherProperties(driverEntry);
        }
        driverIterator->release();
    }
}

void Innie::setBuiltIn(IORegistryEntry *entry) {
    if (entry) {
        char dummy = '\0';
        entry->setProperty("built-in", &dummy, 1);
    }
}

void Innie::updateOtherProperties(IORegistryEntry *entry) {
    if (entry) {
        OSString *internal = OSString::withCString("Internal");
        OSString *internalIcon = OSString::withCString("Internal.icns");
        
        // Update icon
        if (auto icon = entry->getProperty("IOMediaIcon")) {
            if (auto dict = OSDynamicCast(OSDictionary, icon)) {
                dict = OSDictionary::withDictionary(dict);
                if (auto res = dict->getObject("IOBundleResourceFile")) {
                    dict->setObject("IOBundleResourceFile", internalIcon);
                    entry->setProperty("IOMediaIcon", dict);
                }
                dict->release();
            }
        }
        
        // Update interconnect
        if (auto loc = entry->getProperty("Physical Interconnect Location")) {
            if (auto prop = OSDynamicCast(OSString, loc)) {
                entry->setProperty("Physical Interconnect Location", internal);
            }
        }
        
        // Update update protocol characteristics
        if (auto proto = entry->getProperty("Protocol Characteristics")) {
            if (auto dict = OSDynamicCast(OSDictionary, proto)) {
                dict = OSDictionary::withDictionary(dict);
                if (auto loc = dict->getObject("Physical Interconnect Location")) {
                    if (auto prop = OSDynamicCast(OSString, loc)) {
                        dict->setObject("Physical Interconnect Location", internal);
                        entry->setProperty("Protocol Characteristics", dict);
                    }
                }
                dict->release();
            }
        }
        internal->release();
        internalIcon->release();
    }
}
