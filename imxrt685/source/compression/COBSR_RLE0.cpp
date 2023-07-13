/*
 * COBSR_RLE0.h
 *
 * Consistent Overhead Byte Stuffing--Reduced (COBS/R) with Run-Length Encoding of 0.
 */

#include <math.h>
#include "COBSR_RLE0.h"

	
/* COBS/R-encode a string of input bytes, which may save one byte of output.
 *
 * dst_buf_ptr:    The buffer into which the result will be written
 * dst_buf_len:    Length of the buffer into which the result will be written
 * src_ptr:        The byte string to be encoded
 * src_len         Length of the byte string to be encoded
 *
 * returns:        A struct containing the success status of the encoding
 *                 operation and the length of the result (that was written to
 *                 dst_buf_ptr)
 */
cobsr_encode_result cobsr_rle0_encode(uint8_t* dst_buf_ptr, const int dst_buf_len,
    const uint8_t* src_ptr, const int src_len)
{
    cobsr_encode_result result              = { 0, COBSR_ENCODE_OK };
    const uint8_t *   src_read_ptr        = src_ptr;                 // src_ptr;
    const uint8_t *   src_end_ptr         = src_ptr + src_len;       // src_ptr + src_len;
    uint8_t *         dst_buf_start_ptr   = dst_buf_ptr;             // dst_buf_ptr;
    uint8_t *         dst_buf_end_ptr     = dst_buf_ptr+dst_buf_len; // dst_buf_ptr + dst_buf_len;
    uint8_t *         dst_code_write_ptr  = dst_buf_ptr;             // dst_buf_ptr;
    uint8_t *         dst_write_ptr       = dst_code_write_ptr + 1;
    uint8_t           src_byte            = 0;
    
    const int default_count_val = DEFAULT_COUNT_VAL;
    size_t   count_zero          = default_count_val;
    size_t   count_coeff         = default_count_val;
    
    uint8_t write_cap_byte = 0;

    /* First, do a NULL pointer check and return immediately if it fails. */
    if ((dst_buf_ptr == NULL) || (src_ptr == NULL))
    {
        result.status = COBSR_ENCODE_NULL_POINTER;
        return result;
    }

    if (src_len == 0) {
        *dst_code_write_ptr = (uint8_t) count_coeff;
    } else {        /* If first byte is a zero, force first byte to be the coeff count. */
        if (*src_ptr == 0) {
            *dst_code_write_ptr = (uint8_t) count_coeff;          // *dst_code_write_ptr = search_len;
            dst_code_write_ptr = dst_write_ptr++;
        }

        /* Iterate over the source bytes */
        for (;;)
        {
            /* Check for running out of output buffer space */
            if (dst_write_ptr >= dst_buf_end_ptr)
            {
                result.status = (cobsr_encode_status) (result.status | COBSR_ENCODE_OUT_BUFFER_OVERFLOW);
                break;
            }

            src_byte = *src_read_ptr++;                   // src_byte = *src_read_ptr++;
            
            if (src_byte == 0)
            {
                count_zero++;
                
                if(write_cap_byte==2) {
                    *dst_code_write_ptr = (uint8_t) default_count_val;
                    dst_code_write_ptr = dst_write_ptr++;
                }
                write_cap_byte = 0;
                
                // We have encountered the first 0 after a coefficient.
                if(count_coeff > default_count_val) {
                    *dst_code_write_ptr = (uint8_t) count_coeff;          // *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    count_coeff = default_count_val;
                }

                if(count_zero == OVER_COUNT_DELIMETER) {
                    *dst_code_write_ptr = (uint8_t) count_zero;           // *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    count_zero = default_count_val;
                    write_cap_byte = 1;
                }
            }
            else
            {
                /* Copy the non-zero byte to the destination buffer */
                
                count_coeff++;

                if(write_cap_byte==1) {
                    *dst_code_write_ptr = (uint8_t) default_count_val;
                    dst_code_write_ptr = dst_write_ptr++;
                }
                write_cap_byte = 0;

                if(count_zero > default_count_val) {
                    *dst_code_write_ptr = (uint8_t) count_zero;           // *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    count_zero = default_count_val;
                }

                *dst_write_ptr++ = src_byte;                    // *dst_write_ptr++ = src_byte;

                if(count_coeff == OVER_COUNT_DELIMETER) {
                    *dst_code_write_ptr = (uint8_t) count_coeff;          // *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    count_coeff = default_count_val;
                    write_cap_byte = 2;
                }
            }

            if (src_read_ptr >= src_end_ptr)
            {
                break;
            }
        }


        /* We've reached the end of the source data (or possibly run out of output buffer)
         * Finalise the remaining output. In particular, write the code (length) byte.
         *
         * For COBS/R, the final code (length) byte is special: if the final data byte is
         * greater than or equal to what would normally be the final code (length) byte,
         * then replace the final code byte with the final data byte, and remove the final
         * data byte from the end of the sequence. This saves one byte in the output.
         *
         * Update the pointer to calculate the final output length.
         */
        if (dst_code_write_ptr >= dst_buf_end_ptr)
        {
            /* We've run out of output buffer to write the code byte. */
            result.status = (cobsr_encode_status) (result.status | COBSR_ENCODE_OUT_BUFFER_OVERFLOW);
            dst_write_ptr = dst_buf_end_ptr;
        }
        else
        {
            if(count_zero > default_count_val) {
                *dst_code_write_ptr = (uint8_t) count_zero;           // *dst_code_write_ptr = search_len;
            }else if(count_coeff > default_count_val) {
                *dst_code_write_ptr = (uint8_t) count_coeff;          // *dst_code_write_ptr = search_len;
            }else {
                // We just wrote an OVER_COUNT_DELIMETER,
                // but there was no additional zeros or coeff counted afterwards,
                // we can remove the space left for the next 'dst_code'.
                dst_write_ptr--;
            }
        }
    }

    /* Calculate the output length, from the value of dst_code_write_ptr */
    result.out_len = dst_write_ptr - dst_buf_start_ptr;

    return result;
}



/* Decode a COBS/R byte string.
 *
 * dst_buf_ptr:    The buffer into which the result will be written
 * dst_buf_len:    Length of the buffer into which the result will be written
 * src_ptr:        The byte string to be decoded
 * src_len         Length of the byte string to be decoded
 *
 * returns:        A struct containing the success status of the decoding
 *                 operation and the length of the result (that was written to
 *                 dst_buf_ptr)
 */
cobsr_decode_result cobsr_rle0_decode(uint8_t* dst_buf_ptr, const int dst_buf_len,
                                 const uint8_t* src_ptr, const int src_len)
{
    cobsr_decode_result result              = { 0, COBSR_DECODE_OK };
    const uint8_t *     src_read_ptr        = src_ptr;
    const uint8_t *     src_end_ptr         = src_ptr+src_len;
    uint8_t *           dst_buf_start_ptr   = dst_buf_ptr;
    uint8_t *           dst_buf_end_ptr     = dst_buf_ptr+dst_buf_len;
    uint8_t *           dst_write_ptr       = dst_buf_ptr; 
    size_t              remaining_output_bytes;
    size_t              num_output_bytes;
    uint8_t             src_byte;
    int                 i;
    int                 len_code;
    
    const int default_count_val = DEFAULT_COUNT_VAL;
    
    bool expand_coeff = true;


    /* First, do a NULL pointer check and return immediately if it fails. */
    if ((dst_buf_ptr == NULL) || (src_ptr == NULL))
    {
        result.status = COBSR_DECODE_NULL_POINTER;
        return result;
    }

    if (src_len != 0)
    {
        for (;;)
        {
            len_code = *src_read_ptr++;    // len_code = *src_read_ptr++;

            if (len_code == 0 && default_count_val != 0)
            {
                result.status = (cobsr_decode_status) (result.status | COBSR_DECODE_ZERO_BYTE_IN_INPUT);
                break;
            }


            num_output_bytes = len_code - default_count_val;

            /* Check length code against remaining output buffer space */
            remaining_output_bytes = dst_buf_end_ptr - dst_write_ptr;
            if (num_output_bytes > remaining_output_bytes)
            {
                result.status = (cobsr_decode_status) (result.status | COBSR_DECODE_OUT_BUFFER_OVERFLOW);
                num_output_bytes = remaining_output_bytes;
            }

            if(expand_coeff) {
                for (i = num_output_bytes; i != 0; i--)
                {
                    src_byte = *src_read_ptr++;           // src_byte = *src_read_ptr++;
                    if (src_byte == 0)
                    {
                        result.status = (cobsr_decode_status) (result.status | COBSR_DECODE_ZERO_BYTE_IN_INPUT);
                    }
                    *dst_write_ptr++ = src_byte;        // *dst_write_ptr++ = src_byte;
                }
            }else {
                for (i = num_output_bytes; i != 0; i--)
                {
                    *dst_write_ptr++ = 0;
                }
            }
            if(len_code != OVER_COUNT_DELIMETER) {
            	expand_coeff = !expand_coeff;
            }


            if (src_read_ptr >= src_end_ptr)
            {
                break;
            }
            
        }

        result.out_len = dst_write_ptr - dst_buf_start_ptr;
    }

    return result;
}


size_t encode(uint8_t* buffer, int size, uint8_t* encoded, size_t encoded_length) {
	 cobsr_encode_result result = cobsr_rle0_encode(encoded, encoded_length, buffer, size);
	 return result.out_len;
}


size_t decode(uint8_t* buffer, int size, uint8_t* decoded, size_t decoded_length) {
	cobsr_decode_result result = cobsr_rle0_decode(decoded, decoded_length, buffer, size);
	return result.out_len;
}


size_t getEncodedBufferSize(size_t sourceSize) {
	if(sourceSize == 0 ) {
		return 1;
	}else {
		return (size_t) ceil(1.5*sourceSize)+1;
	}
}


