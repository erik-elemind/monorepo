#ifndef LPC_COMMANDS_H
#define LPC_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// RAM and CPU Usage by Task:
void memfree(int argc, char **argv);

void i2c_sensor_scan(int argc, char **argv);
void i2c_batt_scan(int argc, char **argv);
void i2c_sensor_read_byte(int argc, char **argv);
void i2c_sensor_write_byte(int argc, char **argv);
void i2c_batt_read_byte(int argc, char **argv);
void i2c_batt_write_byte(int argc, char **argv);

void gpio_read(int argc, char **argv);
void gpio_write(int argc, char **argv);

// TODO: Port these to LPC for debugging
//void gpioq_command(int argc, char **argv);
//void spiq_command(int argc, char **argv);

void power_off_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif  // LPC_COMMANDS
