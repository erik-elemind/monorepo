/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: LPC interface for applying FW updates.
 */

#include "lpc_protocol.h"
#include "lpc_pkt.h"
#include "custom_board.h"

// nordic sdk
#include "nrf_delay.h"
#include "nrf_bootloader_dfu_timers.h"

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME LP
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#ifndef MIN
#define MIN(a,b)    ((a)<(b)?(a):(b))
#endif

// Default time to wait for a response
#define LPC_DEFAULT_WAIT_MS     (500)

// Helper macro which evaluates an **integer expression** (or function which returns an int).
// If expression is non-zero, the integer expression is logged, and returned immediately.
// This is helpful in unifying a common code pattern in this file. 
#define RETURN_IF_NONZERO(expr) \
    do { \
        int ii = expr; \
        if (ii) \
        { \
            NRF_LOG_WARNING("nonzero status (%d) at line %d", ii, __LINE__); \
            return ii; \
        } \
    } while(0)

// Helper macro which takes a **pointer expression** (or function which returns a pointer). 
// If expression is null, -1 is returned.
// This is helpful in unifying a common code pattern in this file. 
#define RETURN_IF_NULL(expr) \
    if (NULL == expr) \
    { \
        NRF_LOG_WARNING("null result at line %d", __LINE__); \
        return -1; \
    }

static void lpc_pkt_handler(const lpc_pkt_evt_t* p_evt)
{
    if (LPC_PKT_EVT_TYPE_RX == p_evt->evt_type)
    {
        // Always send ACK for received CMDs and DATA
        if (PKT_TYPE_CMD  == p_evt->pkt->hdr.type || 
            PKT_TYPE_DATA == p_evt->pkt->hdr.type)
        {
            (void)lpc_send_ack();
        }
    }
}

static uint32_t get_tick_ms(void)
{
    #ifdef __arm__
    // For Nordic builds, use the bootloader provided tick counter.
    // Per nrf_bootloader_dfu_timers.h, the tick runs at 32768 ticks per second.
    // We round it to 33 ticks per millisecond.
    return nrf_bootloader_dfu_timer_counter_get() / 33;
    #else
    // Use gettimeofday to get the ticks. 
    // The result will truncate into a uint32_t, but that is fine for this use.
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return tval.tv_sec * 1000.0 + tval.tv_usec / 1000.0;
    #endif
}

static const pkt_t* wait_for_pkt(pkt_type_e type, uint32_t ms)
{
    const lpc_pkt_evt_t* p_evt;
    uint32_t elapsed_ms = 0;
    uint32_t ts_start = get_tick_ms();
    uint8_t b;

    if (0 == ms)
    {
        ms = LPC_DEFAULT_WAIT_MS;
    }

    while (elapsed_ms < ms)
    {
        while (0 == lpc_uart_read_byte(&b))
        {
            p_evt = lpc_pkt_handle_byte(b);
            if (p_evt)
            {
                lpc_pkt_handler(p_evt);

                if (LPC_PKT_EVT_TYPE_RX == p_evt->evt_type)
                {
                    if (type == p_evt->pkt->hdr.type)
                    {
                        NRF_LOG_DEBUG("  elapsed=%u (of %u) (type=0x%02X)", 
                            elapsed_ms, ms, p_evt->pkt->hdr.type);
                        return p_evt->pkt;
                    }
                }
            }
        }
        elapsed_ms = get_tick_ms() - ts_start;
    }

    return NULL;
}

static const int wait_for_pkt_rsp(pkt_cmdrsp_tag_e tag, uint32_t ms, const uint32_t** pp_params)
{
    const pkt_t* p_pkt = wait_for_pkt(PKT_TYPE_CMD, ms);
    if (p_pkt)
    {
        if (tag == p_pkt->u.cmdrsp.info.tag)
        {
            // Provide the full params, if the caller is interested
            if (pp_params)
            {
                *pp_params = p_pkt->u.cmdrsp.params;
            }
            // Return the status code. This is 0 for success (kStatus_Success)
            return 0 == p_pkt->u.cmdrsp.params[0] ? 0 : -1;
        }
        else 
        {
            NRF_LOG_WARNING("other rsp. tag=0x%02X, tagrx=0x%02X", tag, p_pkt->u.cmdrsp.info.tag);
        }
    }
    else
    {
        NRF_LOG_WARNING("no rsp. tag=0x%02X", tag);
    }

    // Timeout
    return -2;
}

int lpc_protocol_init(void)
{
    // The number of ping messages sent in attempt to receive a ping response.
    // In practice, the LPC always responds after the 1st ping.
    static const uint32_t PING_ATTEMPTS = 4;

    // Initialize the lower layer
    lpc_pkt_init(lpc_uart_write);

    // Perform the automatic baud rate process by sending a number 
    // of ping packets
    for (uint32_t i=0; i<PING_ATTEMPTS; i++)
    {
        lpc_send_ping();
        if (wait_for_pkt(PKT_TYPE_PING_RSP, 0))
        {
            NRF_LOG_DEBUG("ping rsp after %d attempts.", i+1);
            return 0;
        }
    }

    return -1;
}

int lpc_protocol_apply_fw(uint32_t file_sz)
{
    static uint8_t buf[512];
    const uint32_t* p_params;

    uint32_t max_packet;
    uint32_t offset = 0;
    int32_t bytes_to_send;
    int32_t bytes_rem = file_sz;

    // Fill  & Set Configuration
    RETURN_IF_NONZERO(lpc_send_fill_mem1()); 
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    NRF_LOG_INFO("Fill mem1 success .");

    RETURN_IF_NONZERO(lpc_send_fill_mem2()); 
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    NRF_LOG_INFO("Fill mem2 success .");
    
    RETURN_IF_NONZERO(lpc_send_config_mem());
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    NRF_LOG_INFO("Configure Memory success .");

    // Erase the LPC flash. This also unlocks the device.
    RETURN_IF_NONZERO(lpc_send_flash_erase_all());
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    NRF_LOG_INFO("Flash erase success .");

    // RETURN_IF_NONZERO(lpc_send_flash_erase_region1());
    // RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    // NRF_LOG_INFO("Flash erase 1 success .");

    // RETURN_IF_NONZERO(lpc_send_flash_erase_region2());
    // RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    // NRF_LOG_INFO("Flash erase 2 success .");

    // Read the max packet size
    RETURN_IF_NONZERO(lpc_send_get_property(PROP_MAX_PACKET_SIZE));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    // Second param is the size, in bytes.
    max_packet = MIN(p_params[1], sizeof(buf));

    // Start the write procedure. 
    // There is a response for this packet, and a response at the end 
    // of all the data packets.
    // See diagram and description in MCUBOOTRM.pdf, section 3.3.
    RETURN_IF_NONZERO(lpc_send_write_mem(bytes_rem));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));

    NRF_LOG_INFO("Start write procedure .");

    while (bytes_rem)
    {
        bytes_to_send = MIN(bytes_rem, max_packet);
        // Read a chunk of the FW 'file'
        if (lpc_fw_file_read(offset, buf, bytes_to_send))
        {
            // Read failed.
            return -1;
        }

        // Send the chunk
        RETURN_IF_NONZERO(lpc_send_data(buf, bytes_to_send));
        RETURN_IF_NULL(wait_for_pkt(PKT_TYPE_ACK, 0));

        offset += bytes_to_send;
        bytes_rem -= bytes_to_send;

        // Don't log on the last iteration.
        if (bytes_rem)
        {
            NRF_LOG_INFO("wrote %d / %d", offset, file_sz);
        }
    }

    // LPC responds at the end of the writes
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));
    NRF_LOG_INFO("all bytes written! %d / %d", offset, file_sz);

    // TODO: add isp reset pins here maybe?
    // Setup ISP GPIOs
    nrf_gpio_cfg_input(ISP0N_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_input(ISP1N_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(ISP2N_PIN, NRF_GPIO_PIN_PULLDOWN);

    // Reset the LPC to complete the process
    RETURN_IF_NONZERO(lpc_send_reset());
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GENERIC_RSP, 0, NULL));

    return 0;
}

int lpc_property_read(void)
{
    const uint32_t* p_params;

    RETURN_IF_NONZERO(lpc_send_get_property(PROP_CURRENT_VER));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    NRF_LOG_WARNING("PROP_CURRENT_VER=0x%08X (%d)", p_params[1], p_params[1]);

    RETURN_IF_NONZERO(lpc_send_get_property(PROP_MAX_PACKET_SIZE));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    NRF_LOG_WARNING("PROP_MAX_PACKET_SIZE=0x%08X (%d)", p_params[1], p_params[1]);

    RETURN_IF_NONZERO(lpc_send_get_property(PROP_RAM_SIZE_BYTES));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    NRF_LOG_WARNING("PROP_RAM_SIZE_BYTES=0x%08X (%d)", p_params[1], p_params[1]);

    RETURN_IF_NONZERO(lpc_send_get_property(PROP_FLASH_SECURITY_STATE));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    NRF_LOG_WARNING("PROP_FLASH_SECURITY_STATE=0x%08X (%d)", p_params[1], p_params[1]);

    RETURN_IF_NONZERO(lpc_send_get_property(PROP_AVAIL_COMMANDS));
    RETURN_IF_NONZERO(wait_for_pkt_rsp(PKT_CMDRSP_TAG_GET_PROPERTY_RSP, 0, &p_params));
    NRF_LOG_WARNING("PROP_AVAIL_COMMANDS=0x%08X (%d)", p_params[1], p_params[1]);

    return 0;
}
