/*
 * nand_W25N04KW.c
 *
 * Copyright (C) 2020 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Generic NAND flash driver.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#include "nand_W25N04KW.h"
#include "nand_platform.h"

// Set the log level for this file
#define LOG_LEVEL_MODULE  LOG_WARN
#include "loglevels.h"

/// Logging prefix
static const char* TAG = "nand";

/* NAND Flash transaction buffer
 * The 'nand_read_buff' buffer is used when a FLEXSPI command type "kFLEXSPI_Read" operation is executed.
 * The FLEXSPI read operation clobbers 4 bytes at a time. That is to say, the data-pointer passed to a read
 * operation will be treated as if it points to multiples of 4 bytes. Even when reading 1 byte,
 * the subsequent 3 bytes will have zeros written to them.
 *
 * The 'nand_read_buff' is only used when the user-provided (uint8_t* p_data argument) is not a multiple of 4.
 * It is currently only used in feature/status register READs, and 'read_page_from_cache' READ.
 *
 * The 'nand_read_buff' size is large enough to hold one page of data + spare, because that is the
 * EXPECTED largest transation that could ever occur in the 'read_page_from_cache' READ.
 */
DMA_ALLOCATE_DATA_TRANSFER_BUFFER(static uint32_t nand_read_buff[(NAND_PAGE_PLUS_SPARE_SIZE >> 2)], sizeof(uint32_t)) = {0};

/* RESET_NOW - reset NAND immediately.
 * ENABLE_RESET and ACTIVATE RESET are used together.
 * ENABLE_RESET first, followed by ACTIVATE_RESET.*/
#define NAND_CMD_LUT_SEQ_IDX_RESET            0
#define NAND_CMD_LUT_SEQ_IDX_READ_ID          1
#define NAND_CMD_LUT_SEQ_IDX_READ_STATUS      2
#define NAND_CMD_LUT_SEQ_IDX_WRITE_STATUS     3
#define NAND_CMD_LUT_SEQ_IDX_WRITE_ENABLE     4
#define NAND_CMD_LUT_SEQ_IDX_WRITE_DISABLE    5
#define NAND_CMD_LUT_SEQ_IDX_BLOCK_ERASE      6
#define NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_ZEROED 7
#define NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_RANDOM 8
#define NAND_CMD_LUT_SEQ_IDX_PROGRAM_EXECUTE  9
#define NAND_CMD_LUT_SEQ_IDX_PAGE_READ        10
#define NAND_CMD_LUT_SEQ_IDX_FAST_READ        11
#define NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUAD   12
#define NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUADIO 13
#define NAND_CMD_LUT_SEQ_IDX_POWER_DOWN       14
#define NAND_CMD_LUT_SEQ_IDX_POWER_UP         15


/* LUT for the W25N04KW NAND Flash*/
#define NAND_FLEXSPI_LUT_LENGTH 64

const uint32_t NAND_FLEXSPI_LUT[NAND_FLEXSPI_LUT_LENGTH] = {
    /* Device Reset (FFh) */
	/* Enable Reset (66h) */
	/* Reset Device (99h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_RESET] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x08, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Read ID (9Fh) */
	[4 * NAND_CMD_LUT_SEQ_IDX_READ_ID] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x9F, kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08), //8
	[4 * NAND_CMD_LUT_SEQ_IDX_READ_ID + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Read Status Register (0Fh)*/
	[4 * NAND_CMD_LUT_SEQ_IDX_READ_STATUS] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x0F, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x08), //8
	[4 * NAND_CMD_LUT_SEQ_IDX_READ_STATUS + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Write Status Register (1Fh)*/
	[4 * NAND_CMD_LUT_SEQ_IDX_WRITE_STATUS] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x1F, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x08), //8
	[4 * NAND_CMD_LUT_SEQ_IDX_WRITE_STATUS + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Write Enable (06h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_WRITE_ENABLE] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Write Disable (04h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_WRITE_DISABLE] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Block Erase (D8h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_BLOCK_ERASE] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xD8, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18), //24

	/* Quad Load Program Data (32h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_ZEROED] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x32, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x10), //16
	[4 * NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_ZEROED + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Quad Random Load Program Data (34h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_RANDOM] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x34, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x10), //16
	[4 * NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_RANDOM + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Program Execute (10h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_PROGRAM_EXECUTE] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x10, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18), //24

	/* Page Data Read (13h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_PAGE_READ] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x13, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18), //24

	/* Fast Read Output (0Bh) */
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x0B, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x10), //16
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08, kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x00),

	/* Fast Read Quad Output (6Bh) */
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUAD] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x6B, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x10), //16
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUAD + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08, kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x00),

	/* Fast Read Quad I/0) (ECh) */ // ????????????????
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUADIO] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xEC, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 0x10), //16
	[4 * NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUADIO + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x0A, kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x00),

	/* Deep Power Down (B9h) */
	[4 * NAND_CMD_LUT_SEQ_IDX_POWER_DOWN] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB9, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

	/* Release Power-Down (ABh) */
	[4 * NAND_CMD_LUT_SEQ_IDX_POWER_UP] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xAB, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

};

/** Convert 24-bit page address (in a uint32_t) to 3-byte MSB-first array.

    Don't mask off the first byte--the uint8_t conversion will do that
    for us. This format allows us to use the uint32 address to directly
    initialize the address field in a xxx_command_t struct, i.e.:

      .page_addr = PAGE_ADDR_TO_MSB_FIRST_ARRAY(page_addr)
 */
#define PAGE_ADDR_TO_MSB_FIRST_ARRAY(x) \
  { (uint8_t)(x >> 16), (uint8_t)(x >> 8), (uint8_t)x }

/** Convert 16-bit page offset (in a uint16_t) to 2-byte MSB-first array.

    See notes above. Page offset is the address within a page (since
    pages are 4K+256 bytes, max page offset is 0x10FF == 4352).
 */
#define PAGE_OFFSET_TO_MSB_FIRST_ARRAY(x) \
  { (uint8_t)(x >> 8), (uint8_t)x }


// Flash commands (originally used with GigaDevice GD5F family)
typedef enum {
  // These commands do NOT need an address:
  COMMAND_WRITE_ENABLE  = NAND_CMD_LUT_SEQ_IDX_WRITE_ENABLE,
  COMMAND_WRITE_DISABLE = NAND_CMD_LUT_SEQ_IDX_WRITE_DISABLE,
  COMMAND_POWER_DOWN    = NAND_CMD_LUT_SEQ_IDX_POWER_DOWN,
  COMMAND_POWER_UP      = NAND_CMD_LUT_SEQ_IDX_POWER_UP,

  // These commands DO need an address:
  COMMAND_BLOCK_ERASE =  NAND_CMD_LUT_SEQ_IDX_BLOCK_ERASE,
  COMMAND_PROGRAM_EXECUTE = NAND_CMD_LUT_SEQ_IDX_PROGRAM_EXECUTE,
  COMMAND_PAGE_READ = NAND_CMD_LUT_SEQ_IDX_PAGE_READ
} nand_command_t;

typedef enum{
  COMMAND_RESET_NOW = 0xFF,
  COMMAND_ENABLE_RESET = 0x66,
  COMMAND_ACTIVATE_RESET = 0x99,
} nand_reset_command_t;

// Device constants

// Prevent padding bytes in register union structs. (This is popped back below.)
#pragma pack(push,1)

// Protection Register (Status Register 1) type
typedef union {
  struct {
    // lsb
    bool    srp1 : 1;
    bool    wpe  : 1;
    bool    tb   : 1;
    uint8_t bp   : 4;
    bool    srp0 : 1;
    // msb
  };
  uint8_t raw;
} nand_protection_reg_t;

// Configuration Register (Status Register 2) type
typedef union {
  struct {
    // lsb
    bool    hdis : 1;
    uint8_t ods  : 2;
    bool    buf  : 1;
    uint8_t ecce : 4;
    bool    sr1l : 1;
    bool    otpe : 1;
	bool    otpl : 1;
    // msb
  };
  uint8_t raw;
} nand_configuration_reg_t;

// Status Register (Status Register 3) type
typedef union {
  struct {
    // lsb
    bool    busy     : 1;
    bool    wel      : 1;
    bool    efail    : 1;
    bool    pfail    : 1;
    uint8_t ecc      : 2;
    uint8_t reserved : 2;
    // msb
  };
  uint8_t raw;
} nand_status_reg_t;

typedef union{
  struct{
    // lsb
    uint8_t  reserved : 4;
    uint8_t  bfd      : 4;
    // msb
  };
  uint8_t raw;
} nand_bit_flip_count_reg_t;

typedef union{
  struct{
    // lsb
    bool     bfs0     : 1;
    bool     bfs1     : 1;
    bool     bfs2     : 1;
    bool     bfs3     : 1;
    uint8_t  reserved : 4;
    // msb
  };
  uint8_t raw;
} nand_bit_flip_status_reg_t;

typedef union{
  struct{
    // lsb
    uint8_t mfs      : 3;
    uint8_t reserved : 1;
    uint8_t mbf      : 4;
    // msb
  };
  uint8_t raw;
} nand_max_bit_flip_count_reg_t;

typedef union{
  struct{
    // lsb
    uint8_t bfr_sec0     : 4;
    uint8_t bfr_sec1     : 4;
    // msb
  };
  uint8_t raw;
} nand_bit_flip_count_sec01_reg_t;

typedef union{
  struct{
    // lsb
    uint8_t bfr_sec2     : 4;
    uint8_t bfr_sec3     : 4;
    // msb
  };
  uint8_t raw;
} nand_bit_flip_count_sec23_reg_t;



// Read ID command response
typedef union {
  struct __attribute__((packed)) {
    uint8_t mfg_id;
    uint8_t device_id[2];
  };
  uint8_t raw[3];
} read_id_response_t;

#pragma pack(pop)

/** PROTOTYPES */
static int nand_ecc_result(nand_status_reg_t status_reg);

/** Send command (no response data).

    Sends provided command.

    @param user_data Flash SPI handle
    @param command Command to send
    @param command_len Command length, in bytes

    @return 0 if command and response completed
    succesfully, or error code
 */
static int
nand_command(
  nand_user_data_t *user_data,
  nand_command_t command,
  uint32_t address
  )
{
  status_t status;
  flexspi_transfer_t flashXfer;

  flashXfer.deviceAddress = address;
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Command;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = command;
  flashXfer.data = NULL;
  flashXfer.dataSize = 0;

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  return status;
}

int
nand_init(){
  int status = 0;

  // Update LUT
  FLEXSPI_UpdateLUT(FLEXSPI, 0, NAND_FLEXSPI_LUT, 64);

  FLEXSPI_SoftwareReset(FLEXSPI);

  // enable ecc
  nand_ecc_enable(NULL, true);

  // set bit fit count detection
  nand_bit_flip_count_reg_t ecc1_reg;
  ecc1_reg.raw = 0;
  ecc1_reg.bfd = NAND_ECC_SAFE_CORRECTED_BIT_COUNT;
  status = nand_set_feature_reg(NULL, FEATURE_REG_ECC1, (uint8_t) ecc1_reg.raw);
  if (status!= 0){
	  return status;
  }

  status = nand_platform_init();
  return status;
}

/** NAND RESET */
int
nand_reset(nand_reset_command_t command){
    status_t status;
    flexspi_transfer_t flashXfer;

    flashXfer.deviceAddress = command;
    flashXfer.port = kFLEXSPI_PortA1;
    flashXfer.cmdType = kFLEXSPI_Command;
    flashXfer.SeqNumber = 1;
    flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_RESET;
    flashXfer.data = NULL;
    flashXfer.dataSize = 0;

    status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

    return status;
}

/** NAND POWER DOWN/UP */
static int
nand_power(nand_user_data_t *user_data, bool enable){
	if(enable){
		return nand_command(user_data, COMMAND_POWER_UP, 0);
	}else{
		return nand_command(user_data, COMMAND_POWER_DOWN, 0);
	}
}

/** Get NAND ID. */
int
nand_get_id(
  nand_user_data_t *user_data,
  uint8_t* p_mfg_id,
  uint16_t* p_device_id
  )
{
  LOGV(TAG, "nand_get_id");

  flexspi_transfer_t flashXfer;
  status_t status;
  read_id_response_t response;

  memset(nand_read_buff, 0, sizeof(nand_read_buff));

  flashXfer.deviceAddress = 0;
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Read;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READ_ID;
  flashXfer.data = nand_read_buff;
  flashXfer.dataSize = 3;

  // check that we're WRITING within feature_buff size
  configASSERT(flashXfer.dataSize <= sizeof(nand_read_buff));

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  memcpy(response.raw, &(nand_read_buff[0]), 3);
  *p_mfg_id = response.mfg_id;
  *p_device_id = (response.device_id[0] << 8) | (response.device_id[1]);

  return status;
}

/** Get feature register.

    @param user_data Flash SPI handle
    @param feature_reg Feature register to read
    @param data Data to read (1 byte)

    @return 0 if command and response completed
    succesfully, or error code
 */
int
nand_get_feature_reg(
    nand_user_data_t *user_data,
    feature_reg_t feature_reg,
    uint8_t *data)
{
  flexspi_transfer_t flashXfer;
  status_t status;

  memset(nand_read_buff, 0, sizeof(nand_read_buff));

  flashXfer.deviceAddress = feature_reg;
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Read;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READ_STATUS;
  flashXfer.data = nand_read_buff;
  flashXfer.dataSize = 1;

  // check that we're WRITING within feature_buff size
  configASSERT(flashXfer.dataSize <= sizeof(nand_read_buff));

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  *data = nand_read_buff[0] & 0xFF;

  LOGV(TAG, "nand_get_feature_reg: feature_reg 0x%X (1 B) in: 0x%X (1 B)", feature_reg, data);

  return status;
}

/** Set feature register.

    @param user_data Flash SPI handle
    @param feature_reg Feature register to read
    @param data Data to write (1 byte)

    @return 0 if command and response completed
    succesfully, or error code
 */
int
nand_set_feature_reg(
  nand_user_data_t *user_data,
  feature_reg_t feature_reg,
  uint8_t data
  )
{
  LOGV(TAG, "nand_set_feature_reg: feature_reg 0x%X (1 B) in: 0x%X (1 B)", feature_reg, data);

  flexspi_transfer_t flashXfer;
  status_t status;

  flashXfer.deviceAddress = feature_reg;
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Write;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_WRITE_STATUS;
  flashXfer.data = &data;
  flashXfer.dataSize = 1;

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  return status;
}

/** Thin convenience function for readability.

    Output: nand_status_reg_t *status_reg so we can check for
            OIP, E_FAIL, P_FAIL, and ECC bits

    @return 0 or error
 */
static inline int
nand_get_status_reg(
  nand_user_data_t *user_data,
  nand_status_reg_t *status_reg
  )
{
  LOGV(TAG, "nand_get_status_reg: status_reg 0x%X (1 B)", FEATURE_REG_STATUS);

  // Get status register
  int status = nand_get_feature_reg(user_data,
    FEATURE_REG_STATUS, &(status_reg->raw));

  LOGV(TAG, "...nand_get_status_reg: out: 0x%X (1 B)", (uint8_t)status_reg->raw);

  return status;
}

/** Unlock flash. */
int
nand_unlock(nand_user_data_t *user_data)
{
  LOGV(TAG, "nand_unlock");

  nand_protection_reg_t protection_reg;
  protection_reg.srp1 = 0;
  protection_reg.wpe = 0;
  protection_reg.tb = 0;
  protection_reg.bp = 0;
  protection_reg.srp0 = 0;

  int status = nand_set_feature_reg(user_data,
    FEATURE_REG_PROTECTION, protection_reg.raw);

  return status;
}

/** Write Enable/Disable*/
static int
nand_write_enable(nand_user_data_t *user_data, bool enable){
	if(enable){
		return nand_command(user_data, COMMAND_WRITE_ENABLE, 0);
	}else{
		return nand_command(user_data, COMMAND_WRITE_DISABLE, 0);
	}
}

/** Enable/disable on-chip ECC. */
int
nand_ecc_enable(nand_user_data_t *user_data, bool enable)
{
  LOGV(TAG, "nand_ecc_enable: %d", (int)enable);

  nand_configuration_reg_t configuration_reg;
  int status = nand_get_feature_reg(user_data,
      FEATURE_REG_CONFIGURATION, &configuration_reg.raw);
  if (status < 0) {
    return status;
  }

  configuration_reg.ecce = enable;
  status = nand_set_feature_reg(user_data,
      FEATURE_REG_CONFIGURATION, configuration_reg.raw);

  return status;
}

/** Read flash page. */
int
nand_read_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  int status;
  int ecc_status;

  // page_addr here is the bitwise combo of both block number and page number.
  uint32_t page_addr = nand_block_page_to_page_addr(block, page, user_data->chipinfo->block_addr_offset);

  LOGV(TAG, "nand_read_page: blk %d, pg %d, pg_addr 0x%lX, pg_offset %d, p_data %p, len %d",
    (int)block, (int)page, page_addr, (int)page_offset, p_data, (int)data_len);

  status = nand_read_page_into_cache(user_data, page_addr);
  LOGV(TAG, "nand_read_page_into_cache: status: %d", (int) status);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    LOGE(TAG, "nand_read_page(%lu.%lu): nand_read_page_into_cache: %d", block, page, (int)status);
    return status;
  }

  // status could be: NO_ERR (0), ECC_OK (1), ECC_FAIL (-2)
  ecc_status = status;

  // Read page out from the chip's internal cache
  // This way we should get most data, even if it includes some corrupt bits
  status = nand_read_page_from_cache(user_data, page_offset, p_data, data_len);
  LOGV(TAG, "nand_read_page_from_cache: status: %d", (int) status);
  if (status < 0) {
    LOGE(TAG, "nand_read_page(%lu.%lu): nand_read_page_from_cache: %d", block, page, (int)status);
    return status;
  }

  if (ecc_status == NAND_ECC_FAIL) {
    LOGW(TAG, "nand_read_page(%lu.%lu): status_reg.eccs: %d, returning NAND_ECC_FAIL", block, page, ecc_status);
    return NAND_ECC_FAIL;
  }

  if (ecc_status == NAND_ECC_OK) {
    status = NAND_ECC_OK;
  }

  // return NAND_NO_ERR or NAND_ECC_OK
  return status;
}

/** Write flash page. */
int
nand_write_page(
  nand_user_data_t *user_data,
  uint32_t block,
  uint32_t page,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  int status;

  // page_addr here is the bitwise combo of both block number and page number.
  uint32_t page_addr = nand_block_page_to_page_addr(block, page, user_data->chipinfo->block_addr_offset);

  LOGV(TAG, "nand_write_page: page_addr 0x%lX, page_offset %d, p_data %p, data_len %d",
    page_addr, (int)page_offset, p_data, (int)data_len);

  ///@todo BH - Check page_addr, page_offset, data_len value

  // Write data to program cache
  status = nand_write_into_page_cache(user_data, page_offset, p_data, data_len);
  LOGV(TAG, "nand_write_into_page_cache: status: %d", (int) status);
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_PROGRAM_LOAD error: %d",
      status);
    return status;
  }

  status = nand_program_page_cache(user_data, page_addr);
  LOGV(TAG, "nand_program_page_cache: status: %d", (int) status);
  return status;
}

/** Copy page: Copy a page into another address using internal cache.

    @param user_data Platform/user data handle
    @param src_page_addr Source 24-bit page address (incl. block shifted over some bits)
    @param dest_page_addr Destination 24-bit page address (incl. block shifted over some bits)

    NOTE: Unlike other functions, this does not take a block index, page index pair.
    It takes the page address, as returned from nand_block_page_to_page_addr().

    @return 0 if successful, or other value if error
*/
// IMPORTANT: This function will not work for our part on arbitrary page
// pairs -- only some cases are permissible. See comments in
// dhara_nand.c for details.
int
nand_copy_page_from_cache(
  nand_user_data_t *user_data,
  uint32_t src_page_addr,
  uint32_t dest_page_addr
  )
{
  LOGV(TAG, "nand_copy_page_from_cache: src_pg_addr 0x%lX, dest_pg_addr 0x%lX", src_page_addr, dest_page_addr);

  int status;
  int ecc_status;

  status = nand_read_page_into_cache(user_data, src_page_addr);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    LOGE(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): nand_read_page_into_cache: %d", src_page_addr, dest_page_addr, (int)status);
    return status;
  }

  // Check ECCS bits after read from cache operation. Note that ECC failures
  // are expected normal behavior as the chip wears out.
  // status could be: NO_ERR (0), ECC_OK (1), ECC_FAIL (-2)
  ecc_status = status;

  if (ecc_status == NAND_ECC_FAIL) {
    LOGW(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): status_reg.eccs: %d, returning NAND_ECC_FAIL", src_page_addr, dest_page_addr, ecc_status);
    return NAND_ECC_FAIL;
  }

  if (ecc_status == NAND_ECC_OK) {
    status = NAND_ECC_OK;
  }

  status = nand_program_page_cache(user_data, dest_page_addr);
  if (status < 0) {
    LOGE(TAG, "nand_copy_page_from_cache(0x%lx->0x%lx): nand_read_page_from_cache: %d", src_page_addr, dest_page_addr, (int)status);
    return status;
  }

  // return NAND_NO_ERR or NAND_ECC_OK
  return status;
}

/** Read flash page cache. */
int
nand_read_page_from_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  LOGV(TAG, "nand_read_page_from_cache: page_offset %d, p_data 0x%lX... (%d B)",
    (int)page_offset, *(uint32_t *)p_data, (int)data_len);

  int status;
  flexspi_transfer_t flashXfer;

  // Create a temporary data pointer and flag.
  // These are used when the 'p_data' pointer passed in is not a multiple of 4 bytes
  // This is because the FLEXSPI driver always reads in 4 byte increments,
  // even if only 1 byte is read, the remaining 3 bytes will be zeroed.
  configASSERT(data_len <= sizeof(nand_read_buff));
  uint8_t* p_data_temp = p_data;
  bool use_data_temp = (data_len % sizeof(uint32_t)) != 0;

  if (use_data_temp){
	  p_data_temp = (uint8_t*) &(nand_read_buff[0]);
  }


#if 1
  uint8_t seqIndex = NAND_CMD_LUT_SEQ_IDX_FAST_READ;
#elif 0
  uint8_t seqIndex = NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUAD;
#else
  uint8_t seqIndex = NAND_CMD_LUT_SEQ_IDX_FAST_READ_QUADIO;
#endif

  flashXfer.deviceAddress = page_offset;
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Read;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = seqIndex;
  flashXfer.data = (uint32_t*) p_data_temp;
  flashXfer.dataSize = data_len;

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  if(status == kStatus_Success &&use_data_temp ){
	  memcpy(p_data, p_data_temp, data_len);
  }

  return status;
}

/** Load flash page into the NAND chip's onboard cache, return ECC result. 
    Returns: <0, or One of NO_ERR (0), ECC_OK (1), or ECC_FAIL (-2)
*/
int
nand_read_page_into_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

#if 0
  if (g_runtime_log_level == LOG_VERBOSE) {
    nand_print_detailed_status(user_data);
  }
#endif

  status = nand_command(user_data,COMMAND_PAGE_READ,page_addr);
  if (status < 0) {
    LOGE(TAG, "nand_read_page_into_cache(): COMMAND_PAGE_READ error: %d", status);
    return status;
  }

  // Wait for internal cache read to complete
  while(true) {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);
    if(status == kStatus_Success){
      if(status_reg.busy == 0){
        break;
      }
    }
    // We are still waiting for the flash chip to be ready.
    nand_platform_yield_delay(0);   // 0 means just yield; this should take <1 ms (80 usecs)
  };
  if (status < 0) {
    LOGE(TAG, "nand_read_page_into_cache(): _is_device_ready() error: %d", status);
    return status;
  }

  // Check ECCS bits after read from cache operation. Note that ECC failures
  // are expected normal behavior as the chip wears out.
  return nand_ecc_result(status_reg);

  return 0;
}

/** Write flash page cache. */
int
nand_write_into_page_cache(
  nand_user_data_t *user_data,
  uint16_t page_offset,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  LOGV(TAG, "nand_write_into_page_cache: page_offset %d, p_data: 0x%lX... (%d B)",
    (int)page_offset, *(uint32_t *)p_data, (int)data_len);

  int status;
  flexspi_transfer_t flashXfer;

  // Set write enable latch
  status = nand_write_enable(user_data, true);
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_WRITE_ENABLE error: %d",
      status);
    return status;
  }

#if 1
  uint8_t seqIndex = NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_ZEROED;
#else
  uint8_t seqIndex = NAND_CMD_LUT_SEQ_IDX_QUAD_LOAD_RANDOM;
#endif

  flashXfer.deviceAddress = page_offset; // TODO: Is this just the page_offset ????
  flashXfer.port = kFLEXSPI_PortA1;
  flashXfer.cmdType = kFLEXSPI_Write;
  flashXfer.SeqNumber = 1;
  flashXfer.seqIndex = seqIndex;
  flashXfer.data = (uint32_t*) p_data;
  flashXfer.dataSize = data_len;

  status = FLEXSPI_TransferBlocking(NAND_FLEXSPI_PERIPHERAL, &flashXfer);

  return status;
}

// Flush the NAND Chip internal buffer to the give 24-bit page_addr
int nand_program_page_cache(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

  // Program data from cache into flash page
  status = nand_command(user_data, COMMAND_PROGRAM_EXECUTE, page_addr); // TODO: Use page_addr directly???
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): COMMAND_PROGRAM_EXECUTE error: %d",
      status);
    return status;
  }

  // Wait for program execution to complete
  while(true) {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);
    if(status == kStatus_Success){
      if(status_reg.busy == 0){
        break;
      }
    }
    // TODO: Handle other bad status
    // We are still waiting for the flash chip to be ready.
    nand_platform_yield_delay(0);   // 0 means just yield; this should take <1 ms (80 usecs)
  };
  if (status < 0) {
    LOGE(TAG, "nand_write_page(): _is_device_ready() error: %d", status);
    return status;
  }

  // Check status register bit
  if (status_reg.pfail == 1) {
    status = NAND_UNKNOWN_ERR;  // Program Failed
  }

  return status;
}

/** Check flash block. */
int
nand_block_status(
  nand_user_data_t *user_data,
  uint32_t page_addr,
  bool* p_good
  )
{
  ///@todo BH Check page_addr valid
  // NOTE: Only page 0 is valid for a factory bad block check

  int status;

  // TODO: Not sure if disabling ECC is necessary for the Winbond Flash.
//  // check if ECC is enabled
//  // according to the documentation, ecc should be disabled when checking the bad page mark.
//  nand_configuration_reg_t config_reg;
//  status = nand_get_feature_reg(user_data,
//		  FEATURE_REG_CONFIGURATION, &config_reg.raw);
//  if (status != 0) {
//    return status;
//  }
//  bool ecc_enabled = config_reg.ecce;
//
//  // disable ECC
//  if(ecc_enabled){
//    config_reg.ecce = false;
//    status = nand_set_feature_reg(user_data,
//    		FEATURE_REG_CONFIGURATION, config_reg.raw);
//    if (status != 0) {
//      return status;
//    }
//  }

  // Get block and page numbers
  uint32_t block, page;
  nand_page_addr_to_block_page(
    page_addr, user_data->chipinfo->block_addr_offset, &block, &page);
  // But always use page 0 since that is where the marker is.
  page = 0;

  // First byte of spare is the marker. Datasheet calls this the
  // "Bad Block Mark".
  static const uint32_t GOOD_BLOCK_MARKER_ADDR = NAND_PAGE_SIZE;
  // Anything other than 0xFF is bad.
  static const uint8_t GOOD_BLOCK_MARKER = 0xFF;

  uint8_t data;
  status = nand_read_page(user_data,
    block, page, GOOD_BLOCK_MARKER_ADDR, &data, sizeof(data));
  if (status == kStatus_Success) {
    *p_good = (data == GOOD_BLOCK_MARKER);
  }else{
    *p_good = false;
  }

  // TODO: Not sure if disabling ECC is necessary for the Winbond Flash.
  // enable ECC
//  if(ecc_enabled){
//	config_reg.ecce = true;
//    status = nand_set_feature_reg(user_data,
//    		FEATURE_REG_CONFIGURATION, config_reg.raw);
//    if (status != 0) {
//      return status;
//    }
//  }

  return status;
}


/** Erase flash block. */
int
nand_erase_block(
  nand_user_data_t *user_data,
  uint32_t page_addr
  )
{
  int status;
  nand_status_reg_t status_reg;

  ///@todo BH - Check page_addr valid
  uint32_t block;
  uint32_t page;
  nand_page_addr_to_block_page(page_addr,user_data->chipinfo->block_addr_offset, &block, &page);
  LOGV(TAG, "nand_erase_block(): page_addr 0x%lx (%lu), page %lu of block %lu", page_addr, page_addr, page, block);

  status = nand_write_enable(user_data, true);
  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): COMMAND_WRITE_ENABLE error: %d",
      status);
    return status;
  }

  // Erase block
  status = nand_command(user_data, COMMAND_BLOCK_ERASE, (block << NAND_PAGES_PER_BLOCK_LOG2));
  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): COMMAND_ERASE_BLOCK error: %d",
      status);
    return status;
  }

  /* Wait for erase block to complete (could take up to 5 ms--wait in
     between checks). */

  // "Typical: 3 ms, max: 5 ms". Minimize SPI bus traffic by sleeping the first 2 ms.
  nand_platform_yield_delay(2);

  do {
    // Check OIP (Operation In Progress) bit
    status = nand_get_status_reg(user_data, &status_reg);
    if (status == NAND_NO_ERR && status_reg.busy == 1) {
      // We are still waiting for the flash chip to be ready.
      nand_platform_yield_delay(1);
    }
  } while (status == NAND_NO_ERR && status_reg.busy == 1);

  if (status < 0) {
    LOGE(TAG, "nand_erase_block(): _get_status_reg() error: %d", status);
    return status;
  }

  // Check E_FAIL bit in status reg
  if (status_reg.efail == 1) {
    LOGW(TAG, "nand_erase_block(): e_fail==1 on block: %ld, returning NAND_UNKNOWN_ERR", block);
    status = NAND_UNKNOWN_ERR;  // Erase Failed.
  }

  return status;
}

/* Output detailed flash status. */
int
nand_print_detailed_status(
  nand_user_data_t *user_data
  )
{
  int status;

  // Get protection register
  nand_protection_reg_t protection_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_PROTECTION, &protection_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Protection register (%02x): 0x%x\n",
      FEATURE_REG_PROTECTION, protection_reg.raw);
    printf("    srp1: %d\n", protection_reg.srp1);
    printf("    wpe: %d\n", protection_reg.wpe);
    printf("    tb: %d\n", protection_reg.tb);
    printf("    bp: %d\n", protection_reg.bp);
    printf("    srp0: %d\n", protection_reg.srp0);
  }
  else {
    printf("Error reading protection register: %d\n", status);
  }

  // Get configuration register
  nand_configuration_reg_t config_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_CONFIGURATION, &config_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Feature1 register (%02x): 0x%x\n",
    		FEATURE_REG_CONFIGURATION, config_reg.raw);
    printf("    hdis: %d\n", config_reg.hdis);
    printf("    ods: %d\n", config_reg.ods);
    printf("    buf: %d\n", config_reg.buf);
    printf("    ecce: %d\n", config_reg.ecce);
    printf("    sr1l: %d\n", config_reg.sr1l);
    printf("    otpe: %d\n", config_reg.otpe);
    printf("    otpl: %d\n", config_reg.otpl);
  }
  else {
    printf("Error reading feature1 register: %d\n", status);
  }

  // Get status register
  nand_status_reg_t status_reg;
  status = nand_get_feature_reg(user_data,
    FEATURE_REG_STATUS, &status_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  Status register (%02x): 0x%x\n",
      FEATURE_REG_STATUS, status_reg.raw);
    printf("    busy: %d\n", status_reg.busy);
    printf("    wel: %d\n", status_reg.wel);
    printf("    efail: %d\n", status_reg.efail);
    printf("    pfail: %d\n", status_reg.pfail);
    printf("    ecc: %d\n", status_reg.ecc);
  }
  else {
    printf("Error reading status register: %d\n", status);
  }

  // Get ECC1 register
  nand_bit_flip_count_reg_t ecc1_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_ECC1, &ecc1_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  ECC1 register (%02x): 0x%x\n",
    		FEATURE_REG_ECC1, ecc1_reg.raw);
    printf("    bfd: %d\n", ecc1_reg.bfd);
  }
  else {
    printf("Error reading ECC1 register: %d\n", status);
  }

  ////////////////////////////////////////////

  // Get ECC2 register
  nand_bit_flip_status_reg_t ecc2_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_ECC2, &ecc2_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  ECC1 register (%02x): 0x%x\n",
    		FEATURE_REG_ECC2, ecc2_reg.raw);
    printf("    bfs0: %d\n", ecc2_reg.bfs0);
    printf("    bfs1: %d\n", ecc2_reg.bfs1);
    printf("    bfs2: %d\n", ecc2_reg.bfs2);
    printf("    bfs3: %d\n", ecc2_reg.bfs3);
  }
  else {
    printf("Error reading ECC2 register: %d\n", status);
  }

  // Get ECC3 register
  nand_max_bit_flip_count_reg_t ecc3_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_ECC3, &ecc3_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  ECC1 register (%02x): 0x%x\n",
    		FEATURE_REG_ECC3, ecc3_reg.raw);
    printf("    mfs: %d\n", ecc3_reg.mfs);
    printf("    mbf: %d\n", ecc3_reg.mbf);
  }
  else {
    printf("Error reading ECC3 register: %d\n", status);
  }

  // Get ECC4 register
  nand_bit_flip_count_sec01_reg_t ecc4_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_ECC4, &ecc4_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  ECC1 register (%02x): 0x%x\n",
    		FEATURE_REG_ECC4, ecc4_reg.raw);
    printf("    bfr_sec0: %d\n", ecc4_reg.bfr_sec0);
    printf("    bfr_sec1: %d\n", ecc4_reg.bfr_sec1);
  }
  else {
    printf("Error reading ECC4 register: %d\n", status);
  }

  // Get ECC5 register
  nand_bit_flip_count_sec23_reg_t ecc5_reg;
  status = nand_get_feature_reg(user_data,
		  FEATURE_REG_ECC5, &ecc5_reg.raw);
  if (status == NAND_NO_ERR) {
    printf("  ECC1 register (%02x): 0x%x\n",
    		FEATURE_REG_ECC5, ecc5_reg.raw);
    printf("    bfr_sec2: %d\n", ecc5_reg.bfr_sec2);
    printf("    bfr_sec3: %d\n", ecc5_reg.bfr_sec3);
  }
  else {
    printf("Error reading ECC5 register: %d\n", status);
  }

  return status;
}


/** Evaluate the status register ECC bits for data safety analysis.

  WARNING: Not all chips report the number of ECC bits in the status register.
  Further work is be necessary for chips which do not report
  this status field, including the Macronix MX35LF1 family.

  @param status_reg  The NAND status register byte to evaluate.
  @return            One of NO_ERR (0), ECC_OK (1), or ECC_FAIL (-2)
*/
static int
nand_ecc_result(nand_status_reg_t status_reg)
{
  switch (status_reg.ecc){
  case 0: return NAND_NO_ERR;
  case 1: return NAND_NO_ERR;
  case 2: return NAND_ECC_FAIL;
  case 3: return NAND_ECC_OK;
  default: return NAND_ECC_FAIL;
  }
}

/** Read and evaluate the status register ECC bits for data safety analysis. */
int nand_ecc_bits(nand_user_data_t *user_data, uint32_t* num_bits)
{
  int status = 0;
  *num_bits = 0xFFFFFFFF;

  nand_bit_flip_count_sec01_reg_t ecc_sec01_reg;
  status = nand_get_feature_reg(user_data, FEATURE_REG_ECC4, &ecc_sec01_reg.raw);
  if (status != 0) {
	  return status;
  }

  nand_bit_flip_count_sec23_reg_t ecc_sec23_reg;
  status = nand_get_feature_reg(user_data, FEATURE_REG_ECC5, &ecc_sec23_reg.raw);
  if (status != 0) {
	  return status;
  }

  *num_bits = ecc_sec01_reg.bfr_sec0 + ecc_sec01_reg.bfr_sec1 +  ecc_sec23_reg.bfr_sec2 + ecc_sec23_reg.bfr_sec3;

  return status;
}


