#include "als-dpdic17.h"
#include "als.h"
#include "i2c.h"
#include "config.h"
#include "loglevels.h"

static const char *TAG = "als";	// Logging prefix for this module

/* 1x, equivalent to ALS_DPDIC17_GAIN_x1 
 *
 * Used in lux calculation below.  Must match gain setting. */
static const int PGA_ALS = 1; 
static const int ALS_TIME = 255;
static const int WAIT_TIME = 1;
static const int CONV_TIME_MS = 5 * WAIT_TIME + (2.888 + 2.625 * (ALS_TIME + 1));

static int
als_read_reg(uint8_t reg_addr, uint8_t* data, uint8_t len) 
{
  status_t status = i2c_mem_read(
    &I2C4_RTOS_HANDLE,
    ALS_DPDIC17_ADDR,
    reg_addr, sizeof(reg_addr),
    data, len);
  return kStatus_Success == status ? 0 : -1;
}

static int
als_write_reg(uint8_t reg_addr, uint8_t data) 
{
  status_t status = i2c_mem_write(
    &I2C4_RTOS_HANDLE,
    ALS_DPDIC17_ADDR,
    reg_addr, sizeof(reg_addr),
    &data, 1);
  return kStatus_Success == status ? 0 : -1;
}

void als_init(void) {
  // TODO: add return code checks
  als_dpdic17_init(als_read_reg, als_write_reg);
  uint16_t als_product_id;
  als_dpdic17_get_product_id(&als_product_id);
  if(als_product_id != ALS_DPDIC17_PRODUCT_ID) {
    LOGE(TAG, "Invalid ALS product ID");
  }
  als_dpdic17_set_wait_time(WAIT_TIME);
  als_dpdic17_set_wait(true);
  als_dpdic17_set_als_time(ALS_TIME);
  als_start(); 
}

void als_start(void) {
  als_dpdic17_clear_int();
  als_dpdic17_enable_als(true);
}

/* Blocking wait.  For performance sensitive applications, do a
 * non-blocking wait for als_wait_time() ms instead before calling
 * als_get_lux()
 */
void als_wait(void) {
  vTaskDelay(pdMS_TO_TICKS(als_wait_time()));
}

void als_stop(void) {
  als_dpdic17_enable_als(false);
}

// in ms
int als_wait_time(void) {
  return CONV_TIME_MS;
}

// Calculate lux using algorithm and constants from page 16 of datasheet.
int als_get_lux(float* lux) {
  uint16_t data0;
  uint16_t data1;
  bool err;
  static const float K1 = .41f;
  static const float K2 = .57f;
  static const float K3 = 1.58f;
  float K;
  float d;

  if (ALS_DPDIC17_STATUS_SUCCESS != als_dpdic17_get_por(&err)) {
    LOGE(TAG, "ALS error calling get_por");
    return -1;
  }
  if (err) {
    LOGE(TAG, "ALS: POR error\n\r");
    return -1;
  }
  if (ALS_DPDIC17_STATUS_SUCCESS != als_dpdic17_get_error(&err)) {
    LOGE(TAG, "ALS error calling get_error");
    return -1;
  }
  if (err) {
    LOGE(TAG, "Error in ALS conversion");
    return -1;
  }
  if (ALS_DPDIC17_STATUS_SUCCESS != als_dpdic17_get_ch0(&data0)) {
    LOGE(TAG, "ALS error calling get_ch0");
    return -1;
  }
  if (ALS_DPDIC17_STATUS_SUCCESS != als_dpdic17_get_ch1(&data1)) {
    LOGE(TAG, "ALS error calling get_ch1");
    return -1;
  }

  d = (float)data0 / (float)data1;

  if (d < .42f) {
    K = K1;
  } else if (d > .61f) {
    K = K3;
  } else {
    K = K2;
  }
  *lux = ((float)data0 / PGA_ALS) * ((64.0f) / (ALS_TIME + 1)) * K;

  return 0;
}
