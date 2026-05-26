#ifndef _SENTINEL_SHARED_BUFFER_H_
#define _SENTINEL_SHARED_BUFFER_H_

#include <Uefi.h>

// Signature for verification
#define SENTINEL_SMM_BUFFER_SIGNATURE SIGNATURE_32('S', 'S', 'M', 'M')

// Maximum payload size for CPL3 -> CPL0 communications
#define SENTINEL_MAX_PAYLOAD_SIZE 4096

typedef enum {
    SMM_CMD_NONE = 0,
    SMM_CMD_READ_SPI,
    SMM_CMD_WRITE_SPI,
    SMM_CMD_GET_THERMAL,
    SMM_CMD_SET_POWER_STATE
} SENTINEL_SMM_COMMAND;

// Structure defining the Circular Shared Memory Buffer boundary
// between the CPL0 SMM Supervisor and CPL3 SMI Handlers
typedef struct {
    UINT32                  Signature;
    SENTINEL_SMM_COMMAND    Command;
    UINT32                  PayloadSize;
    EFI_STATUS              Status;
    
    // Cryptographic signature of the payload for operations like SPI Write
    // Checked by the CPL0 Supervisor before executing hardware operations
    UINT8                   CryptoSignature[256]; 
    
    // Circular buffer payload layout
    UINT8                   Payload[SENTINEL_MAX_PAYLOAD_SIZE];
} SENTINEL_SHARED_BUFFER;

#endif // _SENTINEL_SHARED_BUFFER_H_
