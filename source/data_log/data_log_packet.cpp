/*
 * data_log_packet.cpp
 *
 *  Created on: May 5, 2022
 *      Author: DavidWang
 */

#include "data_log_packet.h"

// Returns the enum as a string. Helpful for logs.
const char* data_log_packet_str(data_log_packet_t type)
{
    switch (type)
    {
        case DLPT_EEG_START: return "DLPT_EEG_START";
        case DLPT_EEG_DATA: return "DLPT_EEG_DATA";
        case DLPT_INST_AMP_PHS: return "DLPT_INST_AMP_PHS";
        case DLPT_PULSE_START_STOP: return "DLPT_PULSE_START_STOP";
        case DLPT_EEG_DATA_PACKED: return "DLPT_EEG_DATA_PACKED";
        case DLPT_INST_AMP_PHS_PACKED: return "DLPT_INST_AMP_PHS_PACKED";
        case DLPT_ECHT_CHANNEL: return "DLPT_ECHT_CHANNEL";
        case DLPT_SWITCH: return "DLPT_SWITCH";
        case DLPT_STIM_AMP: return "DLPT_STIM_AMP";
        case DLPT_STIM_AMP_PACKED: return "DLPT_STIM_AMP_PACKED";
        case DLPT_CMD: return "DLPT_CMD";
        case DLPT_ACCEL_XYZ: return "DLPT_ACCEL_XYZ";
        case DLPT_ACCEL_TEMP: return "DLPT_ACCEL_TEMP";
        case DLPT_ALS: return "DLPT_ALS";
        case DLPT_MIC: return "DLPT_MIC";
        case DLPT_TEMP: return "DLPT_TEMP";
        case DLPT_EEG_COMP_HEADER: return "DLPT_EEG_COMP_HEADER";
        case DLPT_EEG_COMP_FRAME: return "DLPT_EEG_COMP_FRAME";
        case DLPT_INST_AMP_COMP_HEADER: return "DLPT_INST_AMP_COMP_HEADER";
        case DLPT_INST_AMP_COMP_FRAME: return "DLPT_INST_AMP_COMP_FRAME";
        case DLPT_INST_PHS_COMP_HEADER: return "DLPT_INST_PHS_COMP_HEADER";
        case DLPT_INST_PHS_COMP_FRAME: return "DLPT_INST_PHS_COMP_FRAME";
    }

    return "unknown";
}
