 
/**
 * @file    VCNL4020C.c
 */

#include "vcnl4020c.h"

#include <stdio.h>

#include "fsl_debug_console.h"
#include "fsl_i2c_freertos.h"
#include "board_config.h"

#include "vcnl4020c_regs.h"


#define VCNL_COMMAND_REG           0x80 // 0
#define VCNL_PRODUCT_ID_REG        0x81 // 1
#define VCNL_BIOSENSOR_CONFIG_REG  0x82 // 2
#define VCNL_LED_CURRENT_REG       0x83 // 3
#define VCNL_AMBIENT_CONFIG_REG    0x84 // 4
#define VCNL_AMBIENT_RESULT_REG    0x85 // 5 & 6
#define VCNL_BIOSENSOR_RESULT_REG  0x87 // 7 & 8
#define VCNL_INTERRUPT_CONFIG_REG  0x89 // 9
#define VCNL_LO_THRESH_REG         0x8A // 10 & 11
#define VCNL_HI_THRESH_REG         0x8C // 12 & 13
#define VCNL_INTERRUPT_STATUS_REG  0x8E // 14
#define VCNL_BIOSENSOR_TIMING_REG  0x8F // 15

/*
 * Write 8bit VAL to register address REG. Returns true if successful, false otherwise.
 */
static vcnl_status vcnl4020c_write_reg(vcnl4020c* vcnl, uint8_t reg, uint8_t val)
{
    i2c_master_transfer_t masterXfer;
    status_t status;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress   = vcnl->address;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = reg;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = &val;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    status = I2C_RTOS_Transfer(vcnl->master_rtos_handle, &masterXfer);
    if(status!=kStatus_Success) return VCNL_STATUS_FAIL;

    return VCNL_STATUS_SUCCESS;
}

/*
 * Read 8bit into VAL from register address REG. Returns true if successful, false otherwise.
 */
static vcnl_status vcnl4020c_read_reg(vcnl4020c* vcnl, uint8_t reg, uint8_t *val)
{
    i2c_master_transfer_t masterXfer;
    status_t status;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress   = vcnl->address;
    masterXfer.direction      = kI2C_Read;
    masterXfer.subaddress     = reg;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = val;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    status = I2C_RTOS_Transfer(vcnl->master_rtos_handle, &masterXfer);
    if(status!=kStatus_Success) return VCNL_STATUS_FAIL;

    return VCNL_STATUS_SUCCESS;
}

/*
 * Read unsigned 16bit into VAL from register address REG. Returns true if successful, false otherwise.
 */
static vcnl_status vcnl4020c_read_uint16_reg(vcnl4020c* vcnl, uint8_t reg, uint16_t *val)
{
    i2c_master_transfer_t masterXfer;
    status_t status;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress   = vcnl->address;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data           = &reg;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    status = I2C_RTOS_Transfer(vcnl->master_rtos_handle, &masterXfer);
    if(status!=kStatus_Success) return VCNL_STATUS_FAIL;

    uint8_t* uint8_val = (uint8_t*)val;
    masterXfer.direction      = kI2C_Read;
    masterXfer.data           = uint8_val;
    masterXfer.dataSize       = 1;

    status = I2C_RTOS_Transfer(vcnl->master_rtos_handle, &masterXfer);
    if(status!=kStatus_Success) return VCNL_STATUS_FAIL;

    masterXfer.direction      = kI2C_Read;
    masterXfer.data           = uint8_val+1;
    masterXfer.dataSize       = 1;

    status = I2C_RTOS_Transfer(vcnl->master_rtos_handle, &masterXfer);
    if(status!=kStatus_Success) return VCNL_STATUS_FAIL;

    return VCNL_STATUS_SUCCESS;
}


hrm_product_id_val vcnl4020c_get_product_id(vcnl4020c* vcnl)
{
	hrm_product_id_val reg;
	// read the register
	vcnl4020c_read_reg(vcnl, VCNL_PRODUCT_ID_REG, &(reg.init_all));
	return reg;
}


void vcnl4020c_init(vcnl4020c* vcnl, i2c_rtos_handle_t* master_rtos_handle,
		uint8_t red_port, uint8_t red_pin,
		uint8_t ir_port, uint8_t ir_pin,
		pint_pin_int_t pint_pin, pint_cb_t pint_callback, bool config_pint_interrupt)
{
	vcnl->master_rtos_handle = master_rtos_handle;
	vcnl->address = 0x13;
	vcnl->red_port = red_port;
	vcnl->red_pin = red_pin;
	vcnl->ir_port = ir_port;
	vcnl->ir_pin = ir_pin;

	/*
	 * The following lines are commented out, in response to a "bug"
	 * that is documented here (https://community.nxp.com/thread/504805).
	 *
	 * Summary: "GPIO_PortInit" resets any previous GPIO settings.
	 * The result is given the following two calls, which one might
	 * expect would init both GPIO port 0 and GPIO port 1,
	 * actually ONLY initializes GPIO port 1.
	 * Only the LAST call to "GPIO_PortInit" is preserved:
	 *   GPIO_PortInit(GPIO,0);
	 *   GPIO_PortInit(GPIO,1);
	 *
	 * The behavior of the "GPIO_PortInit" function changed from
	 * SDK version 2.1.1 to 2.1.2.
	 * (NXP has a standard SDK that is modified slightly for each processor).
	 * In SDK 2.1.2, "GPIO_PortInit" would reset all previous GPIO settings
	 * if the processor had that ability. As of this writing, the SDK used
	 * is 2.7.1, which inherits this reset.
	 *
	 * The other functionality of "GPIO_PortInit", is to enable the GPIO clocks.
	 * This must now be done separately.
	 */
    //GPIO_PortInit(GPIO, ir_port);
    GPIO_PinInit(GPIO, ir_port, ir_pin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});

    //GPIO_PortInit(GPIO, red_port);
    GPIO_PinInit(GPIO, red_port, red_pin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});

    GPIO_PinInit(GPIO, BOARD_INITPINS_HRM_INTn_PORT, BOARD_INITPINS_HRM_INTn_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput, 0});
}


void vcnl4020c_set_measure_once(vcnl4020c* vcnl, bool enable_ambient, bool enable_biosensor)
{
	hrm_command_val reg;
	reg.init_all = 0;
	// configure register value
  	reg.als_od = enable_ambient;
   	reg.bs_od = enable_biosensor;
    if(enable_ambient || enable_biosensor){
    	reg.selftimed_en = 0;
    }
    // write to the register
	vcnl4020c_write_reg(vcnl, VCNL_COMMAND_REG, reg.init_all);
}

void vcnl4020c_set_measure_periodic(vcnl4020c* vcnl, bool enable_ambient, bool enable_biosensor)
{
	hrm_command_val reg;
	reg.init_all = 0;
	// configure register value
  	reg.als_en = enable_ambient;
   	reg.bs_en = enable_biosensor;
    if(enable_ambient || enable_biosensor){
    	reg.selftimed_en = 1;
    }
    // write to the register
	vcnl4020c_write_reg(vcnl, VCNL_COMMAND_REG, reg.init_all);
}


/*
 * Set the sampling rate of the biosensor
 *
 * rate = a value ranging from 0 - 8, representing the measurements/second
 *        0 - 1.95 measurements/s (DEFAULT)
 *        1 - 3.90625 measurements/s
 *        2 - 7.8125 measurements/s
 *        3 - 16.625 measurements/s
 *        4 - 31.25 measurements/s
 *        5 - 62.5 measurements/s
 *        6 - 125 measurements/s
 *        7 - 250 measurements/s
 */
void vcnl4020c_set_biosensor_rate(vcnl4020c* vcnl, uint8_t rate)
{
	hrm_biosensor_config_val reg;
	reg.init_all = 0;
	// configure register value
	reg.rate = rate;
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_BIOSENSOR_CONFIG_REG, reg.init_all);
}

/*
 * Set the LED current.
 *
 * current - A value that ranges from 0 to 20.
 *           0 = 0mA, 10 = 100mA, 20 = 200mA.
 */
void vcnl4020c_set_led_current(vcnl4020c* vcnl, uint8_t current)
{
	hrm_led_current_val reg;
	reg.init_all = 0;
	// configure register value
	reg.led_current = current;
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_LED_CURRENT_REG, reg.init_all);
}

/*
 * Set configuration parameters for the ambient sensor
 *
 * enable_cont_conv    - true=enable, false=disable(DEFAULT)
 * ambient_sample_rate -
 *                       000 - 1 samples/s
 *                       001 - 2 samples/s = DEFAULT
 *                       010 - 3 samples/s
 *                       011 - 4 samples/s
 *                       100 - 5 samples/s
 *                       101 - 6 samples/s
 *                       110 - 8 samples/s
 *                       111 - 10 samples/s
 * enable_offset_comp - true=enable(DEFAULT), false=disable
 * samples_to_average - the number of single conversions
 *                      done during one measurement cycle
 *                      0 = 1 conv
 *                      1 = 2 conv
 *                      2 = 4 conv
 *                      3 = 8 conv
 *                      4 = 16 conv
 *                      5 = 32 conv
 *                      6 = 64 conv
 *                      7 = 128 conv
 */
void vcnl4020c_set_ambient_config(vcnl4020c* vcnl,
		bool enable_cont_conv,
		uint8_t ambient_sample_rate,
		bool enable_offset_comp,
		uint8_t samples_to_average)
{
	hrm_ambient_config_val reg;
	reg.init_all = 0;
	// configure register value
	reg.avg_func = samples_to_average;
	reg.auto_offset_comp = enable_offset_comp;
	reg.ambient_sample_rate = ambient_sample_rate;
	reg.enable_cont_conv = enable_cont_conv;
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_AMBIENT_CONFIG_REG, reg.init_all);
}

/*
 * Set when the interrupt line is triggered.
 *
 * on_biosensor_ready - enable interrupt on biosensor data ready
 * on_ambient_ready - enable interrupt on ambient data ready
 * enable_thres - enable threshold based interrupt
 * sel_thres - false = threshold on biosensor, true = threshold on ambient.
 * thres_count - number of consecutive measurements above/below
 *               threshold before triggering interrupt.
 *               0 - 1 count = DEFAULT
 *               1 - 2 count
 *               2 - 4 count
 *               3 - 8 count
 *               4 - 16 count
 *               5 - 32 count
 *               6 - 64 count
 *               7 - 128 count
 */
void vcnl4020c_set_interrupt(vcnl4020c* vcnl,
		bool on_biosensor_ready,
		bool on_ambient_ready,
		bool enable_thres,
		bool sel_thres,
		uint8_t thres_count)
{
	hrm_interrupt_config_val reg;
	reg.init_all = 0;
	// configure register value
	reg.int_thres_sel = sel_thres;
	reg.int_thres_en = enable_thres;
	reg.int_als_ready_en = on_ambient_ready;
	reg.int_bs_ready_en = on_biosensor_ready;
	reg.int_count_exceed = thres_count;
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_INTERRUPT_CONFIG_REG, reg.init_all);
}

/*
 * Set the biosensor modulation timing
 *
 * delay_time -
 * freq - set the biosensor test signal frequency.
 *        0 = 390.625 kHz (DEFAULT)
 *        1 = 781.25 kHz
 *        2 = 1.5625 MHz
 *        3 = 3.125 MHz
 * dead_time -
 */
void vcnl4020c_set_biosensor_timing(vcnl4020c* vcnl, uint8_t delay_time, uint8_t freq, uint8_t dead_time)
{
	hrm_biosensor_timing_val reg;
	reg.init_all = 0;
	// configure register value
	reg.dead_time = dead_time;
	reg.freq = freq;
	reg.delay_time = delay_time;
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_BIOSENSOR_TIMING_REG, reg.init_all);
}

hrm_interrupt_status_val vcnl4020c_get_interrupt_status(vcnl4020c* vcnl)
{
	hrm_interrupt_status_val reg;
	// read the register
	vcnl4020c_read_reg(vcnl, VCNL_INTERRUPT_STATUS_REG, &(reg.init_all));
	return reg;
}

void vcnl4020c_clr_interrupt_status(vcnl4020c* vcnl)
{
	// Note that each interrupt status bit is cleared by writing a '1' to it.
	// All interrupt sources are cleared at once to ensure no race conditions exist.
	// For example, the biosensor interrupt bit is set and just as we are issuing 
	// the write to clear it, the als interrupt bit becomes set. If we don't clear 
	// all interrupt sources, the interrupt pin wouldn't deassert and we could 
	// wait forever for the next event.
	static const hrm_interrupt_status_val reg = {
		.int_th_hi = 1,
		.int_th_lo = 1,
		.int_als_ready = 1,
		.int_bs_ready = 1,
	};
	// write to the register
	vcnl4020c_write_reg(vcnl, VCNL_INTERRUPT_STATUS_REG, reg.init_all);
}

/*
 * Get value from ambient sensor
 */
uint16_t vcnl4020c_get_ambient(vcnl4020c* vcnl)
{
	uint16_t val;
	vcnl4020c_read_uint16_reg(vcnl, VCNL_AMBIENT_RESULT_REG, &val);
	return val;
}

/*
 * Get interrupt low threshold
 */
uint16_t vcnl4020c_get_lo_thres(vcnl4020c* vcnl)
{
	uint16_t val;
	vcnl4020c_read_uint16_reg(vcnl, VCNL_LO_THRESH_REG, &val);
	return val;
}

/*
 * Set interrupt low threshold
 */
vcnl_status vcnl4020c_set_lo_thres(vcnl4020c* vcnl, uint16_t val)
{
	vcnl_status status = VCNL_STATUS_SUCCESS;

	status = vcnl4020c_write_reg(vcnl, VCNL_LO_THRESH_REG, 0xFF & (val>>8));
	if(status != VCNL_STATUS_SUCCESS) return VCNL_STATUS_FAIL;

	status = vcnl4020c_write_reg(vcnl, VCNL_LO_THRESH_REG+1, 0xFF & val);
	if(status != VCNL_STATUS_SUCCESS) return VCNL_STATUS_FAIL;

	return status;
}

/*
 * Get interrupt high threshold
 */
uint16_t vcnl4020c_get_hi_thres(vcnl4020c* vcnl)
{
	uint16_t val;
	vcnl4020c_read_uint16_reg(vcnl, VCNL_HI_THRESH_REG, &val);
	return val;
}

/*
 * Set interrupt high threshold
 */
vcnl_status vcnl4020c_set_hi_thres(vcnl4020c* vcnl, uint16_t val)
{
	vcnl_status status = VCNL_STATUS_SUCCESS;

	status = vcnl4020c_write_reg(vcnl, VCNL_HI_THRESH_REG, 0xFF & (val>>8));
	if(status != VCNL_STATUS_SUCCESS) return VCNL_STATUS_FAIL;

	status = vcnl4020c_write_reg(vcnl, VCNL_HI_THRESH_REG+1, 0xFF & val);
	if(status != VCNL_STATUS_SUCCESS) return VCNL_STATUS_FAIL;

	return status;
}

/*
 * Get value from biosensor
 */
uint16_t vcnl4020c_get_biosensor(vcnl4020c* vcnl)
{
	uint16_t val;
	vcnl4020c_read_uint16_reg(vcnl, VCNL_BIOSENSOR_RESULT_REG, &val);
	return val;
}

void vcnl4020c_set_led(vcnl4020c* vcnl, bool red, bool ir)
{
    // Mapping table for vcnl4020c eval board
	// red = IRE
	// ir = IRI
	// IRI(ir)  IRE(red)
	//  L        L       D3 (int IR)
	//  L        H       D2 (ext RED)
	//  H        L       D5 (ext GREEN)
	//  H        H       D4 (ext IR)
	if(red){
		// red
		GPIO_PinWrite(GPIO, vcnl->red_port, vcnl->red_pin, 1);
		GPIO_PinWrite(GPIO, vcnl->ir_port, vcnl->ir_pin, 0);
	}else if(ir){
		// green
		GPIO_PinWrite(GPIO, vcnl->red_port, vcnl->red_pin, 0);
		GPIO_PinWrite(GPIO, vcnl->ir_port, vcnl->ir_pin, 1);
	}
}




