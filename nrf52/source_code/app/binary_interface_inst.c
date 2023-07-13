
#include "binary_interface_inst.h"

BinaryInterface bin_itf;

static void handle_commands(BinaryReader *r)
{
    char data_array[LPC_MAX_COMMAND_SIZE + 1];
    ErrValUINT8 data_array_size = readSTRING(r, data_array, LPC_MAX_COMMAND_SIZE + 1);
    // check for parsing error
    if (data_array_size.error_ != ERROR_NONE)
    {
        NRF_LOG_ERROR("Failed to read command.");
        return;
    }
    // check if buffer is full
//    if (data_array_size.value_ > LPC_MAX_COMMAND_SIZE)
//    {
//        // Buffer is full--reset
//        // Note that since logging is deferred, command may not print out right
//        NRF_LOG_ERROR("Command too long, ignoring: '%s'", data_array);
//        return;
//    }
    // Parse command
    lpc_uart_parse_command((char *)data_array, data_array_size.value_);
}

bool bin_itf_send_command(char *buf, size_t buf_size)
{
    BinaryWriter *bw = getWriter(&bin_itf);

    bool success = true;
    success &= writeUINT8(bw, CC_CHARACTERISTIC);
    success &= writeSTRING(bw, (char *)buf, buf_size);
    // For efficiency, return line must be incorporated into command.
    // success &= writeSTRING(bw, "\n", 1);
    success &= bw_send(bw);
    return success;
}

// defined in main.c
extern void ble_nus_send_data_wrapper(char *buf, size_t buf_size);

static void handle_file_command(BinaryReader *r)
{
    char data_array[BUFFER_SIZE+1];
    ErrValUINT8 data_array_size = readSTRING(r, data_array, BUFFER_SIZE+1);
    // check for parsing error
    
    if (data_array_size.error_ != ERROR_NONE)
    {
        NRF_LOG_ERROR("Failed to read command.");
        return;
    }
    // check if buffer is full
//    if (data_array_size.value_ > BUFFER_SIZE)
//    {
//        // Buffer is full--reset
//        // Note that since logging is deferred, command may not print out right
//        NRF_LOG_ERROR("Command too long, ignoring: '%s'", data_array);
//        return;
//    }

    // send data over bluetooth
    ble_nus_send_data_wrapper((char *)data_array,data_array_size.value_);
}

bool bin_itf_send_file_command(const uint8_t *buf, size_t buf_size)
{
    BinaryWriter *bw = getWriter(&bin_itf);

    bool success = true;
    success &= writeUINT8(bw, CC_NUS);
    success &= writeSTRING(bw, (char *)buf, buf_size);
    success &= bw_send(bw);
    return success;
}

size_t serial_write(uint8_t val)
{
    // TODO: Error handling
    ret_code_t err_code;
    // Send the data one byte at a time.
    err_code = app_uart_put(val);
    // Return success
    if (err_code != NRF_SUCCESS)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

size_t serial_write_buffer(const char *buffer, size_t size)
{
    // TODO: Error handling
    ret_code_t err_code;
    // Next, send the data one byte at a time.
    int index;
    for (index = 0; index < size; index++)
    {
        do
        {
        	//break; // TODO: yield for buffer space
        	err_code = app_uart_put(buffer[index]);
        }while(err_code != NRF_SUCCESS);
    }
    return index;
}

Command bin_itf_commands[] = {
    {CC_NONE, NULL},
    {CC_UNUSED1, NULL},
    {CC_UNUSED2, NULL},
    {CC_NUS, handle_file_command},
    {CC_CHARACTERISTIC, handle_commands}};

void bin_itf_init()
{
    // serial functions
    bin_itf.pSerial.serial_available_f = 0;
    bin_itf.pSerial.serial_read_f = 0;

    bin_itf.pSerial.serial_write_f = serial_write;
    bin_itf.pSerial.serial_write_buffer_f = serial_write_buffer;
    
    bin_itf.on_packet_f = NULL;

    bi_init(&bin_itf, bin_itf_commands, ARRAY_SIZE(bin_itf_commands));
}

bool bin_itf_handle_messages(uint8_t data)
{
    return handleMessages(&bin_itf, data);
}
