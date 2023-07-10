
#include "als.h"
#include "als-veml7700.h"
#include "i2c.h"
#include "config.h"
#include "loglevels.h"

static const char *TAG = "als";	// Logging prefix for this module

static int
als_read_reg(uint8_t reg_addr, uint16_t* data)
{
  uint8_t read_data[2] = {0};

  status_t status = i2c_mem_read(
      &ALS_I2C_RTOS_HANDLE,
      ALS_VEML7700_ADDR,
      reg_addr, sizeof(reg_addr),
	  read_data, sizeof(read_data));

  if(status == kStatus_Success)
  {
	  *data = ((uint16_t)read_data[1] << 8) | read_data[0];
	  return kStatus_Success;
  }

  return -1;
}

static int
als_write_reg(uint8_t reg_addr, uint16_t* data)
{
	// Convert to bytes for low level driver interface
	uint8_t write_data[2] = {0};
	write_data[0] = (uint8_t)(*data & 0xFF);
	write_data[1] = (uint8_t)(*data >> 8);

	status_t status = i2c_mem_write(
			&ALS_I2C_RTOS_HANDLE,
			ALS_VEML7700_ADDR,
			reg_addr, sizeof(reg_addr),
			write_data, sizeof(write_data));
  return kStatus_Success == status ? 0 : -1;
}

void als_init(void) {
	if(als_veml7700_init(als_read_reg, als_write_reg) != ALS_VEML7700_STATUS_SUCCESS)
	{
		LOGE(TAG, "Error initializing ALS");
	}
}

void als_start(void) {
	if(als_veml7700_power_on() != ALS_VEML7700_STATUS_SUCCESS)
	{
		LOGE(TAG, "Error starting ALS");
	}
}

void als_stop(void) {
	if(als_veml7700_power_off() != ALS_VEML7700_STATUS_SUCCESS)
	{
		LOGE(TAG, "Error stopping ALS");
	}
}

int als_get_lux(float* lux) {
	if(als_veml7700_read_lux(lux) != ALS_VEML7700_STATUS_SUCCESS)
	{
		LOGE(TAG, "Error reading lux off ALS");
		return -1;
	}

	return 0;
}
