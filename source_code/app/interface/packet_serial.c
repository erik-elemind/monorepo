
#include "packet_serial.h"

void init_cobs_packet_serial(PacketSerial *ps, uint8_t packet_marker)
{
    ps->encode_f = cobs_encode;
    ps->decode_f = cobs_decode;
    ps->get_encoded_buffer_size_f = cobs_getEncodedBufferSize;

    ps->recieve_buffer_index = 0;
    ps->packet_marker = packet_marker;
}

void init_slip_packet_serial(PacketSerial *ps, uint8_t packet_marker)
{
    ps->encode_f = slip_encode;
    ps->decode_f = slip_decode;
    ps->get_encoded_buffer_size_f = slip_getEncodedBufferSize;

    ps->recieve_buffer_index = 0;
    ps->packet_marker = packet_marker;
}

void update_polling(PacketSerial *ps)
{
    if (ps == NULL || ps->serial_available_f == NULL || ps->serial_read_f == NULL)
    {
        return;
    }

    while (ps->serial_available_f() > 0)
    {

        uint8_t data = ps->serial_read_f();

        if (data == ps->packet_marker)
        {

            size_t numDecoded = ps->decode_f(ps->recieve_buffer,
                                             ps->recieve_buffer_index,
                                             ps->decode_buffer);

            if (ps->on_packet_f)
            {
                ps->on_packet_f(ps->on_packet_c, ps->decode_buffer, numDecoded);
            }

            ps->recieve_buffer_index = 0;
        }
        else
        {
            if ((ps->recieve_buffer_index + 1) < BUFFER_SIZE)
            {
                ps->recieve_buffer[(ps->recieve_buffer_index)++] = data;
            }
            else
            {
                // Error, buffer overflow if we write.
            }
        }
    }
}

void update(PacketSerial *ps, uint8_t data)
{
    if (ps == NULL)
    {
        return;
    }

    if (data == ps->packet_marker)
    {

        size_t numDecoded = ps->decode_f(ps->recieve_buffer,
                                         ps->recieve_buffer_index,
                                         ps->decode_buffer);

        if (ps->on_packet_f)
        {
            ps->on_packet_f(ps->on_packet_c, ps->decode_buffer, numDecoded);
        }

        ps->recieve_buffer_index = 0;
    }
    else
    {
        if ((ps->recieve_buffer_index + 1) < BUFFER_SIZE)
        {
            ps->recieve_buffer[(ps->recieve_buffer_index)++] = data;
        }
        else
        {
            // Error, buffer overflow if we write.
        }
    }
}

void send(PacketSerial *ps, const uint8_t *buffer, size_t size)
{
    if (ps == NULL || ps->encode_f == NULL || ps->serial_write_buffer_f == NULL || ps->serial_write_f == NULL || buffer == NULL || size == 0)
    {
        return;
    }

    size_t numEncoded = ps->encode_f(buffer,
                                     size,
                                     ps->encode_buffer);

    ps->serial_write_buffer_f((const char *)ps->encode_buffer, numEncoded);
    ps->serial_write_f(ps->packet_marker);
}
