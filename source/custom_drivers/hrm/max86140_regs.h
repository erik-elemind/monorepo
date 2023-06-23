/*
 * max86140_regs.h
 *
 *  Created on: Jun 11, 2023
 *      Author: tyler
 */

#ifndef HRM_MAX86140_REGS_H_
#define HRM_MAX86140_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX86140_REG_INT_STATUS_1                   0x00
#define MAX86140_REG_INT_STATUS_2                   0x01
#define MAX86140_REG_INT_ENABLE_1                   0x02
#define MAX86140_REG_INT_ENABLE_2                   0x03
#define MAX86140_REG_FIFO_WRITE_POINTER             0x04
#define MAX86140_REG_FIFO_READ_POINTER              0x05
#define MAX86140_REG_OVERFLOW_COUNTER               0x06
#define MAX86140_REG_FIFO_DATA_COUNTER              0x07
#define MAX86140_REG_FIFO_DATA_REGISTER             0x08
#define MAX86140_REG_FIFO_CONFIGURATION_1           0x09
#define MAX86140_REG_FIFO_CONFIGURATION_2           0x0A
#define MAX86140_REG_SYSTEM_CONTROL                 0x0D
#define MAX86140_REG_PPG_SYNC_CONTROL               0x10
#define MAX86140_REG_PPG_CONFIGURATION_1            0x11
#define MAX86140_REG_PPG_CONFIGURATION_2            0x12
#define MAX86140_REG_PPG_CONFIGURATION_3            0x13
#define MAX86140_REG_PROX_INTERRUPT_THRESHOLD       0x14
#define MAX86140_REG_PHOTO_DIODE_BIAS               0x15
#define MAX86140_REG_PICKET_FENCE                   0x16
#define MAX86140_REG_LED_SEQUENCE_REGISTER_1        0x20
#define MAX86140_REG_LED_SEQUENCE_REGISTER_2        0x21
#define MAX86140_REG_LED_SEQUENCE_REGISTER_3        0x22
#define MAX86140_REG_LED1_PA                        0x23
#define MAX86140_REG_LED2_PA                        0x24
#define MAX86140_REG_LED3_PA                        0x25
#define MAX86140_REG_LED4_PA                        0x26
#define MAX86140_REG_LED5_PA                        0x27
#define MAX86140_REG_LED6_PA                        0x28
#define MAX86140_REG_LED_PILOT_PA                   0x29
#define MAX86140_REG_LED_RANGE_1                    0x2A
#define MAX86140_REG_LED_RANGE_2                    0x2B
#define MAX86140_REG_S1_HI_RES_DAC1                 0x2C
#define MAX86140_REG_S2_HI_RES_DAC1                 0x2D
#define MAX86140_REG_S3_HI_RES_DAC1                 0x2E
#define MAX86140_REG_S4_HI_RES_DAC1                 0x2F
#define MAX86140_REG_S5_HI_RES_DAC1                 0x30
#define MAX86140_REG_S6_HI_RES_DAC1                 0x31
#define MAX86140_REG_S1_HI_RES_DAC2                 0x32
#define MAX86140_REG_S2_HI_RES_DAC2                 0x33
#define MAX86140_REG_S3_HI_RES_DAC2                 0x34
#define MAX86140_REG_S4_HI_RES_DAC2                 0x35
#define MAX86140_REG_S5_HI_RES_DAC2                 0x36
#define MAX86140_REG_S6_HI_RES_DAC2                 0x37
#define MAX86140_REG_DIE_TEMPERATURE_CONFIGURATION  0x40
#define MAX86140_REG_DIE_TEMPERATURE_INTEGER        0x41
#define MAX86140_REG_DIE_TEMPERATURE_FRACTION       0x42
#define MAX86140_REG_SHA_COMMAND                    0xF0
#define MAX86140_REG_SHA_CONFIGURATION              0xF1
#define MAX86140_REG_MEMORY_CONTROL                 0xF2
#define MAX86140_REG_MEMORY_INDEX                   0xF3
#define MAX86140_REG_MEMORY_DATA                    0xF4
#define MAX86140_REG_PART_ID                        0xFF

#define REG_SYSTEM_CONTROL_SHDN_BIT 1
#define REG_SYSTEM_CONTROL_RESET_BIT 0
#define REG_SYSTEM_CONTROL_LP_MODE_BIT 2

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif
#endif /* HRM_MAX86140_REGS_H_ */
