#ifndef ALS_DPDIC17
#define ALS_DPDIC17

#include <stdint.h>
#include <stdbool.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

#define ALS_DPDIC17_ADDR (0x38)

#define ALS_DPDIC17_PRODUCT_ID 0x1614

// Routines implemented by the caller which perform read and write operations
// using the system-specific APIs.
// These are blocking functions and return 0 for success and non-0 for failure.
typedef int (*als_dpdic17_reg_read_func_t)(uint8_t reg_addr, uint8_t* data, uint8_t len);
typedef int (*als_dpdic17_reg_write_func_t)(uint8_t reg_addr, uint8_t data);

typedef enum {
  ALS_DPDIC17_GAIN_x1 = 0,
  ALS_DPDIC17_GAIN_x4 = 1,
  ALS_DPDIC17_GAIN_x8 = 2,
  ALS_DPDIC17_GAIN_x32 = 3,
  ALS_DPDIC17_GAIN_x96 = 4,
} als_dpdic17_gain_t;

typedef enum {
  ALS_DPDIC17_INT_SOURCE_CH0 = 0,
  ALS_DPDIC17_INT_SOURCE_CH1 = 1,
} als_dpdic17_int_source_t;

  typedef enum {
  ALS_DPDIC17_STATUS_SUCCESS = 0,
  ALS_DPDIC17_STATUS_ERROR,
  ALS_DPDIC17_STATUS_ERROR_NOT_INIT,
} als_dpdic17_status_t;

als_dpdic17_status_t als_dpdic17_init(als_dpdic17_reg_read_func_t read_func, als_dpdic17_reg_write_func_t write_func);
als_dpdic17_status_t als_dpdic17_softreset(void);
als_dpdic17_status_t als_dpdic17_set_wait(bool wait);
als_dpdic17_status_t als_dpdic17_enable_als(bool enable);
als_dpdic17_status_t als_dpdic17_get_data_valid(bool* valid);
als_dpdic17_status_t als_dpdic17_get_por(bool* por);
als_dpdic17_status_t als_dpdic17_clear_int(void);
als_dpdic17_status_t als_dpdic17_set_wait_time(uint8_t wait_time /* in 5ms increments */);
als_dpdic17_status_t als_dpdic17_set_gain(als_dpdic17_gain_t gain);
// TALS=2.888 + 2.625 x (time + 1) (ms)
als_dpdic17_status_t als_dpdic17_set_als_time(uint8_t time);
als_dpdic17_status_t als_dpdic17_set_persistence(uint8_t persistence);
als_dpdic17_status_t als_dpdic17_set_thres_l(uint16_t thres);
als_dpdic17_status_t als_dpdic17_set_thres_h(uint16_t thres);
als_dpdic17_status_t als_dpdic17_set_int_source(als_dpdic17_int_source_t source);
als_dpdic17_status_t als_dpdic17_get_error(bool* error);
als_dpdic17_status_t als_dpdic17_get_ch0(uint16_t* data);
als_dpdic17_status_t als_dpdic17_get_ch1(uint16_t* data);
als_dpdic17_status_t als_dpdic17_get_product_id(uint16_t* data);
  
// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // ALS_DPDIC17

