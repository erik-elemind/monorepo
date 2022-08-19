#include <string.h>

#include "FreeRTOS.h"
#include "timers.h"
#include "lpc_commands.h"
#include "loglevels.h"
#include "config.h"
#include "board_config.h"
#include "i2c.h"
#include "ble.h"
#include "command_helpers.h"
#include "system_monitor.h"

// From the MCUXpresso-generated .ld file
extern unsigned int *_HeapSize;

// Copied from mcuxpresso/amazon-freertos/freertos_kernel/portable/MemMang/heap_useNewlib.c
#if defined(__MCUXPRESSO)
  #define configLINKER_HEAP_BASE_SYMBOL _pvHeapStart
  #define configLINKER_HEAP_LIMIT_SYMBOL _pvHeapLimit
  #define configLINKER_HEAP_SIZE_SYMBOL _HeapSize
#elif defined(__GNUC__)
  #define configLINKER_HEAP_BASE_SYMBOL end
  #define configLINKER_HEAP_LIMIT_SYMBOL __HeapLimit
  #define configLINKER_HEAP_SIZE_SYMBOL HEAP_SIZE
#endif

#define GPIO_PORT_MAX 1
#define GPIO_PIN_MAX 31


void memfree(int argc, char **argv) {
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  TaskHandle_t task_handle;
  TaskStatus_t task_status;
  int stack_size_bytes;
  int stack_used_bytes;
  int stack_free_bytes;
  int stack_used_percent;

  UBaseType_t task_count;
  TaskStatus_t *task_status_array;
  unsigned long total_run_time;
  unsigned long run_time_percent;

  task_count = uxTaskGetNumberOfTasks();
  // Reminder: malloc() must be free()'d below:
  task_status_array = malloc(sizeof(TaskStatus_t) * task_count);

  // Get the current Stack and CPU usage of every task:
  uxTaskGetSystemState(task_status_array, task_count, &total_run_time);

  printf("\r\n CPU & Memory Usage:\r\n---------------------\r\n\r\n");

  int index;
  for(index = 0 ; index < task_count ; index++) {

    task_status = task_status_array[index];
    task_handle = task_status.xHandle;

    // ThreadLocalStoragePointer index 0 is set to the thread's STACK_SIZE in ../main.c
    stack_size_bytes = (unsigned int)pvTaskGetThreadLocalStoragePointer( task_handle, 0 ) * sizeof(UBaseType_t);

    // A couple of threads are auto-created by FreeRTOS. We get their stack
    // sizes manually:
    if (strncmp("Tmr Svc", task_status.pcTaskName, strlen("Tmr Svc")) == 0) {
      stack_size_bytes = configTIMER_TASK_STACK_DEPTH * sizeof(UBaseType_t);
    }
    if (strncmp("IDLE", task_status.pcTaskName, strlen("IDLE")) == 0) {
      stack_size_bytes = configMINIMAL_STACK_SIZE * sizeof(UBaseType_t);
    }

    stack_free_bytes = uxTaskGetStackHighWaterMark(task_handle) * sizeof(UBaseType_t);
    stack_used_bytes = stack_size_bytes - stack_free_bytes;
    stack_used_percent = (100 * stack_used_bytes) / stack_size_bytes;

    run_time_percent = (100 * task_status.ulRunTimeCounter) / total_run_time;
    if (run_time_percent == 0) {
      printf("%14s (<1%% CPU):", task_status.pcTaskName);
    } else {
      printf("%14s (%2ld%% CPU):", task_status.pcTaskName, run_time_percent);
    }

    printf("%6d / %4d bytes used (%2d%%, %4d bytes free)\r\n",
      stack_used_bytes, stack_size_bytes, stack_used_percent, stack_free_bytes);
  }

  printf("\r\n FreeRTOS Heap for malloc(): %u bytes free\r\n", xPortGetFreeHeapSize());

  int heap_bytes_remaining = (int)&configLINKER_HEAP_SIZE_SYMBOL;
  // that's (&__HeapLimit)-(&__HeapBase)
  // On GCC it's: (unsigned long)&_estack - (unsigned long)&_end
  printf(" RAM reserved for main() and ISR stacks: %u bytes\r\n", heap_bytes_remaining);

  free(task_status_array);
  return;
}


void i2c_scan(i2c_rtos_handle_t *handle) {

  // Print header
  printf("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");

  // Valid 7-bit I2C Slave addresses are 0x07 to 0x77:
  uint8_t index = 0;
  while (index <= I2C_ADDRESS_MAX) {

    if ((index & 0x0F) == 0x00) { // New row
      printf("%02X: ", index & 0xF0);
    }

    // I2C Address values 0x00 to 0x07 are reserved, don't probe them:
    if (index <= I2C_ADDRESS_MIN) {
      printf("   ");
      index++;
      continue;
    }

    // Check for device
    status_t status = i2c_is_device_ready(handle, index);
    if (status == kStatus_Success)
    {
      printf(" %02X", index);
    } else {
      printf(" --");
    }

    index++;

    if ((index & 0x0F) == 0x00) {
      printf("\n");
    }
  }

  printf("\n"); // End of row
}

void i2c4_scan(int argc, char **argv)
{
  i2c_scan(&I2C4_RTOS_HANDLE);
}

void i2c5_scan(int argc, char **argv)
{
  i2c_scan(&I2C5_RTOS_HANDLE);
}

static void i2c_read_byte(i2c_rtos_handle_t *handle, int argc, char **argv)
{
  if (argc != 3) {
    printf("Error: Missing device and/or register number\n");
    printf("Usage: %s <addr> <reg>\n", argv[0]);
    return;
  }

  // Get address
  uint8_t address;
  if (!parse_uint8_arg_min_max(argv[0], argv[1],
      I2C_ADDRESS_MIN, I2C_ADDRESS_MAX, &address)) {
    return;
  }

  // Get register
  uint8_t reg;
  if (!parse_uint8_arg(argv[0], argv[2], &reg)) {
    return;
  }

  // Read data
  uint8_t data;
  status_t status = i2c_mem_read_byte(handle, address, reg, &data);
  if (status == kStatus_Success) {
    printf("Data: %d (0x%x)\n", data, data);
  }
  else {
    printf("Error: I2C read returned %ld (0x%lx)\n", status, status);
  }
}

static void i2c_write_byte(i2c_rtos_handle_t *handle, int argc, char **argv)
{
  if (argc != 4) {
    printf("Error: Missing device and/or register number\n");
    printf("Usage: %s <addr> <reg> <value>\n", argv[0]);
    return;
  }

  // Get address
  uint8_t address;
  if (!parse_uint8_arg_min_max(argv[0], argv[1],
      I2C_ADDRESS_MIN, I2C_ADDRESS_MAX, &address)) {
    return;
  }

  // Get register
  uint8_t reg;
  if (!parse_uint8_arg(argv[0], argv[2], &reg)) {
    return;
  }

  // Get value
  uint8_t value;
  if (!parse_uint8_arg(argv[0], argv[3], &value)) {
    return;
  }

  // Write data
  status_t status = i2c_mem_write_byte(handle,
    address, reg, value);
  if (status != kStatus_Success) {
    printf("Error: I2C write returned %ld (0x%lx)\n", status, status);
  }
}

void i2c4_read_byte(int argc, char **argv)
{
  i2c_read_byte(&I2C4_RTOS_HANDLE, argc, argv);
}

void i2c4_write_byte(int argc, char **argv)
{
  i2c_write_byte(&I2C4_RTOS_HANDLE, argc, argv);
}

void i2c5_read_byte(int argc, char **argv)
{
  i2c_read_byte(&I2C5_RTOS_HANDLE, argc, argv);
}

void i2c5_write_byte(int argc, char **argv)
{
  i2c_write_byte(&I2C5_RTOS_HANDLE, argc, argv);
}




void gpio_read(int argc, char **argv)
{
  if (argc != 3) {
    printf("Error: Missing port and/or pin\n");
    printf("Usage: %s <port> <pin>\n", argv[0]);
    return;
  }

  // Get port
  uint8_t port;
  if (!parse_uint8_arg_max(argv[0], argv[1], GPIO_PORT_MAX, &port)) {
    return;
  }

  // Get pin
  uint8_t pin;
  if (!parse_uint8_arg_max(argv[0], argv[2], GPIO_PIN_MAX, &pin)) {
    return;
  }

  // Read pin
  uint32_t value = GPIO_PinRead(GPIO, port, pin);

  printf("PIO%d_%d: %lu", port, pin, value);
}

void gpio_write(int argc, char **argv)
{
  if (argc != 4) {
    printf("Error: Missing port, pin and/or value\n");
    printf("Usage: %s <port> <pin> <value>\n", argv[0]);
    return;
  }

  // Get port
  uint8_t port;
  if (!parse_uint8_arg_max(argv[0], argv[1], GPIO_PORT_MAX, &port)) {
    return;
  }

  // Get pin
  uint8_t pin;
  if (!parse_uint8_arg_max(argv[0], argv[2], GPIO_PIN_MAX, &pin)) {
    return;
  }

  // Get value
  uint8_t value;
  if (!parse_uint8_arg_max(argv[0], argv[3], 1, &value)) {
    return;
  }

  // Set pin
  GPIO_PinWrite(GPIO, port, pin, value);

  printf("Set PIO%d_%d to %d", port, pin, value);
}

#if 0

// TODO: Port this to LPC for GPIO probing and SPI port scanning

static void
dump_gpio_port(GPIO_TypeDef *port, char *header)
{
  printf("%sMODER: 0x%08lx OTYPER: 0x%08lx OSPEEDR: 0x%08lx PUPDR: 0x%08lx\n",
      header, port->MODER, port->OTYPER, port->OSPEEDR, port->PUPDR);

  printf("%sIDR:   0x%08lx ODR:    0x%08lx AFR0:    0x%08lx AFR1:  0x%08lx\n\n",
      header, port->IDR, port->ODR, port->AFR[0], port->AFR[1]);
  //TODO: parse out the fields and make something fancy
}

void
gpioq_command(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: argc = %d\n", argc);
    return;
  }

#ifdef GPIOA
  dump_gpio_port(GPIOA, "GPIOA ");
#endif
#ifdef GPIOB
  dump_gpio_port(GPIOB, "GPIOB ");
#endif
#ifdef GPIOC
  dump_gpio_port(GPIOC, "GPIOC ");
#endif
#ifdef GPIOD
  dump_gpio_port(GPIOD, "GPIOD ");
#endif
#ifdef GPIOE
  dump_gpio_port(GPIOE, "GPIOE ");
#endif
#ifdef GPIOF
  dump_gpio_port(GPIOF, "GPIOF ");
#endif
#ifdef GPIOG
  dump_gpio_port(GPIOG, "GPIOG ");
#endif
#ifdef GPIOH
  dump_gpio_port(GPIOH, "GPIOH ");
#endif
#ifdef GPIOI
  dump_gpio_port(GPIOI, "GPIOI ");
#endif
}

static void
dump_spi(SPI_TypeDef *spi, char *header)
{
  printf("%sCR1: 0x%08lx CR2: 0x%08lx SR: 0x%08lx DR: 0x%08lx\n",
      header, spi->CR1, spi->CR2, spi->SR, spi->DR);

  printf("%sCRCPR:   0x%08lx RXCRCR:    0x%08lx TXCRCR: 0x%08lx\n\n",
      header, spi->CRCPR, spi->RXCRCR, spi->TXCRCR);
  //TODO: parse out the fields and make something fancy
}

void
spiq_command(int argc, char **argv)
{
  if (argc != 1) {
    printf("Error: argc = %d\n", argc);
    return;
  }

#ifdef SPI1
  dump_spi(SPI1, "SPI1 ");
#endif
#ifdef SPI2
  dump_spi(SPI2, "SPI2 ");
#endif
#ifdef SPI3
  dump_spi(SPI3, "SPI3 ");
#endif
}
#endif

void power_off_command(int argc, char **argv) {
  system_monitor_event_power_off();
}


