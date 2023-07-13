
#include "uart.h"
#include "lpc_protocol.h"
#include "lpc_pkt.h"

#include <stdio.h>
#include <sys/time.h>   // for gettimeofday()
#include <string.h> // for memcpy()

// Include the firmware file as an array
#include "lpcxpresso55s69_led_blinky.bin.c"

// Reference the FW file contents
static const uint8_t* fw_fdata = lpcxpresso55s69_led_blinky_bin;
static uint32_t fw_fsize = lpcxpresso55s69_led_blinky_bin_len;

#ifndef MIN
#define MIN(a,b)    ((a)<(b)?(a):(b))
#endif

// Implementation of the function needed by lpc interface code
int lpc_fw_file_read(uint32_t offset, uint8_t* p_buf, uint32_t len)
{
    if (offset + len > lpcxpresso55s69_led_blinky_bin_len)
    {
        return -1;
    }

    memcpy(p_buf, &fw_fdata[offset], len);

    return 0;
}

// Implementation of the function needed by lpc interface code
int lpc_uart_read_byte(uint8_t* byte) 
{ return uart_read_byte(byte); }

// Implementation of the function needed by lpc interface code
int lpc_uart_write(const uint8_t* buf, uint32_t len)
{ return uart_write(buf, len); }

int main(int argc, char* argv[])
{
    int res;

    if (argc < 2)
    {
        printf("Please specify UART port\n");
        printf("eg. %s /dev/cu.usbserial-123456\n", argv[0]);
        return -1;
    }

    res = uart_init(argv[1], "8N1", 115200);
    if (0 == res)
    {
        if (0 == lpc_protocol_init())
        {
            struct timeval t1, t2;
            // start timer
            gettimeofday(&t1, NULL);
            if (0 == lpc_protocol_apply_fw(fw_fsize))
            {
                // stop timer
                gettimeofday(&t2, NULL);
                // compute delta
                unsigned elapsed;
                elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
                elapsed += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

                printf("Test passed! Elapsed time: %ums\n", elapsed);
            }
            else 
            {
                printf("Failed to apply FW update\n");
                printf("Please check logs\n");
            }
        }
        else 
        {
            printf("Failed to establish link with target via PING packets.\n");
            printf("Please ensure device is in ISP mode.\n");
        }
    }
    else
    {
        printf("failed to open port %s.", argv[1]);
    }

    return 0;
}