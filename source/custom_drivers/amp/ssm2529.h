/*
 * Modified:     Jan, 2023
 * Modified by:  Tyler Gage
 */

#ifndef SSM2529_H_
#define SSM2529_H_

#include "fsl_i2c_freertos.h"

/* SSM2529 I2C Address = 001101x0 */
/*                       00110100 = 0x34*/
#define SSM2529_I2C_ADDR  0x34

/* SSM2529 Registers Address */
#define SSM2529_PWR_CTRL                    0x00
#define SSM2529_SYS_CTRL                    0x01
#define SSM2529_SAI_FMT1                    0x02
#define SSM2529_SAI_FMT2                    0x03
#define SSM2529_Channel_mapping_control     0x04
#define SSM2529_VOL_BF_FDSP                 0x05
#define SSM2529_VOL_AF_FDSP                 0x06
#define SSM2529_Volume_and_mute_control     0x07
#define SSM2529_DPLL_CTRL                   0x08
#define SSM2529_APLL_CTRL1                  0x09
#define SSM2529_APLL_CTRL2                  0x0A
#define SSM2529_APLL_CTRL3                  0x0B
#define SSM2529_APLL_CTRL4                  0x0C
#define SSM2529_APLL_CTRL5                  0x0D
#define SSM2529_APLL_CTRL6                  0x0E
#define SSM2529_FAULT_CTRL1                 0x0F
#define SSM2529_FAULT_CTRL2                 0x10
#define SSM2529_DEEMP_CTRL                  0x14
#define SSM2529_HPF_CTRL                    0x15
#define SSM2529_EQ1_COEF0_HI                0x16
#define SSM2529_EQ1_COEF0_LO                0x17
#define SSM2529_EQ1_COEF1_HI                0x18
#define SSM2529_EQ1_COEF1_LO                0x19
#define SSM2529_EQ1_COEF2_HI                0x1A
#define SSM2529_EQ1_COEF2_LO                0x1B
#define SSM2529_EQ1_COEF3_HI                0x1C
#define SSM2529_EQ1_COEF3_LO                0x1D
#define SSM2529_EQ1_COEF4_HI                0x1E
#define SSM2529_EQ1_COEF4_LO                0x1F
#define SSM2529_EQ2_COEF0_HI                0x20
#define SSM2529_EQ2_COEF0_LO                0x21
#define SSM2529_EQ2_COEF1_HI                0x22
#define SSM2529_EQ2_COEF1_LO                0x23
#define SSM2529_EQ2_COEF2_HI                0x24
#define SSM2529_EQ2_COEF2_LO                0x25
#define SSM2529_EQ2_COEF3_HI                0x26
#define SSM2529_EQ2_COEF3_LO                0x27
#define SSM2529_EQ2_COEF4_HI                0x28
#define SSM2529_EQ2_COEF4_LO                0x29
#define SSM2529_EQ3_COEF0_HI                0x2A
#define SSM2529_EQ3_COEF0_LO                0x2B
#define SSM2529_EQ3_COEF1_HI                0x2C
#define SSM2529_EQ3_COEF1_LO                0x2D
#define SSM2529_EQ3_COEF2_HI                0x2E
#define SSM2529_EQ3_COEF2_LO                0x2F
#define SSM2529_EQ3_COEF3_HI                0x30
#define SSM2529_EQ3_COEF3_LO                0x31
#define SSM2529_EQ3_COEF4_HI                0x32
#define SSM2529_EQ3_COEF4_LO                0x33
#define SSM2529_EQ4_COEF0_HI                0x34
#define SSM2529_EQ4_COEF0_LO                0x35
#define SSM2529_EQ4_COEF1_HI                0x36
#define SSM2529_EQ4_COEF1_LO                0x37
#define SSM2529_EQ4_COEF2_HI                0x38
#define SSM2529_EQ4_COEF2_LO                0x39
#define SSM2529_EQ4_COEF3_HI                0x3A
#define SSM2529_EQ4_COEF3_LO                0x3B
#define SSM2529_EQ4_COEF4_HI                0x3C
#define SSM2529_EQ4_COEF4_LO                0x3D
#define SSM2529_EQ5_COEF0_HI                0x3E
#define SSM2529_EQ5_COEF0_LO                0x3F
#define SSM2529_EQ5_COEF1_HI                0x40
#define SSM2529_EQ5_COEF1_LO                0x41
#define SSM2529_EQ5_COEF2_HI                0x42
#define SSM2529_EQ5_COEF2_LO                0x43
#define SSM2529_EQ5_COEF3_HI                0x44
#define SSM2529_EQ5_COEF3_LO                0x45
#define SSM2529_EQ5_COEF4_HI                0x46
#define SSM2529_EQ5_COEF4_LO                0x47
#define SSM2529_EQ6_COEF0_HI                0x48
#define SSM2529_EQ6_COEF0_LO                0x49
#define SSM2529_EQ6_COEF1_HI                0x4A
#define SSM2529_EQ6_COEF1_LO                0x4B
#define SSM2529_EQ6_COEF2_HI                0x4C
#define SSM2529_EQ6_COEF2_LO                0x4D
#define SSM2529_EQ7_COEF0_HI                0x4E
#define SSM2529_EQ7_COEF0_LO                0x4F
#define SSM2529_EQ7_COEF1_HI                0x50
#define SSM2529_EQ7_COEF1_LO                0x51
#define SSM2529_EQ7_COEF2_HI                0x52
#define SSM2529_EQ7_COEF2_LO                0x53
#define SSM2529_EQ_CTRL1                    0x54
#define SSM2529_EQ_CTRL2                    0x55
#define SSM2529_DRC_CTRL1                   0x56
#define SSM2529_DRC_CTRL2                   0x57
#define SSM2529_DRC_CTRL3                   0x58
#define SSM2529_DRC_CURVE1                  0x59
#define SSM2529_DRC_CURVE2                  0x5A
#define SSM2529_DRC_CURVE3                  0x5B
#define SSM2529_DRC_CURVE4                  0x5C
#define SSM2529_DRC_CURVE5                  0x5D
#define SSM2529_DRC_HOLD_TIME               0x5E
#define SSM2529_DRC_RIPPLE_CTRL             0x5F
#define SSM2529_DRC_mode_control            0x60
#define SSM2529_FDSP_EN                     0x61
#define SSM2529_SPK_PROT_EN                 0x80
#define SSM2529_TEMP_AMBIENT                0x81
#define SSM2529_SPKR_DCR                    0x82
#define SSM2529_SPKR_TC                     0x83
#define SSM2529_SP_CF1_H                    0x84
#define SSM2529_SP_CF1_L                    0x85
#define SSM2529_SP_CF2_H                    0x86
#define SSM2529_SP_CF2_L                    0x87
#define SSM2529_SP_CF3_H                    0x88
#define SSM2529_SP_CF3_L                    0x89
#define SSM2529_SP_CF4_H                    0x8A
#define SSM2529_SP_CF4_L                    0x8B
#define SSM2529_SPKR_TEMP                   0x8C
#define SSM2529_SPKR_TEMP_MAG               0x8D
#define SSM2529_MAX_SPKR_TEMP               0x8E
#define SSM2529_SPK_GAIN                    0x8F
#define SSM2529_SOFT_RST                    0xFF

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

void SSM2529_WriteReg(char regAddr, unsigned char data);
unsigned char SSM2529_ReadReg(char regAddr);
void SSM2529_Init(i2c_rtos_handle_t *i2c_handle);
void SSM2529_SetVolume(uint8_t volume);
void SSM2529_Mute(bool mute);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif /* SSM2529_H_ */
