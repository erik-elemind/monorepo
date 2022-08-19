/* als-dpdic17 driver */

#include "als-dpdic17.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#ifndef NULL
#define NULL ((void*)0)
#endif

#define REG_ADDR_SYSM_CTRL 0x00
#define REG_ADDR_INT_CTRL 0x01
#define REG_ADDR_INT_FLAG 0x02
#define REG_ADDR_WAIT_TIME 0x03
#define REG_ADDR_ALS_GAIN 0x04
#define REG_ADDR_ALS_TIME 0x05
#define REG_ADDR_PERSISTENCE 0x11
#define REG_ADDR_ALS_THRES_LL 0x0C
#define REG_ADDR_ALS_THRES_LH 0x0D
#define REG_ADDR_ALS_THRES_HL 0x0E
#define REG_ADDR_ALS_THRES_HH 0x0F
#define REG_ADDR_INT_SOURCE 0x16
#define REG_ADDR_ERROR_FLAG 0x17
#define REG_ADDR_CH0_DATA_L 0x1C
#define REG_ADDR_CH0_DATA_H 0x1D
#define REG_ADDR_CH1_DATA_L 0x1E
#define REG_ADDR_CH1_DATA_H 0x1F
#define REG_ADDR_PNO_LB 0xBC
#define REG_ADDR_PNO_HB 0xBD

#define REG_DATA_SOFT_RESET 0x80
#define REG_DATA_EN_WAIT 0x40
#define REG_DATA_EN_ALS 0x01

#define REG_DATA_DATA_FLAG 0x40
#define REG_DATA_INT_ALS 0x00
#define REG_DATA_INT_POR 0x80

typedef struct {
  als_dpdic17_reg_read_func_t read_func;
  als_dpdic17_reg_write_func_t write_func;
  uint8_t SYSM_CTRL;
} als_dpdic17_context;

static als_dpdic17_context g_context;

als_dpdic17_status_t als_dpdic17_init(als_dpdic17_reg_read_func_t read_func, als_dpdic17_reg_write_func_t write_func) {
  g_context.read_func = read_func;
  g_context.write_func = write_func;
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}


als_dpdic17_status_t als_dpdic17_softreset(void) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_SYSM_CTRL, REG_DATA_SOFT_RESET)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_set_wait(bool wait) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  if (wait) {
    g_context.SYSM_CTRL |= REG_DATA_EN_WAIT;
  } else {
    g_context.SYSM_CTRL &= ~REG_DATA_EN_WAIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_SYSM_CTRL, g_context.SYSM_CTRL)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_enable_als(bool enable) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  if (enable) {
    g_context.SYSM_CTRL |= REG_DATA_EN_ALS;
  } else {
    g_context.SYSM_CTRL &= ~REG_DATA_EN_ALS;
  }
  
  if (0 == g_context.write_func(REG_ADDR_SYSM_CTRL, g_context.SYSM_CTRL)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_get_data_valid(bool* valid) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  uint8_t data;
  if (0 == g_context.read_func(REG_ADDR_INT_CTRL, &data, 1)) {
    *valid = !(data & REG_DATA_DATA_FLAG);
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_get_por(bool* por) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  uint8_t data;
  if (0 == g_context.read_func(REG_ADDR_INT_FLAG, &data, 1)) {
    *por = (data & REG_DATA_INT_POR) != 0;
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_clear_int(void) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_INT_FLAG, REG_DATA_INT_ALS)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_set_wait_time(uint8_t wait_time /* in 5ms increments */) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_WAIT_TIME, wait_time)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_set_gain(als_dpdic17_gain_t gain) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_ALS_GAIN, gain)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

// Time is 1024 x (time + 1) – 1
als_dpdic17_status_t als_dpdic17_set_als_time(uint8_t time) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_ALS_TIME, time)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_set_persistence(uint8_t persistence) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  if (persistence >= 16) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  
  if (0 == g_context.write_func(REG_ADDR_PERSISTENCE, persistence | 0x10 )) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_set_thres_l(uint16_t thres) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  if (0 != g_context.write_func(REG_ADDR_ALS_THRES_LL, thres & 0xFF )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  if (0 != g_context.write_func(REG_ADDR_ALS_THRES_LH, thres >> 8 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

als_dpdic17_status_t als_dpdic17_set_thres_h(uint16_t thres) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  if (0 != g_context.write_func(REG_ADDR_ALS_THRES_HL, thres & 0xFF )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  if (0 != g_context.write_func(REG_ADDR_ALS_THRES_HH, thres >> 8 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

als_dpdic17_status_t als_dpdic17_set_int_source(als_dpdic17_int_source_t source) {
  if (NULL == g_context.write_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }
  
  if (0 == g_context.write_func(REG_ADDR_INT_SOURCE, source)) {
    return ALS_DPDIC17_STATUS_SUCCESS;
  }
  
  return ALS_DPDIC17_STATUS_ERROR;
}

als_dpdic17_status_t als_dpdic17_get_error(bool* error) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  // Must read low byte first, hardware latches high byte until read.
  uint8_t d;
  if (0 != g_context.read_func(REG_ADDR_ERROR_FLAG, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *error = d != 0;
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

als_dpdic17_status_t als_dpdic17_get_ch0(uint16_t* data) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  // Must read low byte first, hardware latches high byte until read.
  uint8_t d;
  if (0 != g_context.read_func(REG_ADDR_CH0_DATA_L, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data = d;
  if (0 != g_context.read_func(REG_ADDR_CH0_DATA_H, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data |= (uint16_t)d << 8;
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

als_dpdic17_status_t als_dpdic17_get_ch1(uint16_t* data) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  // Must read low byte first, hardware latches high byte until read.
  uint8_t d;
  if (0 != g_context.read_func(REG_ADDR_CH1_DATA_L, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data = d;
  if (0 != g_context.read_func(REG_ADDR_CH1_DATA_H, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data |= (uint16_t)d << 8;
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

als_dpdic17_status_t als_dpdic17_get_product_id(uint16_t* data) {
  if (NULL == g_context.read_func) {
    return ALS_DPDIC17_STATUS_ERROR_NOT_INIT;
  }

  // Must read low byte first, hardware latches high byte until read.
  uint8_t d;
  if (0 != g_context.read_func(REG_ADDR_PNO_LB, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data = d;
  if (0 != g_context.read_func(REG_ADDR_PNO_HB, &d, 1 )) {
    return ALS_DPDIC17_STATUS_ERROR;
  }
  *data |= (uint16_t)d << 8;
  
  return ALS_DPDIC17_STATUS_SUCCESS;
}

#pragma GCC diagnostic pop
