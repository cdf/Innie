//
//  kern_innie.hpp
//  Innie
//
//  Copyright © 2020 cdf. All rights reserved.
//

#ifndef kern_innie_hpp
#define kern_innie_hpp

// #include <Library/LegacyIOService.h>
#include <IOKit/IODeviceTreeSupport.h>

class INNIE {
public:
    void init();
    void deinit();
    
private:
    void internalizeDevices();
    void updateIcon(IORegistryEntry *propEntry);
    void updateInterconnect(IORegistryEntry *propEntry);
    void updateProtocolCharacteristics(IORegistryEntry *propEntry);
    
    struct ClassCode {
        enum : uint32_t {
            SATADevice     = 0x010601,
            NVMeDevice     = 0x010802,
        };
    };

};

#endif /* kern_innie */
