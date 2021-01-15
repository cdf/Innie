//
//  Innie.hpp
//  Innie
//
//  Copyright © 2021 cdf. All rights reserved.
//

#ifndef Innie_hpp
#define Innie_hpp

#include <IOKit/IOService.h>

class Innie : public IOService {
    OSDeclareDefaultStructors(Innie)
    
public:
    virtual bool init(OSDictionary *dictionary = NULL) override;
    virtual void free(void) override;
    virtual IOService *probe(IOService *provider, SInt32 *score) override;
    virtual void stop(IOService *provider) override;
    virtual bool start(IOService *provider) override;
    
private:
    void processRoot();
    void recurseBridge(IORegistryEntry *entry);
    void internalizeDevice(IORegistryEntry *entry);
    void setBuiltIn(IORegistryEntry *entry);
    void updateOtherProperties(IORegistryEntry *entry);
    
    struct classCode {
        enum : uint32_t {
            PCIBridge      = 0x060400,
            SATADevice     = 0x010601,
            NVMeDevice     = 0x010802,
        };
    };
};

#ifdef DEBUG
#define DBGLOG(args...) do { IOLog("Innie: " args); } while (0)
#else
#define DBGLOG(args...) do { } while (0)
#endif

#endif /* Innie */
