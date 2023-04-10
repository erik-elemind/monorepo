/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Low level packet interface for the LPC bootloader.
 *              See links to NXP documentation in lpc_pkt_defs.h.
 */

#include "lpc_pkt.h"
#include <string.h> // for memset()

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME PKT
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Define a macro to get the number of elements in an array
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Define states for receive parsing
typedef enum 
{
    ST_START,       // Waiting for start byte
    ST_TYPE,        // Waiting for packet type
    ST_LEN_LOWER,   // Waiting for length, lower byte
    ST_LEN_UPPER,   // Waiting for length, upper byte
    ST_CRC_LOWER,   // Waiting for crc, lower byte
    ST_CRC_UPPER,   // Waiting for crc, upper byte
    ST_DATA,        // Collecting data bytes
    ST_DONE,        // Complete packet received
    ST_NUM,
} parse_state_e;

// Pointer to UART tx routine
static lpc_pkt_uart_tx_func_t m_uart_tx_func = NULL;

static const char* get_type_string(pkt_type_e type)
{
    switch (type)
    {
        case PKT_TYPE_ACK:
            return "PKT_TYPE_ACK";
        case PKT_TYPE_NAK:
            return "PKT_TYPE_NAK";
        case PKT_TYPE_ACK_ABORT:
            return "PKT_TYPE_ACK_ABORT";
        case PKT_TYPE_CMD:
            return "PKT_TYPE_CMD";
        case PKT_TYPE_DATA:
            return "PKT_TYPE_DATA";
        case PKT_TYPE_PING:
            return "PKT_TYPE_PING";
        case PKT_TYPE_PING_RSP:
            return "PKT_TYPE_PING_RSP";

        default:
            break;
    }

    return "UNKNOWN";
}

static const char* get_cmdrsp_type_string(uint8_t type)
{
    switch (type)
    {
        case PKT_CMDRSP_TAG_FLASH_ERASE_ALL:
            return "PKT_CMDRSP_TAG_FLASH_ERASE_ALL";
        case PKT_CMDRSP_TAG_FLASH_ERASE_REGION:
            return "PKT_CMDRSP_TAG_FLASH_ERASE_REGION";
        case PKT_CMDRSP_TAG_READ_MEM:
            return "PKT_CMDRSP_TAG_READ_MEM";
        case PKT_CMDRSP_TAG_WRITE_MEM:
            return "PKT_CMDRSP_TAG_WRITE_MEM";
        case PKT_CMDRSP_TAG_FILL_MEM:
            return "PKT_CMDRSP_TAG_FILL_MEM";
        case PKT_CMDRSP_TAG_FLASH_SEC_DISABLE:
            return "PKT_CMDRSP_TAG_FLASH_SEC_DISABLE";
        case PKT_CMDRSP_TAG_GET_PROPERTY:
            return "PKT_CMDRSP_TAG_GET_PROPERTY";
        case PKT_CMDRSP_TAG_RSVD:
            return "PKT_CMDRSP_TAG_RSVD";
        case PKT_CMDRSP_TAG_EXECUTE:
            return "PKT_CMDRSP_TAG_EXECUTE";
        case PKT_CMDRSP_TAG_CALL:
            return "PKT_CMDRSP_TAG_CALL";
        case PKT_CMDRSP_TAG_RESET:
            return "PKT_CMDRSP_TAG_RESET";
        case PKT_CMDRSP_TAG_SET_PROPERTY:
            return "PKT_CMDRSP_TAG_SET_PROPERTY";
        case PKT_CMDRSP_TAG_FLASH_ERASE_ALL_UNSEC:
            return "PKT_CMDRSP_TAG_FLASH_ERASE_ALL_UNSEC";
        case PKT_CMDRSP_TAG_FLASH_PROGRAM_ONCE:
            return "PKT_CMDRSP_TAG_FLASH_PROGRAM_ONCE";
        case PKT_CMDRSP_TAG_FLASH_READ_ONCE:
            return "PKT_CMDRSP_TAG_FLASH_READ_ONCE";
        case PKT_CMDRSP_TAG_FLASH_READ_RESOURCE1:
            return "PKT_CMDRSP_TAG_FLASH_READ_RESOURCE1";
        case PKT_CMDRSP_TAG_CONFIG_QUAD_SPI:
            return "PKT_CMDRSP_TAG_CONFIG_QUAD_SPI";
        case PKT_CMDRSP_TAG_RELIABLE_UPDATE:
            return "PKT_CMDRSP_TAG_RELIABLE_UPDATE";

        case PKT_CMDRSP_TAG_GENERIC_RSP:
            return "PKT_CMDRSP_TAG_GENERIC_RSP";
        case PKT_CMDRSP_TAG_GET_PROPERTY_RSP:
            return "PKT_CMDRSP_TAG_GET_PROPERTY_RSP";
        case PKT_CMDRSP_TAG_READ_MEMORY_RSP:
            return "PKT_CMDRSP_TAG_READ_MEMORY_RSP";
        case PKT_CMDRSP_TAG_FLASH_READ_ONCE_RSP:
            return "PKT_CMDRSP_TAG_FLASH_READ_ONCE_RSP";
        case PKT_CMDRSP_TAG_READ_RESOURCE_RSP:
            return "PKT_CMDRSP_TAG_READ_RESOURCE_RSP";

        default:
            break;
    }

    return "UNKNOWN";
}

static uint16_t crc16_update(const uint8_t * src, uint32_t lengthInBytes, const uint16_t* crc_init)
{
    uint32_t crc = 0;
    uint32_t j;

    // Are we continuing a previous calculation?
    if (NULL != crc_init)
    {
        crc = *crc_init;
    }

    for (j=0; j < lengthInBytes; ++j)
    {
        uint32_t i;
        uint32_t byte = src[j];
        crc ^= byte << 8;
        for (i = 0; i < 8; ++i)
        {
            uint32_t temp = crc << 1;
            if (crc & 0x8000)
            {
                temp ^= 0x1021;
            }
            crc = temp;
        }
    }
    return crc;
}

static uint16_t pkt_crc(const pkt_t* pkt)
{
    uint16_t crc_calc;

    if (PKT_TYPE_CMD == pkt->hdr.type)
    {
        // Init CRC with the header bytes, including start byte
        crc_calc = crc16_update(
            (uint8_t*)&pkt->hdr, sizeof(pkt->hdr), NULL);

        crc_calc = crc16_update(
            (uint8_t*)&pkt->u.cmdrsp.framing,
            sizeof(pkt->u.cmdrsp.framing) - sizeof(pkt->u.cmdrsp.framing.crc16),
            &crc_calc);

        crc_calc = crc16_update(
            (uint8_t*)&pkt->u.cmdrsp.info,
            pkt->u.cmdrsp.framing.len,
            &crc_calc);
    }
    else if (PKT_TYPE_DATA == pkt->hdr.type)
    {
        // Init CRC with the header bytes, including start byte
        crc_calc = crc16_update(
            (uint8_t*)&pkt->hdr, sizeof(pkt->hdr), NULL);

        crc_calc = crc16_update(
            (uint8_t*)&pkt->u.data.framing,
            sizeof(pkt->u.data.framing) - sizeof(pkt->u.data.framing.crc16),
            &crc_calc);

        crc_calc = crc16_update(
            (uint8_t*)&pkt->u.data.payload,
            pkt->u.data.framing.len,
            &crc_calc);
    }
    else if (PKT_TYPE_PING_RSP == pkt->hdr.type)
    {
        // Pingrsp CRC includes everything but the 2 trailing CRC bytes.
        crc_calc = crc16_update(
            (uint8_t*)pkt,
            sizeof(pkt->hdr) + sizeof(pkt->u.pingrsp) - 2,
            NULL);
    }
    else 
    {
        // No crc for this type
        crc_calc = 0xFFFF;
    }

    return crc_calc;
}

const lpc_pkt_evt_t*  lpc_pkt_handle_byte(uint8_t b)
{
    static parse_state_e state = ST_START;
    parse_state_e state_next = state;

    static pkt_t pkt;
    static uint16_t data_rem;
    static uint8_t* p_data;

    // Default to no event
    static lpc_pkt_evt_t evt;
    evt.evt_type = LPC_PKT_EVT_TYPE_NONE;

    switch (state)
    {
        case ST_START:
            memset(&pkt, 0, sizeof(pkt));
            pkt.hdr.start_byte = b;
            if (LPC_START_BYTE == b)
            {
                state_next = ST_TYPE;
            }
            else
            {
                NRF_LOG_WARNING("non-start byte: 0x%02x", b);
            }
            break;

        case ST_TYPE:
            pkt.hdr.type = b;
            if (PKT_TYPE_PING == b)
            {
                state_next = ST_DONE;
            }

            else if (PKT_TYPE_PING_RSP == b)
            {
                // ping response crc starts at the bugfix field
                p_data = (uint8_t*)&pkt.u.pingrsp;
                data_rem = sizeof(pkt_ping_rsp_t);
                state_next = ST_DATA;
            }

            else if (PKT_TYPE_ACK == b)
            {
                state_next = ST_DONE;
            }

            else if (PKT_TYPE_NAK == b)
            {
                state_next = ST_DONE;
            }

            else if (PKT_TYPE_CMD  == b ||
                     PKT_TYPE_DATA == b) 
            {
                state_next = ST_LEN_LOWER;
            }

            else 
            {
                NRF_LOG_WARNING("unknown packet type: 0x%02X", b);
                evt.evt_type = LPC_PKT_EVT_TYPE_RX_ERROR_TYPE;
                state_next = ST_START;
            }
            break;

        case ST_LEN_LOWER:
            pkt.u.cmdrsp.framing.len = b;
            state_next = ST_LEN_UPPER;
            break;

        case ST_LEN_UPPER:
            pkt.u.cmdrsp.framing.len += (uint16_t)b << 8;
            state_next = ST_CRC_LOWER;
            break;

        case ST_CRC_LOWER:
            pkt.u.cmdrsp.framing.crc16 = b;
            state_next = ST_CRC_UPPER;
            break;

        case ST_CRC_UPPER:
            pkt.u.cmdrsp.framing.crc16 += (uint16_t)b << 8;

            if (pkt.u.cmdrsp.framing.len > 0)
            {
                if (PKT_TYPE_CMD == pkt.hdr.type)
                {
                    if (pkt.u.cmdrsp.framing.len > PKT_CMDRSP_LEN_MAX)
                    {
                        NRF_LOG_WARNING("cmdrsp len exceeded. %u>%lu", pkt.u.cmdrsp.framing.len, PKT_CMDRSP_LEN_MAX);
                        evt.evt_type = LPC_PKT_EVT_TYPE_RX_ERROR_LEN;
                        state_next = ST_START;
                    }
                    else
                    {
                        p_data = (uint8_t*)&pkt.u.cmdrsp.info;
                        data_rem = pkt.u.cmdrsp.framing.len;
                        state_next = ST_DATA;
                    }
                }

                else if (PKT_TYPE_DATA == pkt.hdr.type)
                {
                    if (pkt.u.data.framing.len > PKT_DATA_LEN_MAX)
                    {
                        NRF_LOG_WARNING("data len exceeded. %d>%lu", pkt.u.data.framing.len, PKT_DATA_LEN_MAX);
                        state_next = ST_START;
                    }
                    else 
                    {
                        p_data = (uint8_t*)&pkt.u.data.payload;
                        data_rem = pkt.u.cmdrsp.framing.len;
                        state_next = ST_DATA;
                    }
                }
            }
            else 
            {
                state_next = ST_DONE;
            }
            break;

        case ST_DATA:
            if (data_rem > 0)
            {
                data_rem--;
                *p_data = b;
                p_data++;
            }

            if (0 == data_rem)
            {
                state_next = ST_DONE;
            }
            break;

        default:
            break;
    }

    if (state_next != state)
    {
        // NRF_LOG_WARNING("state %d->%d", state, state_next);
        if (ST_DONE == state_next)
        {
            uint16_t crc_calc = pkt_crc(&pkt);
            uint16_t crc_rx;

            NRF_LOG_INFO("RX type=%s (0x%02X)",
                get_type_string(pkt.hdr.type), pkt.hdr.type);

            if (PKT_TYPE_CMD == pkt.hdr.type)
            {
                crc_rx = pkt.u.cmdrsp.framing.crc16;
                NRF_LOG_INFO("  len=0x%04X, crc=0x%04X (0x%04X)", 
                    pkt.u.cmdrsp.framing.len, crc_rx, crc_calc);
                NRF_LOG_INFO("  tag=%s (0x%02X), flags=0x%02X, count=%d",
                    get_cmdrsp_type_string(pkt.u.cmdrsp.info.tag),
                    pkt.u.cmdrsp.info.tag,
                    pkt.u.cmdrsp.info.flags,
                    pkt.u.cmdrsp.info.param_count);
                for (uint32_t i=0; i<pkt.u.cmdrsp.info.param_count; i++)
                {
                    NRF_LOG_INFO("  param[%d]=0x%08X", i, pkt.u.cmdrsp.params[i]);
                }
            }
            else if (PKT_TYPE_DATA == pkt.hdr.type)
            {
                crc_rx = pkt.u.data.framing.crc16;
                NRF_LOG_INFO("  len=0x%04X, crc=0x%04X (0x%04X)",
                    pkt.u.data.framing.len, pkt.u.data.framing.crc16, crc_calc);
            }
            else if (PKT_TYPE_PING_RSP == pkt.hdr.type)
            {
                crc_rx = pkt.u.pingrsp.crc16_l + (pkt.u.pingrsp.crc16_h << 8);

                NRF_LOG_INFO("  crc=0x%04X (0x%04X)", crc_rx, crc_calc);
                NRF_LOG_INFO("  protocol ver=%d.%d.%d, name=0x%02X", 
                    pkt.u.pingrsp.p_major,
                    pkt.u.pingrsp.p_minor,
                    pkt.u.pingrsp.p_bugfix,
                    pkt.u.pingrsp.p_name);
                NRF_LOG_INFO("  opt=0x%02X%02X", pkt.u.pingrsp.option_h, pkt.u.pingrsp.option_l);
            }
            else 
            {
                crc_rx = crc_calc;
            }

            evt.evt_type = (crc_rx == crc_calc) ? 
                    LPC_PKT_EVT_TYPE_RX : LPC_PKT_EVT_TYPE_RX_ERROR_CRC,
            evt.pkt = &pkt;

            // Advance directly to start without waiting for a new byte
            state_next = ST_START;
        }
        state = state_next;
    }

    return &evt;
}

void lpc_pkt_init(lpc_pkt_uart_tx_func_t uart_tx_func)
{
    m_uart_tx_func = uart_tx_func;
}

int lpc_send_ping(void)
{
    static const uint8_t pkt[] = {
        LPC_START_BYTE,
        PKT_TYPE_PING
    };

    if (m_uart_tx_func)
    {
        return m_uart_tx_func((uint8_t*)&pkt, sizeof(pkt));
    }

    return -1;
}

int lpc_send_ack(void)
{
    static const uint8_t pkt[] = {
        LPC_START_BYTE,
        PKT_TYPE_ACK
    };

    NRF_LOG_INFO("TX ACK");

    if (m_uart_tx_func)
    {
        return m_uart_tx_func((uint8_t*)&pkt, sizeof(pkt));
    }

    return -1;
}

// Common cmdrsp sending routine
static int send_cmdrsp(pkt_cmdrsp_tag_e tag, uint32_t param_count, const uint32_t* params, uint8_t flags)
{
    // Buffer for outbound data, sized down to a max cmdrsp.
    static uint8_t tx_pkt_buf[sizeof(pkt_hdr_t) + sizeof(pkt_cmdrsp_t)];
    // Point to the buffer
    static pkt_t* pkt = (void*)tx_pkt_buf;

    pkt->hdr.start_byte             = LPC_START_BYTE;
    pkt->hdr.type                   = PKT_TYPE_CMD;
    pkt->u.cmdrsp.info.tag          = tag;
    pkt->u.cmdrsp.info.rsvd         = 0;
    pkt->u.cmdrsp.info.flags        = flags;
    pkt->u.cmdrsp.info.param_count  = param_count;

    memcpy(pkt->u.cmdrsp.params, params, sizeof(pkt->u.cmdrsp.params[0])*param_count);

    pkt->u.cmdrsp.framing.len = 
        sizeof(pkt->u.cmdrsp.info) + 
        sizeof(pkt->u.cmdrsp.params[0]) * param_count;

    pkt->u.cmdrsp.framing.crc16 = pkt_crc(pkt);

    NRF_LOG_INFO("TX CMDRSP:");
    NRF_LOG_INFO("  type=%s (0x%02X), len=0x%04X, crc=0x%04X (0x%04X)",
        get_type_string(pkt->hdr.type), pkt->hdr.type, 
        pkt->u.cmdrsp.framing.len, 0, pkt->u.cmdrsp.framing.crc16);
    NRF_LOG_INFO("  tag=%s (0x%02X), flags=0x%02X, count=%d",
        get_cmdrsp_type_string(pkt->u.cmdrsp.info.tag),
        pkt->u.cmdrsp.info.tag,
        pkt->u.cmdrsp.info.flags,
        pkt->u.cmdrsp.info.param_count);
    for (uint32_t i=0; i<pkt->u.cmdrsp.info.param_count; i++)
    {
        NRF_LOG_INFO("  param[%d]=0x%08X", i, pkt->u.cmdrsp.params[i]);
    }

    if (m_uart_tx_func)
    {
        uint32_t len = 
            sizeof(pkt->hdr) +
            sizeof(pkt->u.cmdrsp.framing) +
            sizeof(pkt->u.cmdrsp.info) + 
            pkt->u.cmdrsp.info.param_count * sizeof(pkt->u.cmdrsp.params[0]);
        return m_uart_tx_func((uint8_t*)pkt, len);
    }

    return -1;
}

int lpc_send_flash_erase_region1(void)
{
    static const uint32_t params1[] = {
        0x8000000 ,        // start addr
        0x400000       // len
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_FLASH_ERASE_REGION, ARRAY_SIZE(params1), params1, 0);
}

int lpc_send_flash_erase_region2(void)
{
    static const uint32_t params2[] = {
        0x8400000 ,        // start addr
        0x400000       // len
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_FLASH_ERASE_REGION, ARRAY_SIZE(params2), params2, 0);
}


int lpc_send_get_property(property_type_t prop)
{
    uint32_t params[] = {
        prop,
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_GET_PROPERTY, ARRAY_SIZE(params), params, 0);
}

int lpc_send_reset(void)
{
    return send_cmdrsp(PKT_CMDRSP_TAG_RESET, 0, NULL, 0);
}

int lpc_send_fill_mem1(void)
{
    uint32_t params[] = {
        0x1C000,         // start addr
        4,               // len
        0xC1000204,      // pattern
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_FILL_MEM, ARRAY_SIZE(params), params, PKT_CMDRSP_FLAG_MORE_DATA);
}

int lpc_send_fill_mem2(void)
{
    uint32_t params[] = {
        0x1C004,         // start addr
        4,               // len
        0x20000000,      // pattern
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_FILL_MEM, ARRAY_SIZE(params), params, PKT_CMDRSP_FLAG_MORE_DATA);
}

int lpc_send_config_mem(void)
{
    uint32_t params[] = {
        9,               // memory ID
        0x1C000,      // config block address
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_CONFIG_QUAD_SPI, ARRAY_SIZE(params), params, PKT_CMDRSP_FLAG_MORE_DATA);
}

int lpc_send_write_mem(uint32_t len)
{
    uint32_t params[] = {
        0x8000000 ,      // start addr
        len,    // len
        0,
    };
    return send_cmdrsp(PKT_CMDRSP_TAG_WRITE_MEM, ARRAY_SIZE(params), params, PKT_CMDRSP_FLAG_MORE_DATA);
}

int lpc_send_data(const uint8_t* p_data, uint32_t len)
{
    // Buffer for outbound data, sized down to a data packet header.
    static uint8_t tx_pkt_buf[sizeof(pkt_hdr_t) + sizeof(pkt_framing_hdr_t)];
    // Point to the buffer
    static pkt_t* pkt = (void*)tx_pkt_buf;
    uint16_t crc_calc;

    pkt->hdr.start_byte         = LPC_START_BYTE;
    pkt->hdr.type               = PKT_TYPE_DATA;
    pkt->u.data.framing.len     = len;

    // Init CRC with the header bytes, including start byte
    crc_calc = crc16_update(
        (uint8_t*)&pkt->hdr, sizeof(pkt->hdr), NULL);

    // Add the framing
    crc_calc = crc16_update(
        (uint8_t*)&pkt->u.data.framing,
        sizeof(pkt->u.data.framing) - sizeof(pkt->u.data.framing.crc16),
        &crc_calc);

    // And finally, the data buffer
    crc_calc = crc16_update(p_data, len, &crc_calc);

    pkt->u.data.framing.crc16 = crc_calc;
    NRF_LOG_INFO("TX DATA: type=%s (0x%02X), len=0x%04X, crc=0x%04X (0x%04X)", 
        get_type_string(pkt->hdr.type), pkt->hdr.type, 
        pkt->u.data.framing.len, 0, pkt->u.data.framing.crc16);

    if (m_uart_tx_func)
    {
        // Send our local buffer, and the data buffer
        if (0 == m_uart_tx_func(tx_pkt_buf, sizeof(tx_pkt_buf)) &&
            0 == m_uart_tx_func(p_data, len))
        {
            return 0;
        }
    }

    return -1;
}

