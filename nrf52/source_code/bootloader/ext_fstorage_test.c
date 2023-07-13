/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: June, 2021
 * Author:  Paul Adelsbach
 *
 * Description: External flash test code.
 */

#include "ext_fstorage_test.h"
#include "ext_fstorage.h"
#include "ext_flash.h"
#include "sha256.h"
#include "nrf_sdh.h" // for nrf_sdh_is_enabled()
#include "nrf_delay.h"

// nordic log
// <0=> Off
// <1=> Error
// <2=> Warning
// <3=> Info
// <4=> Debug
#define NRF_LOG_LEVEL       (4)
#define NRF_LOG_MODULE_NAME EXTTEST
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define NRF_RTT_DELAY_MS (5)

// Fill a buffer with random bytes
static void rand_fill(uint8_t* buf, uint32_t len)
{
    // Fill byte-at-a-time because the SD only has 64 byte rand buffer.
    // The delay between bytes is enough for it to refill. not really lol wut is this
    uint32_t size_remaining = len;   
    for (uint32_t i=0; i<len; i+=64)
    {
        uint8_t p_bytes_available = 0;	
        do{
            (void)sd_rand_application_bytes_available_get(&p_bytes_available);
        }while(p_bytes_available < 64); // max rand buffer size

        if (size_remaining > 64)
        {
            (void)sd_rand_application_vector_get(&buf[i], 64);
            size_remaining -= 64;
        }
        else
        {
            (void)sd_rand_application_vector_get(&buf[i], size_remaining);
        }
    }
}

// Test writing data of various lengths at various offsets within the page.
static bool test_write_offset(void)
{
    static uint8_t buf_read[4*SPI_FLASH_PAGE_LEN];
    static uint8_t buf_write[2*SPI_FLASH_PAGE_LEN];

    // Arbitrary base address for the test.
    // TODO: must be BLOCK aligned and start at defined block addr
    // Must be page aligned.
    static const uint32_t base_addr = 260864;// + 2*SPI_FLASH_BLOCK_LEN + 3*SPI_FLASH_PAGE_LEN;

    ret_code_t err_code;
    uint8_t exp;

    rand_fill(buf_write, sizeof(buf_write));

    // Speed things up by using lists of boundary case values rather than 
    // exhaustive loops.
    static const uint32_t offset_vals[] = {
        0,1,2,3,4,5,6,7,8,
        127,128,129,
        SPI_FLASH_PAGE_LEN-4,
        SPI_FLASH_PAGE_LEN-3,
        SPI_FLASH_PAGE_LEN-2,
        SPI_FLASH_PAGE_LEN-1,
        SPI_FLASH_PAGE_LEN,
        SPI_FLASH_PAGE_LEN+1,
        SPI_FLASH_PAGE_LEN+2,
        SPI_FLASH_PAGE_LEN+3,
    };
    static const uint32_t len_vals[] = {
        0,1,2,3,4,5,6,7,8,16,
        255,256,257,
        sizeof(buf_write)-4,
        sizeof(buf_write)-3,
        sizeof(buf_write)-2,
        sizeof(buf_write)-1,
        sizeof(buf_write),
    };

    NRF_LOG_INFO("test_writeoffset test .");

    for (uint32_t i=0; i<ARRAY_SIZE(offset_vals); i++)
    {
        uint32_t offset = offset_vals[i];

        for (uint32_t j=0; j<ARRAY_SIZE(len_vals); j++)
        {
            NRF_LOG_INFO("i: %d, j: %d", i, j);
            uint32_t len = len_vals[j];

            NRF_LOG_INFO("numblocks to erase: %d", ext_fstorage_block_count(base_addr, sizeof(buf_read)));

            // Erase the blocks to clean up
            err_code = ext_fstorage_erase(base_addr, 
                ext_fstorage_block_count(base_addr, sizeof(buf_read)), NULL, false);
            if (NRF_SUCCESS != err_code)
            {
                return false;
            }

            // Read back the larger buffer
            err_code = ext_fstorage_read(base_addr, buf_read, sizeof(buf_read));
            if (NRF_SUCCESS != err_code)
            {
                return false;
            }

            // Check it is all erased
            for (uint32_t i=0; i<sizeof(buf_read); i++)
            {
                if (0xFF != buf_read[i])
                {
                    NRF_LOG_WARNING("erased byte expected 0x%02X. found=0x%02X, i=%d, offset=%d, len=%d", 0xFF, buf_read[i], i, offset, len);
                    return false;
                }
            }

            // Write a number of bytes at the given offset
            err_code = ext_fstorage_write(base_addr+offset, buf_write, len, NULL, false);
            if (NRF_SUCCESS != err_code)
            {
                return false;
            }

            // Read back the larger buffer
            err_code = ext_fstorage_read(base_addr, buf_read, sizeof(buf_read));
            if (NRF_SUCCESS != err_code)
            {
                return false;
            }

            // Check it
            for (uint32_t i=0; i<sizeof(buf_read); i++)
            {
                if (i<offset || i>=offset+len)
                {
                    exp = 0xFF;
                }
                else
                {
                    exp = buf_write[i-offset];
                }

                if (buf_read[i] != exp)
                {
                    NRF_LOG_WARNING("expected 0x%02X. found=0x%02X, i=%d, offset=%d, len=%d", exp, buf_read[i], i, offset, len);
                    return false;
                }
            }

            NRF_LOG_INFO("write offset done. offset=%d, len=%d", offset, len);
        }
    }

    NRF_LOG_WARNING("write offset test: pass.");
    return true;
}

// Write a chunk to flash, then read back to verify in various lengths.
static bool test_read_offset(void)
{
    const uint32_t base_addr = 16*SPI_FLASH_BLOCK_LEN;
    const uint32_t len = 16*SPI_FLASH_PAGE_LEN;

    static uint8_t buf[2*SPI_FLASH_PAGE_LEN];

    // Use sha256 for integrity checking
    static sha256_context_t shactx;
    static uint8_t sha_write[32];
    static uint8_t sha_read[32];

    ret_code_t err_code;

    NRF_LOG_WARNING("read offset test: start.");

    // Erase the blocks to start clean
    err_code = ext_fstorage_erase(base_addr, ext_fstorage_block_count(base_addr, len), NULL, false);
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }

    err_code = sha256_init(&shactx);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha init failed. code=%d", err_code);
        return false;
    }

    for (uint32_t offset=0; offset<len; offset+=sizeof(buf))
    {
        uint32_t len_write = MIN(sizeof(buf), len-offset);

        // Fill the buffer with random bytes
        rand_fill(buf, len_write);

        // Write it
        err_code = ext_fstorage_write(base_addr+offset, buf, len_write, NULL, false);
        if (NRF_SUCCESS != err_code)
        {
            return false;
        }

        // Hash it for future comparison
        err_code = sha256_update(&shactx, buf, len_write);
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_WARNING("sha update failed. code=%d", err_code);
            return false;
        }
    }

    // Compute and save the sha in little endian
    err_code = sha256_final(&shactx, sha_write, true);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha final failed. code=%d", err_code);
        return false;
    }

    NRF_LOG_WARNING("read offset test: starting readback.");

    // Speed things up by using lists of boundary case values rather than 
    // exhaustive loops.
    static const uint32_t offset_vals[] = {
        0,1,2,3,4,5,6,7,8,
        sizeof(buf)/2,
        sizeof(buf)-3,
        sizeof(buf)-2,
        sizeof(buf)-1,
        sizeof(buf),
    };

    for (uint32_t i=0; i<ARRAY_SIZE(offset_vals); i++)
    {
        uint32_t offset = offset_vals[i];
        for (uint32_t len_read=1; len_read<=sizeof(buf); len_read++)
        {
            // Reinit the sha for readback
            err_code = sha256_init(&shactx);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_WARNING("sha init failed (read). code=%d", err_code);
                return false;
            }

            // Fill the buffer to clear any residual data
            memset(buf, 0xA5, sizeof(buf));

            // Read the initial chunk
            err_code = ext_fstorage_read(base_addr, buf, offset);
            if (NRF_SUCCESS != err_code)
            {
                return false;
            }
            err_code = sha256_update(&shactx, buf, offset);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_WARNING("sha update failed. code=%d", err_code);
                return false;
            }

            uint32_t bytes_remaining = len - offset;

            for (uint32_t addr=base_addr+offset; addr<base_addr+len; addr+=len_read)
            {
                uint32_t len_read_min = MIN(len_read, bytes_remaining);
                bytes_remaining -= len_read_min;
                err_code = ext_fstorage_read(addr, buf, len_read_min);
                if (NRF_SUCCESS != err_code)
                {
                    return false;
                }

                // Hash it for future comparison
                err_code = sha256_update(&shactx, buf, len_read_min);
                if (NRF_SUCCESS != err_code)
                {
                    NRF_LOG_WARNING("sha update failed. code=%d", err_code);
                    return false;
                }
            }

            // Compute and save the sha in little endian
            err_code = sha256_final(&shactx, sha_read, true);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_WARNING("sha final failed. code=%d", err_code);
                return false;
            }

            // Finally, check that the hashes match.
            if (0 == memcmp(sha_read, sha_write, sizeof(sha_read)))
            {
                NRF_LOG_WARNING("read offset done. offset=%d, len=%d", offset, len_read);
            }
            else 
            {
                NRF_LOG_WARNING("sha mismatch. read:");
                NRF_LOG_HEXDUMP_WARNING(sha_read, sizeof(sha_read));
                NRF_LOG_WARNING("sha mismatch. write:");
                NRF_LOG_HEXDUMP_WARNING(sha_write, sizeof(sha_write));
                return false;
            }
        }
    }

    NRF_LOG_WARNING("read offset test: pass");
    return true;
}

// Write a chunk of random data to flash.
// Then readback and check integrity against what was written.
// This mimics the DFU download flow.
static bool test_write_readback(const uint32_t base_addr, const uint32_t len)
{
    static uint8_t buf_read[SPI_FLASH_PAGE_LEN];
    static uint8_t buf_write[SPI_FLASH_PAGE_LEN];

    // Use sha256 for integrity checking
    static sha256_context_t shactx;
    static uint8_t sha_write[32];
    static uint8_t sha_read[32];

    ret_code_t err_code;

    NRF_LOG_WARNING("write-readback test: start. base=0x%08x, len=%d", base_addr, len);

    // Erase the range to start.
    // This is not explicitly tested, but readback will fail if this doesn't work.
    err_code = ext_fstorage_erase(base_addr, ext_fstorage_block_count(base_addr, len), NULL, false);
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }

    err_code = sha256_init(&shactx);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha init failed. code=%d", err_code);
        return false;
    }

    for (uint32_t addr=base_addr, len_write; addr<base_addr+len; addr+=len_write)
    {
        // Randomly pick a number of bytes to write
        rand_fill((uint8_t*)&len_write, sizeof(len_write));
        // But make sure it is not bigger than our buffer.
        len_write %= sizeof(buf_write);
        // And make sure not to extend past the len specified by the caller.
        if ((addr + len_write) > (base_addr + len))
        {
            len_write = (base_addr + len) - addr;
        }

        // Fill write buffer with random data
        rand_fill(buf_write, len_write);

        err_code = ext_fstorage_write(addr, buf_write, len_write, NULL, false);
        if (NRF_SUCCESS != err_code)
        {
            return false;
        }

        err_code = sha256_update(&shactx, buf_write, len_write);
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_WARNING("sha update failed. code=%d", err_code);
            return false;
        }
    }

    // Compute and save the sha in little endian
    err_code = sha256_final(&shactx, sha_write, true);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha final failed. code=%d", err_code);
        return false;
    }

    // Reinit the sha for readback
    err_code = sha256_init(&shactx);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha init failed (read). code=%d", err_code);
        return false;
    }

    NRF_LOG_WARNING("write-readback test: reading back.");

    for (uint32_t addr=base_addr, len_read; addr<base_addr+len; addr+=len_read)
    {
        len_read = MIN(sizeof(buf_read), base_addr+len-addr);

        // Fill the buffer to clear any residual data
        memset(buf_read, 0xA5, sizeof(buf_read));

        // Read back chunk-at-a-time
        err_code = ext_fstorage_read(addr, buf_read, len_read);
        if (NRF_SUCCESS != err_code)
        {
            return false;
        }

        err_code = sha256_update(&shactx, buf_read, len_read);
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_WARNING("sha update failed (read). code=%d", err_code);
            return false;
        }
    }

    // Compute and save the sha in little endian
    err_code = sha256_final(&shactx, sha_read, true);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_WARNING("sha final failed. code=%d", err_code);
        return false;
    }

    // Finally, check that the hashes match.
    if (0 == memcmp(sha_read, sha_write, sizeof(sha_read)))
    {
        NRF_LOG_WARNING("write-readback test: pass.");
    }
    else
    {
        NRF_LOG_WARNING("sha mismatch. read:");
        NRF_LOG_HEXDUMP_WARNING(sha_read, sizeof(sha_read));
        NRF_LOG_WARNING("sha mismatch. write:");
        NRF_LOG_HEXDUMP_WARNING(sha_write, sizeof(sha_write));
        return false;
    }

    return true;
}

static bool test_readId(void)
{
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("readID test .")
    return (NRF_SUCCESS == ext_flash_cmd_read_id());
}

static bool test_erase_first_block(void)
{
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("test_erase_first_block .");
    return (NRF_SUCCESS == ext_fstorage_erase(SPI_FLASH_OTA_START_ADDR, 1, NULL, false));
}

static bool test_read_first_page(void)
{
    uint8_t p_dest[SPI_FLASH_PAGE_LEN];

    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("test_read_first_page test .");
    
    if (NRF_SUCCESS == ext_fstorage_read(SPI_FLASH_OTA_START_ADDR, p_dest, SPI_FLASH_PAGE_LEN))
    {
        // nrf_delay_ms(NRF_RTT_DELAY_MS);
        // for (int i = 0; i < sizeof(p_dest); i++)
        // {
        //     NRF_LOG_INFO("%02X ", p_dest[i]);
        // }
        return true;
    }
    return false;
}

static bool test_write_first_page(void)
{
    uint8_t p_src[SPI_FLASH_PAGE_LEN];
    rand_fill(p_src, sizeof(p_src));

    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("test_write_first_page test .");

    // for (int i = 0; i < sizeof(p_src); i++)
    // {
    //     NRF_LOG_INFO("%02X ", p_src[i]);
    // }
    return (NRF_SUCCESS == ext_fstorage_write(SPI_FLASH_OTA_START_ADDR, p_src, SPI_FLASH_PAGE_LEN, NULL, false));
}

static bool test_write_read_first_page(void)
{
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    uint8_t p_write_buff[SPI_FLASH_PAGE_LEN];
    uint8_t p_read_buff[SPI_FLASH_PAGE_LEN];

    NRF_LOG_INFO("test_write_read_first_page .");

    // write to first page of flash
    rand_fill(p_write_buff, sizeof(p_write_buff));
    if (NRF_SUCCESS != ext_fstorage_write(SPI_FLASH_OTA_START_ADDR, p_write_buff, SPI_FLASH_PAGE_LEN, NULL, false))
    {
        return false;
    }

    // read first page of flash
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    if (NRF_SUCCESS != ext_fstorage_read(SPI_FLASH_OTA_START_ADDR, p_read_buff, SPI_FLASH_PAGE_LEN))
    {
        return false;
    }

    // compare both buffers
    for (int i = 0; i < SPI_FLASH_PAGE_LEN; i++)
    {
        if (p_write_buff[i] != p_read_buff[i])
        {
            NRF_LOG_INFO("index %d does not match! p_write_buff[i] = %d, p_read_buff[i] = %d", i, p_write_buff[i], p_read_buff[i]);
            return false;
        }
    }
    NRF_LOG_INFO("test_write_read_first_page success .");
    return true;
}

static bool test_erase_ota_all(void)
{
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("test_erase_ota_all .");
    return (NRF_SUCCESS == ext_fstorage_erase(SPI_FLASH_OTA_START_ADDR, SPI_FLASH_OTA_NUM_BLOCKS, NULL, false));
}

static bool test_write_verify_ota_all(void)
{
    nrf_delay_ms(NRF_RTT_DELAY_MS);
    NRF_LOG_INFO("test_write_verify_ota_all .");
    uint32_t addr = SPI_FLASH_OTA_START_ADDR;
    uint8_t p_write_buff[SPI_FLASH_PAGE_LEN];
    uint8_t p_read_buff[SPI_FLASH_PAGE_LEN];
    rand_fill(p_write_buff, sizeof(p_write_buff));

    // write each page at a time
    for (int i = 0; i < SPI_FLASH_OTA_NUM_BLOCKS*SPI_FLASH_PAGES_IN_BLOCK; i++)
    {
        if(NRF_SUCCESS == ext_fstorage_write(addr, p_write_buff, SPI_FLASH_PAGE_LEN, NULL, false))
        {
            // read page
            nrf_delay_ms(NRF_RTT_DELAY_MS);
            if (NRF_SUCCESS != ext_fstorage_read(addr, p_read_buff, SPI_FLASH_PAGE_LEN))
            {
                return false;
            }

            // simple verify 
            for (int i = 0; i < SPI_FLASH_PAGE_LEN; i++)
            {
                if (p_write_buff[i] != p_read_buff[i])
                {
                    NRF_LOG_INFO("index %d does not match! p_write_buff[i] = %d, p_read_buff[i] = %d", i, p_write_buff[i], p_read_buff[i]);
                    return false;
                }
            }

            //NRF_LOG_INFO("write success . addr=0x%06X", addr);
            if (i % SPI_FLASH_PAGES_IN_BLOCK == 0) NRF_LOG_INFO("."); // lightweight logging
            addr += 1;
            memset(p_read_buff, 0xA5, sizeof(p_read_buff));
            rand_fill(p_write_buff, sizeof(p_write_buff)); // comment this out if testing takes too long
        }
        else
        {
            return false;
        }
    }
    NRF_LOG_INFO("test_write_verify_ota_all success .");
    return true;
}

// Top level test routine.
void ext_fstorage_test(void)
{
    // Run test once, only after SD has been enabled.
    // Wait for SD to be enabled so that we can use the RNG.
    static bool test_run = false;
    if (test_run || !nrf_sdh_is_enabled())
    {
        NRF_LOG_WARNING("sd not enabled.");
        return;
    }

    test_run = true;

    if (test_readId() && 
        test_erase_ota_all() &&
        test_write_first_page() &&
        test_read_first_page() &&
        test_erase_first_block() &&
        test_write_read_first_page() &&
        test_erase_first_block() &&
        //test_write_verify_ota_all() &&
        test_erase_ota_all()) // clean flash at the end
        //test_write() &&
        //test_write_offset())
    // if (test_write_offset() &&
    //     test_read_offset() &&
    //     test_write_readback(0, 16*1024) &&
    //     test_write_readback(4*1024, 16*1024) &&
    //     test_write_readback(4*1024+1, 16*1024) &&
    //     test_write_readback(4*1024-1, 16*1024) &&
    //     test_write_readback(6*1024, 32*1024) &&
    //     test_write_readback(6*1024+1, 32*1024) &&
    //     test_write_readback(6*1024-1, 32*1024) &&
    //     test_write_readback(0, 1024*1024)
    //    )
    {
        NRF_LOG_WARNING("all tests pass.");
    }
    else 
    {
        NRF_LOG_WARNING("tests fail.");
    }
}