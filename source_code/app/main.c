/*
 * main.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Elemind Morpheus app implementation.
 *
 */
/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not
 *    be reverse engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdint.h>
#include <string.h>

#include "app_timer.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "app_scheduler.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "ble_nus.h"
#include "boards.h"
#include "nrf.h"
#include "nrf_atflags.h"
#include "nrf_delay.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_strerror.h"
#include "sdk_common.h"
#include "nrf_delay.h"
#include "nrf_ringbuf.h"

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
#include "nrf_sdm.h"
#endif

// For buttonless DFU
#include "ble_dfu.h"
#include "nrf_power.h"
#include "nrf_bootloader_info.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

// Logging
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

// NRF SDK Services
#include "ble_bas.h"
#include "ble_dis.h"

// Morpheus
#include "ble_elemind.h"
#include "lpc_uart.h"
#include "version.h"
#include "binary_interface_inst.h"


/** A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_CONN_CFG_TAG 1

/** Name of device. Will be included in the advertising data. */
#define DEVICE_NAME "Elemind Morpheus"

/** Manufacturer. Will be passed to Device Information Service. */
#define MANUFACTURER_NAME "Elemind Technologies, Inc."

/** Model. Will be passed to Device Information Service. */
#if defined(BOARD_FF4)
#define MODEL_NUM "BLE v3.0"
#define HW_VERSION_STRING "4"
#elif defined(BOARD_FF3)
#define MODEL_NUM "FF3"
#define HW_VERSION_STRING "3"
#elif defined(BOARD_FF1)
#define MODEL_NUM "FF1"
#define HW_VERSION_STRING "3"
#elif defined(BOARD_PCA10040)
#define MODEL_NUM "PCA10040"
#define HW_VERSION_STRING ""
#else
#define MODEL_NUM "unknown"
#define HW_VERSION_STRING ""
#endif

/** UUID type for the Nordic UART Service (vendor specific). */
#define NUS_SERVICE_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN

/** Application's BLE observer priority.
    You shouldn't need to modify this value. */
#define APP_BLE_OBSERVER_PRIO 3

/** The fast advertising interval (in units of 0.625 ms). */
#define APP_ADV_FAST_INTERVAL MSEC_TO_UNITS(40, UNIT_0_625_MS)

/** The fast advertising duration in units of 10 milliseconds. */
#define APP_ADV_FAST_DURATION MSEC_TO_UNITS(180000, UNIT_10_MS)

/** The slow advertising interval (in units of 0.625 ms). */
#define APP_ADV_SLOW_INTERVAL MSEC_TO_UNITS(500, UNIT_0_625_MS)

/** The slow advertising duration in units of 10 milliseconds. */
#define APP_ADV_SLOW_DURATION BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED

/** Minimum acceptable connection interval (in units of 1.25ms). */
#define MIN_CONN_INTERVAL MSEC_TO_UNITS(7.5, UNIT_1_25_MS)

/** Maximum acceptable connection interval (in units of 1.25ms). */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(15, UNIT_1_25_MS)

/** Slave latency. */
#define SLAVE_LATENCY 30

/** Connection supervisory timeout (4 seconds).
    Supervision Timeout uses 10 ms units. */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)

/** Time from initiating event (connect or start of notification) to
    first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000)

/** Time between each call to sd_ble_gap_conn_param_update after the
    first call (30 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000)

/** Number of attempts before giving up the connection parameter negotiation. */
#define MAX_CONN_PARAMS_UPDATE_COUNT 3

/**< Scheduler queue entry size */
#define SCHED_MAX_EVENT_DATA_SIZE      APP_TIMER_SCHED_EVENT_DATA_SIZE
/**< Scheduler queue number of entries */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE               20
#else
#define SCHED_QUEUE_SIZE               20
#endif


/** Value used as error code on stack dump, can be used to identify
    stack location on stack unwind. */
#define DEAD_BEEF 0xDEADBEEF

/** UART TX buffer size. */
#define UART_TX_BUF_SIZE 512

/** UART RX buffer size. */
#define UART_RX_BUF_SIZE 512


/** BLE NUS service instance. */
BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);

/** Structure used to identify the battery service. */
BLE_BAS_DEF(m_bas);

/** Elemind Data Service instance. */
BLE_ELEMIND_DEF(m_elemind);

/** GATT module instance. */
NRF_BLE_GATT_DEF(m_gatt);

/** Context for the Queued Write module.*/
NRF_BLE_QWR_DEF(m_qwr);

/** Advertising module instance. */
BLE_ADVERTISING_DEF(m_advertising);

/** Delayed advertising start timer. */
APP_TIMER_DEF(m_delayed_advertising_timer_id);

NRF_RINGBUF_DEF(m_nus_ringbuf, 1024);

/*** Time to delay advertising start by (in milliseconds). */
#define ADVERTISING_DELAY_MS APP_TIMER_TICKS(200)


/** Handle of the current connection. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

/** Maximum length of data (in bytes) that can be transmitted to the
    peer by the Nordic UART service module. */
static uint16_t m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;

/** Universally unique service identifier. */
static ble_uuid_t m_adv_uuids[] =
  {
    /* UUID type inserted in services_init() from m_elemind
       (set in ble_elemind_init()) */
    {ELEMIND_DATA_SERVICE_UUID, BLE_UUID_TYPE_UNKNOWN}
  };

/** Atomic flag for use with UART */
static nrf_atflags_t g_uart_data_flag;

/** Flag to suspend UART reception */
static bool g_uart_pause = false;

/** Forward declaration */
void ble_nus_send_data_wrapper(uint8_t *buf, size_t buf_size);
static void uart_event_handle_sched(void* p_unused, uint16_t unused);

/** Assert macro callback.

    This function will be called in case of an assert in the SoftDevice.

    @warning This handler is an example only and does not fit a final
    product. You need to analyse how your product is supposed to react
    in case of Assert.

    @warning On assert from the SoftDevice, the system can only
    recover on reset.

    @param[in] line_num Line number of the failing ASSERT call.
    @param[in] p_file_name File name of the failing ASSERT call.
*/
void
assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

// Fatal error handler. Based on similar routine from app_error_weak.c
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) 
{
  __disable_irq();
  NRF_LOG_FINAL_FLUSH();

  switch (id)
  {
#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
    case NRF_FAULT_ID_SD_ASSERT:
      NRF_LOG_ERROR("SOFTDEVICE: ASSERTION FAILED");
      break;
    case NRF_FAULT_ID_APP_MEMACC:
      NRF_LOG_ERROR("SOFTDEVICE: INVALID MEMORY ACCESS");
      break;
#endif
    case NRF_FAULT_ID_SDK_ASSERT:
    {
      assert_info_t * p_info = (assert_info_t *)info;
      NRF_LOG_ERROR("ASSERTION FAILED at %s:%u",
              p_info->p_file_name,
              p_info->line_num);
      break;
    }
    case NRF_FAULT_ID_SDK_ERROR:
    {
      error_info_t * p_info = (error_info_t *)info;
      NRF_LOG_ERROR("ERROR %u [%s] at %s:%u\r\nPC at: 0x%08x",
              p_info->err_code,
              nrf_strerror_get(p_info->err_code),
              p_info->p_file_name,
              p_info->line_num,
              pc);
       NRF_LOG_ERROR("End of error report");
      break;
    }
    default:
      NRF_LOG_ERROR("UNKNOWN FAULT at 0x%08X", pc);
      break;
  }

  nrf_delay_ms(100);

  NRF_LOG_WARNING("System reset");
  NVIC_SystemReset();
}

/** Handle Queued Write Module errors.

  A pointer to this function will be passed to each service which
  may need to inform the application about an error.

    @param[in] nrf_error Error code containing information about what
    went wrong.
*/
static void
nrf_qwr_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void * p_context)
{
    if (state == NRF_SDH_EVT_STATE_DISABLED)
    {
        // Softdevice was disabled before going into reset. Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        //Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }
}

/* nrf_sdh state observer. */
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) =
{
    .handler = buttonless_dfu_sdh_state_observer,
};

static void advertising_config_get(ble_adv_modes_config_t * p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled  = true;
    p_config->ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_FAST_DURATION;
}

/**@brief Function for handling dfu events from the Buttonless Secure DFU service
 *
 * @param[in]   event   Event from the Buttonless Secure DFU service.
 */
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
        {
            NRF_LOG_INFO("Device is preparing to enter bootloader mode.");

            // Prevent device from advertising on disconnect.
            ble_adv_modes_config_t config;
            advertising_config_get(&config);
            config.ble_adv_on_disconnect_disabled = true;
            ble_advertising_modes_config_set(&m_advertising, &config);

            // Disconnect all other bonded devices that currently are connected.
            // This is required to receive a service changed indication
            // on bootup after a successful (or aborted) Device Firmware Update.
            (void)sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            break;
        }

        case BLE_DFU_EVT_BOOTLOADER_ENTER:
            NRF_LOG_INFO("Device will enter bootloader mode.");
            break;

        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
            NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
            break;

        case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
            NRF_LOG_ERROR("Request to send a response to client failed.");
            break;

        default:
            NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless: %d", event);
            break;
    }
}

/** Handle the data from the Nordic UART Service.

    This function will process the data received from the Nordic UART
    BLE Service and send it to the UART module.

    @param[in] p_evt Nordic UART Service event.
*/
static void
nus_data_handler(ble_nus_evt_t * p_evt)
{
  if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
      NRF_LOG_INFO("NUS data rx. len=%d", p_evt->params.rx_data.length);
      NRF_LOG_HEXDUMP_INFO(p_evt->params.rx_data.p_data,
        p_evt->params.rx_data.length);

      bin_itf_send_file_command(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
    }

}

/** Initialize the timer module.
 */
static void
timers_init(void)
{
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

/** GAP initialization.

    This function will set up all the necessary GAP (Generic Access
    Profile) parameters of the device. It also sets the permissions
    and appearance.
*/
static void
gap_params_init(void)
{
  uint32_t err_code;
  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  // Get BLE address
  ble_gap_addr_t ble_addr;
  sd_ble_gap_addr_get(&ble_addr);

  // Make Device Name from BLE address
  char buf_ptr[40];
  snprintf(buf_ptr, sizeof(buf_ptr), "%s %02X%02X%02X%02X%02X%02X",
  DEVICE_NAME, 
  ble_addr.addr[5], ble_addr.addr[4], ble_addr.addr[3],
  ble_addr.addr[2], ble_addr.addr[1], ble_addr.addr[0]);

  err_code = sd_ble_gap_device_name_set(&sec_mode,
    (const uint8_t *) buf_ptr,
    strlen(buf_ptr));
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
}

/** Initialize Device Information Service.

    Note that we initialize this service separately, after a delay,
    since it needs information from the LPC.
*/
void
static dis_init(void)
{
  uint32_t err_code;
  ble_dis_init_t dis_init;

  // Initialize Device Information Service.
  memset(&dis_init, 0, sizeof(dis_init));

  ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);
  ble_srv_ascii_to_utf8(&dis_init.model_num_str, (char *)MODEL_NUM);
  ble_srv_ascii_to_utf8(&dis_init.serial_num_str,
    ble_elemind_get_serial_number());
  ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, (char *)HW_VERSION_STRING);
  ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, (char *)FW_VERSION_STRING);
  ble_srv_ascii_to_utf8(&dis_init.sw_rev_str,
    ble_elemind_get_software_version());

  dis_init.dis_char_rd_sec = SEC_OPEN;

  NRF_LOG_DEBUG("ble_dis_init");
  err_code = ble_dis_init(&dis_init);
  NRF_LOG_DEBUG("ble_dis_init: %d", err_code);
  APP_ERROR_CHECK(err_code);
}

/** Initialize services that will be used by the application.

    Note that we don't initialize the Device Information Service here,
    since it needs information from the LPC. See dis_init().
*/
static void
services_init(void)
{
  uint32_t err_code;
  nrf_ble_qwr_init_t qwr_init = {0};
  ble_nus_init_t nus_init;
  ble_bas_init_t bas_init;
  ble_dfu_buttonless_init_t dfus_init = {0};
  ble_elemind_init_t elemind_init;

  // Initialize Queued Write Module.
  qwr_init.error_handler = nrf_qwr_error_handler;

  NRF_LOG_DEBUG("ble_qwr_init");
  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);

  // Initialize NUS.
  memset(&nus_init, 0, sizeof(nus_init));

  nus_init.data_handler = nus_data_handler;

  NRF_LOG_DEBUG("ble_nus_init");
  err_code = ble_nus_init(&m_nus, &nus_init);
  APP_ERROR_CHECK(err_code);
  nrf_ringbuf_init(&m_nus_ringbuf);

  // Initialize Battery Service.
  memset(&bas_init, 0, sizeof(bas_init));

  bas_init.evt_handler = NULL;
  bas_init.support_notification = true;
  bas_init.p_report_ref = NULL;
  bas_init.initial_batt_level = 100;

  // Here the sec level for the Battery Service can be changed/increased.
  bas_init.bl_rd_sec = SEC_OPEN;
  bas_init.bl_cccd_wr_sec = SEC_OPEN;
  bas_init.bl_report_rd_sec = SEC_OPEN;

  NRF_LOG_DEBUG("ble_bas_init");
  err_code = ble_bas_init(&m_bas, &bas_init);
  APP_ERROR_CHECK(err_code);

  err_code = ble_dfu_buttonless_async_svci_init();
  APP_ERROR_CHECK(err_code);

  // Initialize DFU update service
  dfus_init.evt_handler = ble_dfu_evt_handler;
  err_code = ble_dfu_buttonless_init(&dfus_init);
  APP_ERROR_CHECK(err_code);

  // Initialize Elemind Data Service init structure to zero.
  memset(&elemind_init, 0, sizeof(elemind_init));

  // Set initial characteristic values
  ble_elemind_init_defaults(&elemind_init);

  // Security level for Elemind Data Service
  /// @todo BH require no-MITM for this?
  elemind_init.security_level = SEC_OPEN;

  NRF_LOG_DEBUG("ble_elemind_init");
  err_code = ble_elemind_init(&m_elemind, &elemind_init, &m_advertising);
  APP_ERROR_CHECK(err_code);

  // Fixup advertised UUID type, now that ble_elemind_init() has registered it.
  m_adv_uuids[0].type = m_elemind.uuid_type;
}

/** Handle errors from the Connection Parameters module.

    @param[in] nrf_error Error code containing information about what
    went wrong.
*/
static void
conn_params_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

/** Initialize the Connection Parameters module.
 */
static void
conn_params_init(void)
{
  uint32_t err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail = false;
  cp_init.evt_handler = NULL;
  cp_init.error_handler = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

/** Put the chip into sleep mode.

    @note This function will not return.
*/
static void
sleep_mode_enter(void)
{
  /* Go to system-off mode.  This function will not return unless we
     are in debug mode (debugger or RTT connection); wakeup will cause
     a reset. */
  ret_code_t err_code = sd_power_system_off();
  NRF_LOG_ERROR("sd_power_system_off() returned (in debug mode)");
  APP_ERROR_CHECK(err_code);
}

/** Handle advertising events.

    This function will be called for advertising events which are
    passed to the application.

    @param[in] ble_adv_evt Advertising event.
*/
static void
on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
  switch (ble_adv_evt)
    {
      case BLE_ADV_EVT_FAST:
        NRF_LOG_DEBUG("Got BLE_ADV_EVT_FAST.");
        break;
      case BLE_ADV_EVT_SLOW:
        NRF_LOG_DEBUG("Got BLE_ADV_EVT_SLOW.");
        break;
      case BLE_ADV_EVT_IDLE:
        NRF_LOG_DEBUG("Got BLE_ADV_EVT_IDLE, going to sleep.");
        sleep_mode_enter();
        break;
      default:
        break;
    }
}

/** Handle BLE events.

    @param[in] p_ble_evt Bluetooth stack event.
    @param[in] p_context Unused.
*/
static void
ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
  uint32_t err_code;

  switch (p_ble_evt->header.evt_id)
    {
      case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected");
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        const ble_gap_conn_params_t *p_params = 
          &p_ble_evt->evt.gap_evt.params.connected.conn_params;

        NRF_LOG_INFO("Params set. interval: %u-%ums, sl: %u, to: %ums",
            (p_params->min_conn_interval * UNIT_1_25_MS)/1000,
            (p_params->max_conn_interval * UNIT_1_25_MS)/1000,
            p_params->slave_latency,
            (p_params->conn_sup_timeout * 10)
        );

        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected");
        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        break;

      case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
          NRF_LOG_INFO("PHY update request.");
          ble_gap_phys_t const phys =
            {
              .rx_phys = BLE_GAP_PHY_AUTO,
              .tx_phys = BLE_GAP_PHY_AUTO,
            };
          err_code = sd_ble_gap_phy_update(
            p_ble_evt->evt.gap_evt.conn_handle, &phys);
          APP_ERROR_CHECK(err_code);
        } break;

      case BLE_GAP_EVT_PHY_UPDATE:
        {
          ble_gap_evt_phy_update_t phy_update = p_ble_evt->evt.gap_evt.params.phy_update;
          if (phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
          {
            NRF_LOG_INFO("PHY Update: TX=%d, RX=%d", phy_update.tx_phy, phy_update.rx_phy);
          }
          else
          {
            NRF_LOG_WARNING("PHY Update procedure failed! (HCI Error Code: 0x%02X)",
              phy_update.status);
          }
        } break;

      case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        // Pairing not supported
        NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
        err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
          BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        // No system attributes have been stored.
        err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRF_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
          BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRF_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
          BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        // Try to send more data
        ble_nus_send_data_wrapper(NULL,0);
        // And check the UART for new bytes
        uart_event_handle_sched(NULL,0);
        break;

      case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {
          const ble_gap_conn_params_t *p_params = 
            &p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;

          NRF_LOG_INFO("Params update. interval: %u-%ums, sl: %u, to: %ums",
              (p_params->min_conn_interval * UNIT_1_25_MS)/1000,
              (p_params->max_conn_interval * UNIT_1_25_MS)/1000,
              p_params->slave_latency,
              (p_params->conn_sup_timeout * 10)
          );
        }
        break;

      default:
        // No implementation needed.
        break;
    }
}

/** Initialize SoftDevice.

    This function initializes the SoftDevice and the BLE event interrupt.
*/
static void
ble_stack_init(void)
{
  ret_code_t err_code;

  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);

  // Configure the BLE stack using the default settings.
  // Fetch the start address of the application RAM.
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);

  // Enable BLE stack.
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);

  // Register a handler for BLE events.
  NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO,
    ble_evt_handler, NULL);
}

/** Handle events from the GATT library. */
static void
gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
  if (m_conn_handle != p_evt->conn_handle) 
    {
      return;
    }

  switch (p_evt->evt_id)
    {
      case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        {
          m_ble_nus_max_data_len =
            p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
          NRF_LOG_INFO("MTU is set to %d", m_ble_nus_max_data_len);
          // NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
          //   p_gatt->att_mtu_desired_central,
            // p_gatt->att_mtu_desired_periph);
        }
        break;

      case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        #if defined(S112) 
        // DLE not supported on S112 (Morpheus ff1/ff2)
        #else
        NRF_LOG_INFO("Data len set to %d", p_evt->params.data_length);
        #endif
        break;
      
      default:
        break;
    }
}

/** Initialize the GATT library. */
static void
gatt_init(void)
{
  ret_code_t err_code;

  err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt,
    NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
  APP_ERROR_CHECK(err_code);
}

/** Handle received serial data from the LPC.

    We handle serial data for LPC command parsing separately from the
    NUS handler, since the NUS handler can only handle ~20 bytes at a
    time (depending on the MTU)--some command lines are significantly
    longer than that.

    Receives a single character from the app_uart module and appends it
    to a string. The string will be be processed as a command at the
    end of line.
*/
static void
lpc_data_handle(uint8_t data)
{
  // Process all data via the binary interface.
  bin_itf_handle_messages(data);
}

void ble_nus_send_data_wrapper(uint8_t *buf, size_t buf_size)
{
  uint32_t err_code;

  // Thresholds for pausing and unpausing the UART reads.
  // The pause functionality ensures the UART buffer is not drained if the 
  // outbound ring buffer does not have room.
  // These numbers are experimentally determined.
  static const uint32_t NUS_RINGBUF_PAUSE_THRESHOLD = 32;
  static const uint32_t NUS_RINGBUF_UNPAUSE_THRESHOLD = 64;

  if (buf && buf_size)
  {
    // The caller provided a valid buffer. 
    // As a simplification, always add it to the ringbuf, rather than trying 
    // to send it directly. This is an optimization and helps in the case 
    // where we need to stop sending bytes mid-stream.
    while (buf_size > 0)
    {
      size_t sz = buf_size;

      err_code = nrf_ringbuf_cpy_put(&m_nus_ringbuf, buf, &sz);
      if (err_code == NRF_SUCCESS)
      {
        buf += sz;
        buf_size -= sz;
        NRF_LOG_DEBUG("put %d bytes in ringbuf. %d remaining. %d free", 
          sz, buf_size, nrf_ringbuf_free_get(&m_nus_ringbuf));
      }
      else
      {
        NRF_LOG_ERROR("couldn't put %d bytes in ringbuf", buf_size);
        return;
      }
    }

    if (!g_uart_pause && nrf_ringbuf_free_get(&m_nus_ringbuf) < NUS_RINGBUF_PAUSE_THRESHOLD)
    {
      g_uart_pause = true;
      NRF_LOG_DEBUG("pause");
    }
  }

  while (1)
  {
    // Ensure we are asking for a size that we can actually send
    size_t sz = m_ble_nus_max_data_len;
    // Get data from the ringbuf
    err_code = nrf_ringbuf_get(&m_nus_ringbuf, &buf, &sz, false);
    if (NRF_SUCCESS == err_code)
    {
      if (sz == 0) 
      {
        NRF_LOG_DEBUG("ringbuf drained!");
        return;
      }
      else
      {
        NRF_LOG_DEBUG("got %d bytes from ringbuf", sz);
        // TODO: optimization:
        // If we've got less than BLE_NUS_MAX_DATA_LEN, and more data in the
        // ringbuf, read more bytes into a working buffer
      }
    }
    else
    {
      NRF_LOG_ERROR("failed to get bytes from ringbuf. code=0x%x", err_code);
      return;
    }

    // Attempt to send
    uint16_t curr_buf_size = sz;
    err_code = ble_nus_data_send(&m_nus, buf, &curr_buf_size, m_conn_handle);

    if (err_code == NRF_SUCCESS ||
        err_code == NRF_ERROR_INVALID_STATE || 
        err_code == NRF_ERROR_NOT_FOUND)
    {
      NRF_LOG_INFO("NUS send. code=0x%x, size=%d", err_code, sz);

      // Sanity check the size
      if (curr_buf_size != sz)
      {
        NRF_LOG_ERROR("sent less than expected. sent=%d, exp=%d", curr_buf_size, sz);
      }

      // We can free up the buffer now.
      // And carry on through the loop
      (void)nrf_ringbuf_free(&m_nus_ringbuf, sz);

      // Check if we can unpause the UART
      if (g_uart_pause && nrf_ringbuf_free_get(&m_nus_ringbuf) >= NUS_RINGBUF_UNPAUSE_THRESHOLD) 
      {
        NRF_LOG_DEBUG("unpause");
        g_uart_pause = false;
      }

      // Any time we are making forward progress, make sure the UART is also 
      // being checked for new bytes since it may have filled and have been 
      // paused.
      if (!nrf_atflags_fetch_set(&g_uart_data_flag, 0))
      {
        // Use the scheduler here to avoid recursion.
        app_sched_event_put(NULL, 0, uart_event_handle_sched);
      }
    }
    else if (err_code == NRF_ERROR_RESOURCES) 
    {
      // Finally filled the gatt queue, done all we can for this loop.
      // Note that the bytes are left in the ringbuf because we did not 
      // call nrf_ringbuf_free().
      nrf_ringbuf_get_undo(&m_nus_ringbuf);
      return;
    }
    else
    {
      APP_ERROR_CHECK(err_code);
    }
  }
}

/** Handle app_uart events.

    Receives a single character from the app_uart module and pass it
    to the Nordic UART Service (NUS) and Elemind LPC data handlers.
*/
static void
uart_event_handle_sched(void* p_unused, uint16_t unused)
{
  uint8_t data;

  // Clear the flag before starting any work.
  // Doing this here prevents a boundary case where a new byte arrives between 
  // the last app_uart_get() and clearing the flag.
  nrf_atflags_clear(&g_uart_data_flag, 0);

  if (g_uart_pause) {
    return;
  }

  uint32_t status = app_uart_get(&data);
  while (NRF_SUCCESS == status) {
    lpc_data_handle(data);

    if (g_uart_pause) {
      return;
    }

    status = app_uart_get(&data);
  }
}

static void
uart_event_handle(app_uart_evt_t * p_event)
{
  // Handler called in interrupt context
  switch (p_event->evt_type) {
    case APP_UART_COMMUNICATION_ERROR:
      APP_ERROR_HANDLER(p_event->data.error_communication);
      break;

    case APP_UART_FIFO_ERROR:
      APP_ERROR_HANDLER(p_event->data.error_code);
      break;

    case APP_UART_DATA_READY:
      if (nrf_atflags_fetch_set(&g_uart_data_flag, 0)) {
        // Flag was already set. Prevent filling the buffer on every byte.
        break;
      }
      // Defer event through scheduler
      app_sched_event_put(NULL, 0, uart_event_handle_sched);
      break;
    default:
      break;
  }
}

/** Initialize the UART module.
 */
static void
uart_init(void)
{
  uint32_t err_code;
  app_uart_comm_params_t const comm_params =
    {
      .rx_pin_no = RX_PIN_NUMBER,
      .tx_pin_no = TX_PIN_NUMBER,
      .rts_pin_no = RTS_PIN_NUMBER,
      .cts_pin_no = CTS_PIN_NUMBER,
      .flow_control = APP_UART_FLOW_CONTROL_ENABLED,
      .use_parity = false,
#if defined (UART_PRESENT)
      .baud_rate = NRF_UART_BAUDRATE_115200
#else
      .baud_rate = NRF_UARTE_BAUDRATE_115200
#endif
    };

  APP_UART_FIFO_INIT(&comm_params,
    UART_RX_BUF_SIZE,
    UART_TX_BUF_SIZE,
    uart_event_handle,
    APP_IRQ_PRIORITY_LOWEST,
    err_code);
  APP_ERROR_CHECK(err_code);
}

/** Initialize the Advertising functionality.
 */
static void
advertising_init(void)
{
  uint32_t err_code;
  ble_advertising_init_t init;

  memset(&init, 0, sizeof(init));

  init.advdata.uuids_complete.uuid_cnt =
    sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  init.advdata.uuids_complete.p_uuids = m_adv_uuids;
  init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

  init.srdata.name_type = BLE_ADVDATA_FULL_NAME;
  init.srdata.include_appearance = false;

  init.config.ble_adv_fast_enabled = true;
  init.config.ble_adv_fast_interval = APP_ADV_FAST_INTERVAL;
  init.config.ble_adv_fast_timeout = APP_ADV_FAST_DURATION;
  init.config.ble_adv_slow_enabled = true;
  init.config.ble_adv_slow_interval = APP_ADV_SLOW_INTERVAL;
  init.config.ble_adv_slow_timeout = APP_ADV_SLOW_DURATION;
  init.evt_handler = on_adv_evt;

  err_code = ble_advertising_init(&m_advertising, &init);
  APP_ERROR_CHECK(err_code);

  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/** Initialize the nrf log module.
 */
static void
log_init(void)
{
  ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/** Initialize power management.
 */
static void
power_management_init(void)
{
  ret_code_t err_code;
  err_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(err_code);
}

/** Handle the idle state (main loop).

    If there is no pending log operation, then sleep until next the
    next event occurs.
*/
static void
idle_state_handle(void)
{
  app_sched_execute();
  if (NRF_LOG_PROCESS() == false)
    {
      nrf_pwr_mgmt_run();
    }
}

/** Start advertising.
 */
static void
advertising_start(void)
{
  uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(err_code);
}

/** Handle delayed advertising timeout.

    Initialize Device Information Service (using data from LPC), and
    start advertising.
*/
static void
delayed_advertising_timeout_handler(void* p_context)
{
  dis_init();
  advertising_start();
}

/** Start delayed advertising timer.

    We wait to start advertising in order to give the LPC time to send
    us the serial number and software version, so that we can
    initialize the Device Information Service.
*/
static void
delayed_advertising_start(void)
{
  ret_code_t err_code;

  // Create delayed advertising timer
  err_code = app_timer_create(&m_delayed_advertising_timer_id,
    APP_TIMER_MODE_SINGLE_SHOT, delayed_advertising_timeout_handler);
  APP_ERROR_CHECK(err_code);

  // Start delayed advertising timer
  err_code = app_timer_start(m_delayed_advertising_timer_id,
    ADVERTISING_DELAY_MS, NULL);
  APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Event Scheduler.
 */
static void 
scheduler_init(void)
{
  APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/** Application main function.
 */
int
main(void)
{
  // Initialize.
  log_init();
  NRF_LOG_WARNING("boot. version=" FW_VERSION_GIT_STRING);
  NRF_LOG_INFO("log_init.");

  // Init the scheduler prior to any modules which may use it 
  // such as the BLE stack, timers, uart, etc
  scheduler_init();
  NRF_LOG_INFO("scheduler_init.");

  timers_init();
  NRF_LOG_INFO("timers_init.");

  power_management_init();
  NRF_LOG_INFO("power_management_init.");

  ble_stack_init();
  NRF_LOG_INFO("ble_stack_init.");

  gap_params_init();
  NRF_LOG_INFO("gap_params_init.");

  gatt_init();
  NRF_LOG_INFO("gatt_init.");

  services_init();
  NRF_LOG_INFO("services_init.");

  advertising_init();
  NRF_LOG_INFO("advertising_init.");

  conn_params_init();
  NRF_LOG_INFO("conn_params_init.");

  nrf_atflags_init(&g_uart_data_flag, 1, 1);
  NRF_LOG_INFO("atflags_init.");

  uart_init();
  NRF_LOG_INFO("uart_init.");

  // Init the binary interface prior to any uses in the LPC layer.
  bin_itf_init();
  NRF_LOG_INFO("bin_itf_init.");

  lpc_uart_init(&m_bas, &m_elemind);
  NRF_LOG_INFO("lpc_uart_init.");

  /* Wait to start advertising until the LPC has had time to send us
     serial number and software version. */
  delayed_advertising_start();
  NRF_LOG_INFO("delayed_advertising_start.");

  // Enter main loop.
  for (;;)
    {
      idle_state_handle();
    }
}

