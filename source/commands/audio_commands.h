/*
 * audio_commands.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 */

#ifndef AUDIO_COMMANDS_H_
#define AUDIO_COMMANDS_H_

#include <stdbool.h>
#include <stdint.h>
#include "audio_task.h"

#ifdef __cplusplus
extern "C" {
#endif

void audio_power_on_command(int argc, char **argv);
void audio_power_off_command(int argc, char **argv);
void audio_pause_command(int argc, char **argv);
void audio_unpause_command(int argc, char **argv);
void audio_stop_command(int argc, char **argv);
void audio_set_volume_command(int argc, char **argv);
void audio_inc_volume_command(int argc, char **argv);
void audio_dec_volume_command(int argc, char **argv);
void audio_mute_command(int argc, char **argv);
void audio_unmute_command(int argc, char **argv);

void audio_fg_fade_in_command(int argc, char **argv);
void audio_fg_fade_out_command(int argc, char **argv);

void audio_bgwav_play_command(int argc, char **argv);
void audio_bgwav_stop_command(int argc, char **argv);
void audio_bg_fade_in_command(int argc, char **argv);
void audio_bg_fade_out_command(int argc, char **argv);
void audio_bg_volume_command(int argc, char **argv);

void audio_mp3_play_command(int argc, char **argv);
void audio_mp3_stop_command(int argc, char **argv);
void audio_mp3_fade_in_command(int argc, char **argv);
void audio_mp3_fade_out_command(int argc, char **argv);

void audio_pink_play_command(int argc, char **argv);
void audio_pink_stop_command(int argc, char **argv);
void audio_pink_fade_in_command(int argc, char **argv);
void audio_pink_fade_out_command(int argc, char **argv);
void audio_pink_mute_command(int argc, char **argv);
void audio_pink_unmute_command(int argc, char **argv);
void audio_pink_volume_command(int argc, char **argv);

void audio_play_test_command(int argc, char **argv);
void audio_stop_test_command(int argc, char **argv);

void erp_start_command(int argc, char **argv);
void erp_stop_command(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_COMMANDS_H_ */
