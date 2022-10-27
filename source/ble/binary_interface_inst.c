
#include "ble_shell.h"
#include "ble_uart_send.h"
#include "ble_uart_recv.h"
#include "binary_interface_inst.h"
#include "loglevels.h"

static const char *TAG = "ble_itf"; // Logging prefix for this module

BinaryInterface bin_itf;

static SemaphoreHandle_t itf_mutex = NULL;
static StaticSemaphore_t itf_mutex_static_buffer;

static void handle_shake_response(BinaryReader *r)
{
  // do nothing - deprecated
}

static void handle_commands(BinaryReader *r)
{
    static char data_array[BUFFER_SIZE] = {0};
    ErrValUINT8 data_array_size = readSTRING(r, data_array, BUFFER_SIZE);

    // check for parsing error
    if (data_array_size.error_ != ERROR_NONE)
    {
        //TODO: Concerned logging here might cause a hard fault.
        LOGE(TAG, "Failed to read command.");
        return;
    }
    // check if buffer is full
    if (data_array_size.value_ >= BUFFER_SIZE)
    {
        // Buffer is full--reset
        // Note that since logging is deferred, command may not print out right
        //TODO: Concerned logging here might cause a hard fault.
        LOGE(TAG, "Command too long, ignoring: '%s'", data_array);
        return;
    }
    // Parse command
    ble_uart_handle_input_buf(data_array, data_array_size.value_);
}

bool bin_itf_send_command(char* buf, size_t size){
  BinaryWriter *bw = getWriter(&bin_itf);

  bool success = true;

  xSemaphoreTake(itf_mutex, portMAX_DELAY);
  // TODO: these two writes need to be atomic and protected by a semaphore/mutex.
  success &= writeUINT8(bw, CC_COMMANDS);
  success &= writeSTRING(bw, (char*) buf, size);
//  success &= writeSTRING(bw, "\r\n", 2);
  success &= bw_send(bw);
  xSemaphoreGive(itf_mutex);

  return success;
}

static void handle_file_commands(BinaryReader *r)
{
  char data_array[BUFFER_SIZE] = {0};
  ErrValUINT8 data_array_size = readSTRING(r, data_array, BUFFER_SIZE);
  if(data_array_size.error_ == ERROR_NONE){
    //printf("BLE RX (%u): ",data_array_size.value_);
    //printf("%.*s\n",(size_t)data_array_size.value_,data_array);

    ble_shell_add_char_from_ble(data_array,data_array_size.value_);
  }
}

static void handle_ack(BinaryReader *r)
{
  // do nothing - deprecated
}

bool bin_itf_send_uart_command(char* buf, size_t buf_size) {
  //printf("BLE TX (%u): ",buf_size);
  //printf("%.*s\n",buf_size,buf);


  BinaryWriter *bw = getWriter(&bin_itf);

  bool success = true;

  xSemaphoreTake(itf_mutex, portMAX_DELAY);
  // These two writes need to be atomic and protected by a semaphore/mutex.
  success &= writeUINT8(bw, CC_UART);
  success &= writeSTRING(bw, buf, buf_size);
  success &= bw_send(bw);
  xSemaphoreGive(itf_mutex);

  return success;
}

static
size_t serial_write(uint8_t val){
  ble_uart_send_buf((const char*)&val, 1);
  return 1;
}

static
size_t serial_write_buffer(const char *buffer, size_t size){
  ble_uart_send_buf(buffer, size);
  return size;
}

static const Command bin_itf_commands[] = {
    {CC_NONE    , NULL},
    {CC_SHKREQ  , NULL},
    {CC_SHKRSP  , handle_shake_response},
    {CC_UART    , handle_file_commands},
    {CC_COMMANDS, handle_commands},
    {CC_ACK     , handle_ack}};

void bin_itf_init()
{
    itf_mutex = xSemaphoreCreateMutexStatic(&itf_mutex_static_buffer);
  
    // serial functions
    bin_itf.pSerial.serial_available_f = 0;
    bin_itf.pSerial.serial_read_f = 0;

    bin_itf.pSerial.serial_write_f = serial_write;
    bin_itf.pSerial.serial_write_buffer_f = serial_write_buffer;

    bi_init( &bin_itf, bin_itf_commands, sizeof(bin_itf_commands)/sizeof(bin_itf_commands[0]) );
}


bool bin_itf_handle_messages(uint8_t data){
  return handleMessages(&bin_itf, data);
}
