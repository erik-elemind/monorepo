/*
 * data_log_packet_types.h
 *
 *  Created on: Apr 22, 2022
 *      Author: DavidWang
 */

#ifndef DATA_LOG_DATA_LOG_PACKET_H_
#define DATA_LOG_DATA_LOG_PACKET_H_

#include "config.h"
#include "data_log_buffer.h"
#include "compression.h"

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Packet type configuration
 */

#define PACKET_TYPE_IGNORE (0U)
#define PACKET_TYPE_BASIC  (1U)
#define PACKET_TYPE_PACKED (2U)
#define PACKET_TYPE_COMP   (3U)

#ifndef LOG_EEG
//#define LOG_EEG PACKET_TYPE_PACKED
#define LOG_EEG PACKET_TYPE_COMP
#endif

#ifndef LOG_INST_AMP_PHS
//#define LOG_INST_AMP_PHS PACKET_TYPE_PACKED
#define LOG_INST_AMP_PHS PACKET_TYPE_COMP
#endif

#ifndef LOG_STIM_AMP
#define LOG_STIM_AMP PACKET_TYPE_PACKED
#endif

typedef enum data_log_packet_t{
  DLPT_CMD=1,
  DLPT_SOURCE_INFO=2,
  DLPT_SAMPLE_TIME=3,
  DLPT_LOST_SAMPLES=4,
  DLPT_EEG_INFO = 9,
  DLPT_EEG_DATA = 10,
  DLPT_INST_AMP_PHS = 11,
  DLPT_PULSE_START_STOP = 12,
  DLPT_EEG_DATA_PACKED = 13,
  DLPT_INST_AMP_PHS_PACKED = 14,
  DLPT_ECHT_CHANNEL=15,
  DLPT_SWITCH=16,
  DLPT_STIM_AMP=17,
  DLPT_STIM_AMP_PACKED=18,
  DLPT_ACCEL_XYZ=20,
  DLPT_ACCEL_TEMP=21,
  DLPT_ALS=22,
  DLPT_MIC=23,
  DLPT_SKIN_TEMP=24,
  DLPT_EEG_COMP_HEADER=25,
  DLPT_EEG_COMP_FRAME=26,
  DLPT_INST_AMP_COMP_HEADER=27,
  DLPT_INST_AMP_COMP_FRAME=28,
  DLPT_INST_PHS_COMP_HEADER=29,
  DLPT_INST_PHS_COMP_FRAME=30,
} data_log_packet_t;

#define SAMPLE_NUMBER_SIZE sizeof(unsigned long)
#define PACKET_TYPE_SIZE sizeof(data_log_packet_t)

#define MICRO_TIME_SIZE sizeof(uint64_t)

// data packing routines
#define EEG_PACK_NUM_SAMPLES_TO_SEND 10
#define EEG_PACK_BUFFER_SIZE (PACKET_TYPE_SIZE +\
        SAMPLE_NUMBER_SIZE +\
    (EEG_PACK_NUM_SAMPLES_TO_SEND*9))

// data packing routines
#define INST_PACK_NUM_SAMPLES_TO_SEND 10
#define INST_PACK_BUFFER_SIZE (PACKET_TYPE_SIZE +\
		SAMPLE_NUMBER_SIZE +\
        (INST_PACK_NUM_SAMPLES_TO_SEND*8))

// data packing routines
#define STIM_AMP_PACK_NUM_SAMPLES_TO_SEND 10
#define STIM_AMP_PACK_BUFFER_SIZE (PACKET_TYPE_SIZE +\
		SAMPLE_NUMBER_SIZE +\
        (STIM_AMP_PACK_NUM_SAMPLES_TO_SEND*1))

/***************************/
// EEG Compression Types

#define EEG_SAMPLE_FRAME_SIZE_MAX       512
#define EEG_SAMPLE_FRAME_SIZE           512

// Check the frame size is less than or equal to max
static_assert(EEG_SAMPLE_FRAME_SIZE <= EEG_SAMPLE_FRAME_SIZE_MAX,
  "Desired Frame Size is larger than Maximum FrameSize");

#define EEG_COMP_BUFFER_SIZE(SZ) ((size_t) (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + 1 + ((SZ)*1.5+1)))

// TODO: Remove the 100 fudge factor in the allocation of this buffer
#define EEG_COMP_BUFFER_SIZE_MAX (EEG_COMP_BUFFER_SIZE(EEG_SAMPLE_FRAME_SIZE_MAX))

#define DLPT_COMP_HEADER_SIZE (1 + COMP_HEADER_MAX_SIZE)

typedef struct alignas(4) {
  float buffer[EEG_SAMPLE_FRAME_SIZE_MAX];
  size_t index;
} eeg_comp_frame_buf_t;

typedef uint8_t* (*get_buffer_f )(size_t);
typedef void (*write_buffer_f )(uint8_t*, size_t);

typedef struct {
  comp_params_t comp_params;

  union{
    eeg_comp_frame_buf_t eegs[3] = {0};
    struct{
    eeg_comp_frame_buf_t eeg0;
    eeg_comp_frame_buf_t eeg1;
    eeg_comp_frame_buf_t eeg2;
    };
  }bufs;

  bool first_eeg_header = true;
  bool first_eeg_sample_number = true;
  uint32_t eeg_sample_number = 0;

  get_buffer_f get_buffer;
  write_buffer_f write_buffer;

} eeg_comp_t;

void eeg_comp_init(eeg_comp_t* eeg_comp);
void eeg_comp_reset(eeg_comp_t* eeg_comp);
void eeg_comp_add_data_and_write(eeg_comp_t* eeg_comp, uint32_t sample_number, int32_t eeg_channels[]);

/***************************/
// INST Compression Types

#define INST_SAMPLE_FRAME_SIZE_MAX       512
#define INST_SAMPLE_FRAME_SIZE           512

// Check the frame size is less than or equal to max
static_assert(INST_SAMPLE_FRAME_SIZE <= INST_SAMPLE_FRAME_SIZE_MAX,
  "Desired Frame Size is larger than Maximum FrameSize");

#define INST_COMP_BUFFER_SIZE(SZ) ((size_t) (PACKET_TYPE_SIZE + SAMPLE_NUMBER_SIZE + 1 + ((SZ)*1.5+1)))

// TODO: Remove the 100 fudge factor in the allocation of this buffer
#define INST_COMP_BUFFER_SIZE_MAX (INST_COMP_BUFFER_SIZE(INST_SAMPLE_FRAME_SIZE_MAX))

#define DLPT_COMP_HEADER_SIZE (1 + COMP_HEADER_MAX_SIZE)

typedef struct alignas(4) {
  float buffer[INST_SAMPLE_FRAME_SIZE_MAX];
  size_t index;
} inst_comp_frame_buf_t;

typedef uint8_t* (*get_buffer_f )(size_t);
typedef void (*write_buffer_f )(uint8_t*, size_t);

typedef struct {
  union{
    comp_params_t arr[2] = {0};
    struct{
      comp_params_t iamp;
      comp_params_t iphs;
    };
  } comp_params;

  union{
    inst_comp_frame_buf_t arr[2] = {0};
    struct{
    inst_comp_frame_buf_t iamp;
    inst_comp_frame_buf_t iphs;
    };
  }bufs;

  bool first_inst_header = true;
  bool first_inst_sample_number = true;
  uint32_t inst_sample_number = 0;

  get_buffer_f get_buffer;
  write_buffer_f write_buffer;

} inst_comp_t;

#define INST_COMP_IAMP_CHNUM 0
#define INST_COMP_IPHS_CHNUM 1

typedef struct{
  uint32_t sample_number;
  float instAmp;
  float instPhs;
} inst_amp_phs_t;

void inst_comp_init(inst_comp_t* inst_comp);
void inst_comp_reset(inst_comp_t* inst_comp);
void inst_comp_add_data_and_write(inst_comp_t* inst_comp, inst_amp_phs_t* inst_amp_phs);


// Reset functions defined in data_log_packet_***.cpp files
void data_log_eeg_init();
void data_log_eeg_reset();
void data_log_inst_init();
void data_log_inst_reset();
void data_log_stim_reset();

void handle_eeg_data(uint8_t* buf, size_t size);
void handle_inst_data(uint8_t* buf, size_t size);

// Helper function to get user-friendly packet type name
const char* data_log_packet_str(data_log_packet_t type);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif


#endif /* DATA_LOG_DATA_LOG_PACKET_H_ */
