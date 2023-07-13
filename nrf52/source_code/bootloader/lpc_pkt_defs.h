/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Bootloader packet definitions.
 *              Based on the NXP LPC55S6x User manual, chapter 8.
 *              https://www.mouser.com/pdfDocs/NXP_LPC55S6x_UM.pdf
 *              See also MCU Bootloader v2.5.0 reference manual.
 *              https://www.nxp.com/docs/en/reference-manual/MCUBOOTRM.pdf
 */

#pragma once

#include <stdint.h>

// First byte of all packets.
// Note this is included in the CRC.
#define LPC_START_BYTE  (0x5A)

// Packet type bytes.
typedef enum 
{
    PKT_TYPE_ACK        = 0xA1,
    PKT_TYPE_NAK        = 0xA2,
    PKT_TYPE_ACK_ABORT  = 0xA3,
    PKT_TYPE_CMD        = 0xA4,
    PKT_TYPE_DATA       = 0xA5,
    PKT_TYPE_PING       = 0xA6,
    PKT_TYPE_PING_RSP   = 0xA7,
} pkt_type_e;

typedef struct
{
    uint8_t     start_byte;     // must be LPC_START_BYTE
    uint8_t     type;           // see pkt_type_e
} pkt_hdr_t;

typedef struct
{
    uint8_t     p_bugfix;       // protocol version
    uint8_t     p_minor;        // protocol version
    uint8_t     p_major;        // protocol version
    uint8_t     p_name;         // protocol name (must be 0x50, 'P')
    uint8_t     option_l;       // options low
    uint8_t     option_h;       // options high
    uint8_t     crc16_l;        // crc low
    uint8_t     crc16_h;        // crc high
} pkt_ping_rsp_t;

typedef struct 
{
    uint16_t    len;            // length in bytes of the command or 
                                // data that follows
    uint16_t    crc16;          // crc16, xmodem variant
} pkt_framing_hdr_t;

typedef enum
{
    // Command tags
    PKT_CMDRSP_TAG_FLASH_ERASE_ALL        = 0x01, // FlashEraseAll
    PKT_CMDRSP_TAG_FLASH_ERASE_REGION     = 0x02, // FlashEraseRegion
    PKT_CMDRSP_TAG_READ_MEM               = 0x03, // ReadMemory
    PKT_CMDRSP_TAG_WRITE_MEM              = 0x04, // WriteMemory
    PKT_CMDRSP_TAG_FILL_MEM               = 0x05, // FillMemory
    PKT_CMDRSP_TAG_FLASH_SEC_DISABLE      = 0x06, // FlashSecurityDisable
    PKT_CMDRSP_TAG_GET_PROPERTY           = 0x07, // GetProperty
    PKT_CMDRSP_TAG_RSVD                   = 0x08, // Reserved
    PKT_CMDRSP_TAG_EXECUTE                = 0x09, // Execute
    PKT_CMDRSP_TAG_CALL                   = 0x0A, // Call
    PKT_CMDRSP_TAG_RESET                  = 0x0B, // Reset
    PKT_CMDRSP_TAG_SET_PROPERTY           = 0x0C, // SetProperty
    PKT_CMDRSP_TAG_FLASH_ERASE_ALL_UNSEC  = 0x0D, // FlashEraseAllUnsecure
    PKT_CMDRSP_TAG_FLASH_PROGRAM_ONCE     = 0x0E, // FlashProgramOnce
    PKT_CMDRSP_TAG_FLASH_READ_ONCE        = 0x0F, // FlashReadOnce
    PKT_CMDRSP_TAG_FLASH_READ_RESOURCE1   = 0x10, // FlashReadResource
    PKT_CMDRSP_TAG_CONFIG_QUAD_SPI        = 0x11, // ConfigureQuadSPI
    PKT_CMDRSP_TAG_RELIABLE_UPDATE        = 0x12, // ReliableUpdate

    // Response tags
    PKT_CMDRSP_TAG_GENERIC_RSP            = 0xA0,
    PKT_CMDRSP_TAG_GET_PROPERTY_RSP       = 0xA7,
    PKT_CMDRSP_TAG_READ_MEMORY_RSP        = 0xA3,
    PKT_CMDRSP_TAG_FLASH_READ_ONCE_RSP    = 0xAF,
    PKT_CMDRSP_TAG_READ_RESOURCE_RSP      = 0xB0,
} pkt_cmdrsp_tag_e;

// Flags field.
// Bit 0 set to 1 indicates data packets follow.
// For example, set this when sending CMDRSP_TAG_WRITE_MEM, to specify that 
// PKT_TYPE_DATA will follow.
#define PKT_CMDRSP_FLAG_MORE_DATA   (0x01)

typedef struct 
{
    uint8_t     tag;            // see pkt_cmdrsp_tag_e
    uint8_t     flags;          // see PKT_CMDRSP_FLAG_*
    uint8_t     rsvd;           // reserved: must be 0
    uint8_t     param_count;    // number of 32 bit params
} pkt_cmdrsp_hdr_t;

// Max number of params for a command/response packet
#define PKT_CMDRSP_PARAMS_MAX   (7)

// PKT_TYPE_CMD
typedef struct
{
    pkt_framing_hdr_t   framing;
    pkt_cmdrsp_hdr_t    info;
    uint32_t            params[PKT_CMDRSP_PARAMS_MAX];
} pkt_cmdrsp_t;

// Max length in bytes of a command/response packet packet
#define PKT_CMDRSP_LEN_MAX      (sizeof(pkt_cmdrsp_t) - sizeof(pkt_framing_hdr_t))

// Max length in bytes of a data packet payload.
// This is purely a software constraint for receive buffer sizing.
// Read the PROP_MAX_PACKET_SIZE property to determine the actual max value.
#define PKT_DATA_LEN_MAX        (256UL)

// PKT_TYPE_DATA
typedef struct
{
    pkt_framing_hdr_t   framing;
    uint8_t             payload[PKT_DATA_LEN_MAX];
} pkt_data_t;

// Top level packet structure
typedef struct __attribute__ ((packed))
{
    // Start byte and type
    pkt_hdr_t hdr;

    union 
    {
        pkt_cmdrsp_t    cmdrsp;
        pkt_data_t      data;
        pkt_ping_rsp_t  pingrsp;
    } u;
} pkt_t;

// Properties available via GET_PROPERTY and SET_PROPERTY
typedef enum
{
    PROP_CURRENT_VER            = 0x01,
    PROP_AVAIL_PERIPH           = 0x02,
    PROP_FLASH_START_ADDR       = 0x03,
    PROP_FLASH_SIZE_BYTES       = 0x04,
    PROP_FLASH_SECTOR_SIZE      = 0x05,
    PROP_FLASH_BLOCK_COUNT      = 0x06,
    PROP_AVAIL_COMMANDS         = 0x07,
    PROP_CRC_CHECK_STATUS       = 0x08,
    PROP_RSVD                   = 0x09,
    PROP_VERIFY_WRITES          = 0x0A,     // writeable
    PROP_MAX_PACKET_SIZE        = 0x0B,
    PROP_RESERVED_REGIONS       = 0x0C,
    PROP_RAM_START_ADDR         = 0x0E,
    PROP_RAM_SIZE_BYTES         = 0x0F,
    PROP_SYSTEM_DEVICE_ID       = 0x10,
    PROP_FLASH_SECURITY_STATE   = 0x11,
    PROP_UNIQUE_DEVICE_ID       = 0x12,
    PROP_FLASH_FAC_SUPPORT      = 0x13,
    PROP_FLASH_ACCESS_SEG_SIZE  = 0x14,
    PROP_FLASH_ACCESS_SEG_COUNT = 0x15,
    PROP_FLASH_READ_MARGIN      = 0x16,     // writeable
    PROP_QSPI_INIT_STATUS       = 0x17,
    PROP_TARGET_VERSION         = 0x18,
    PROP_EXT_MEM_ATTRIB         = 0x19,
    PROP_RELIABLE_UPDATE_STATUS = 0x1A,
} property_type_t;