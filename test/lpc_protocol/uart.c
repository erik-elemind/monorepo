/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: UART interface for tests running on a PC/laptop host.
 */

#include "uart.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

// Serial port file descriptor
static int serial_fd;

int uart_init(const char* port, const char settings[3], const uint32_t baudrate)
{
    struct termios t;

    serial_fd = open(port, O_RDWR);
    if (serial_fd < 0) 
    {
        return -1;
    }

    (void) tcgetattr(serial_fd, &t);
    cfmakeraw(&t);
    t.c_cflag |= CLOCAL; // ignore modem control lines

    // character bits
    t.c_cflag &= ~CSIZE;
    switch (settings[0])
    {
        case '5':
            t.c_cflag |= CS5;
            break;
        case '6':
            t.c_cflag |= CS6;
            break;
        case '7':
            t.c_cflag |= CS7;
            break;
        case '8':
        default:
            t.c_cflag |= CS8;
            break;
    }

    // parity
    switch (settings[1])
    {
        case 'E':
            t.c_cflag |= PARENB;
            t.c_cflag &= ~PARODD;
            break;
        case 'O':
            t.c_cflag |= PARENB;
            t.c_cflag |= PARODD;
            break;
        case 'N':
        default:
            t.c_cflag &= ~PARENB;
            break;
    }

    // stop bits
    switch (settings[2])
    {
        case '2':
            t.c_cflag |= CSTOPB;
            break;
        case '1':
        default:
            t.c_cflag &= ~CSTOPB;
            break;
    }

    #if __APPLE__
    (void) tcsetattr(serial_fd, TCSANOW, &t);
    // use this hacked-up ioctl which Apple supports for higher/non-standard baud rates
    (void) ioctl(serial_fd, 0x80045402, &baudrate);
    #else
    // TODO: This needs to be a #define, eg B115200
    cfsetspeed(&t, (speed_t) encodedBaudRate);
    (void) tcsetattr(serial_fd, TCSANOW, &t);
    #endif

    return 0;
}

#if 0
void set_blocking(bool should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (serial_fd, &tty) != 0)
    {
        printf("error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 1;// 0.1 seconds read timeout

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0)
    {
        printf("error %d setting term attributes", errno);
    }
}
#endif

int uart_read_byte(uint8_t* byte)
{
    // Check if there is any data available on the port, 
    // and return immediately if not.
    struct pollfd myPoll = { .fd = serial_fd, .events = POLLIN };
    if (poll(&myPoll, 1, 0)) 
    {
        ssize_t result = read(serial_fd, byte, 1);
        if (result == 1)
        {
            return 0;
        }
    }

    return -1;
}

int uart_write(const uint8_t* buf, uint32_t len)
{
    return len == write(serial_fd, buf, len) ? 0 : -1;
}