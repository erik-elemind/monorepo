/*
 * audio_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Jul, 2020
 * Author:  David Wang
 */

#include "audio_commands.h"
#include "loglevels.h"
#include "command_helpers.h"
#include "interpreter.h"
#include "erp.h"
#include "settings.h"

static const char *TAG = "audio_commands"; // Logging prefix for this module

void audio_power_on_command(int argc, char **argv)
{
  audio_power_on();
}

void audio_power_off_command(int argc, char **argv)
{
  audio_power_off();
}

void audio_pause_command(int argc, char **argv)
{
  audio_pause();
}

void audio_unpause_command(int argc, char **argv)
{
  audio_unpause();
}

void audio_stop_command(int argc, char **argv)
{
  audio_stop();
}

void audio_set_volume_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint8_t volume;
  bool success = true;

  char parse_var_buf[10] = {0};
  parse_variable_return_t bg_select = PARSE_VAR_RETURN_NOTHING;
  bg_select = parse_variable_from_string(argv[1], parse_var_buf, sizeof(parse_var_buf));

  // TODO: Do something with tokens
  token_t tokens;
  parse_dot_notation(argv[1], sizeof(argv[1]), &tokens);

  switch(bg_select){
  case PARSE_VAR_RETURN_NOTHING:
    success &= false;
    break;
  case PARSE_VAR_RETURN_ARG:
	// if argument is value
    success &= parse_uint8_arg(argv[0], argv[1], &volume);
    break;
  case PARSE_VAR_RETURN_SETTINGS_VALUE:
  {
	  char settings_audio_vol[5] = {0};
	  char *token_name = tokens.v[1];
    // check if argument is alarm settings file
	  if(strcmp(token_name, "alarm.volume") == 0)
	  {
			if ( 0 == settings_get_string("alarm.volume", settings_audio_vol, sizeof(settings_audio_vol)) )
			{
			  volume = (uint8_t) atoi(settings_audio_vol);
			  success &= true;
			}
			else
			{
			  LOGE(TAG, "Error retrieving getting alarm volume setting\n\r");
			  success &= false;
			}
	  }
	  else
	  {
		  // set the volume to previously saved audio volume
		    if ( 0 == settings_get_string("audio.volume", settings_audio_vol, sizeof(settings_audio_vol)) )
		    {
		      volume = (uint8_t) atoi(settings_audio_vol);
		      success &= true;
		    }
		    else
		    {
		      LOGE(TAG, "Error retrieving getting audio volume setting\n\r");
		      success &= false;
		    }
	  }
    break;
  }
  case PARSE_VAR_RETURN_SETTINGS_KEY:
	// TODO: default audio volume?
	  volume = 60;
	  success &= true;
    // get default value
    break;
  }

  if(success){
    audio_set_volume( volume );
  }
}

void audio_inc_volume_command(int argc, char **argv)
{
  audio_volume_up();
}

void audio_dec_volume_command(int argc, char **argv)
{
  audio_volume_down();
}

void audio_mute_command(int argc, char **argv)
{
  audio_set_mute(true);
}

void audio_unmute_command(int argc, char **argv)
{
  audio_set_mute(false);
}

void audio_pink_mute_command(int argc, char **argv)
{
  audio_pink_mute(true);
}

void audio_pink_unmute_command(int argc, char **argv)
{
  audio_pink_mute(false);
}




void audio_fg_fade_in_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_fg_fadein( fade );
  }
}

void audio_fg_fade_out_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_fg_fadeout( fade );
  }
}



void audio_bgwav_play_command(int argc, char **argv)
{
  CHK_ARGC(3,3); // allow 2 arguments

  bool success = true;
  char* audio_file = NULL;

  // PARSE THE 1ST FILE ARGUMENT
  char parse_var_buf[10] = {0};
  char settings_bgwav_path[256] = {0};
  parse_variable_return_t bg_select = PARSE_VAR_RETURN_NOTHING;

  bg_select = parse_variable_from_string(argv[1], parse_var_buf, sizeof(parse_var_buf));

  // TODO: Do something with tokens
  token_t tokens;
  parse_dot_notation(argv[1], sizeof(argv[1]), &tokens);

  switch(bg_select){
  case PARSE_VAR_RETURN_NOTHING:
    success &= false;
    break;
  case PARSE_VAR_RETURN_ARG:
    success &= true;
    audio_file = argv[1];
    break;
  case PARSE_VAR_RETURN_SETTINGS_VALUE:
  {
    char *token_name = tokens.v[1];
    int settings_bgwav_select = atoi(parse_var_buf);
    char settings_bgwav_path_key[20] = {0};

    if (strcmp(token_name, "alarm.path.select") == 0)
    {
    	snprintf(settings_bgwav_path_key, sizeof(settings_bgwav_path_key), "alarm.path.%d", settings_bgwav_select);
    }
    else
    {
    	snprintf(settings_bgwav_path_key, sizeof(settings_bgwav_path_key), "bgwav.path.%d", settings_bgwav_select);
    }

    if ( 0 == settings_get_string(settings_bgwav_path_key, settings_bgwav_path, sizeof(settings_bgwav_path)) ){
      audio_file = settings_bgwav_path;
      success &= true;
    }else{
      LOGE(TAG,"Failed to get path to audio file.");
      success &= false;
    }
    break;
  }
  case PARSE_VAR_RETURN_SETTINGS_KEY:
    if ( 0 == settings_get_string("bgwav.path.default", settings_bgwav_path, sizeof(settings_bgwav_path)) ){
      audio_file = settings_bgwav_path;
      success &= true;
    }else{
      LOGE(TAG,"Failed to get path to audio file.");
      success &= false;
    }
    // get default value
    break;
  }

  // PARSE THE 2ND LOOP ARGUMENT
  uint8_t loop = 0;
  success &= parse_uint8_arg(argv[0], argv[2], &loop);

  if(success){
    audio_bgwav_play(audio_file, loop);
  }
}

void audio_bgwav_stop_command(int argc, char **argv)
{
  audio_bgwav_stop();
}

void audio_bg_fade_in_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_bg_fadein( fade );
  }
}

void audio_bg_fade_out_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_bg_fadeout( fade );
  }
}

void audio_bg_volume_command(int argc, char **argv){
  double volume = 0;
  if( parse_double_arg_min_max( argv[0], argv[1], 0, 1, &volume ) ){
    audio_bg_script_volume((float) volume);
  }
}


void audio_pink_play_command(int argc, char **argv)
{
  audio_pink_play();
}

void audio_pink_stop_command(int argc, char **argv)
{
  audio_pink_stop();
}

void audio_pink_fade_in_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_pink_fadein( fade );
  }
}

void audio_pink_fade_out_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_pink_fadeout( fade );
  }
}

void audio_pink_volume_command(int argc, char **argv){
  double volume = 0;
  if ( parse_double_arg_min_max( argv[0], argv[1], 0, 1, &volume )) {
    audio_pink_script_volume((float) volume);
  }
}


void audio_mp3_play_command(int argc, char **argv)
{
  CHK_ARGC(3,3);

  uint8_t loop = 0;
  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[2], &loop);

  if(success){
    audio_mp3_play(argv[1], loop);
  }
}

void audio_mp3_stop_command(int argc, char **argv)
{
  audio_mp3_stop();
}

void audio_mp3_fade_in_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_mp3_fadein( fade );
  }
}

void audio_mp3_fade_out_command(int argc, char **argv)
{
  CHK_ARGC(2,2); // allow 1 arguments

  uint32_t fade;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &fade);

  if(success){
    audio_mp3_fadeout( fade );
  }
}


void audio_play_test_command(int argc, char **argv)
{
  audio_sine_play();
}

void audio_stop_test_command(int argc, char **argv)
{
  audio_sine_stop();
}

void audio_info_command(int argc, char **argv)
{
	amp_print_detailed_status();
}

void erp_start_command(int argc, char **argv)
{
  CHK_ARGC(6,6); // allow 5 arguments

  // parse arguments:
  uint32_t num_trials = 0;
  double pulse_dur_ms = 0;
  double isi_ms = 0;
  double jitter_ms = 0;
  uint8_t volume = 0;
  bool success = true;
  success &= parse_uint32_arg(argv[0], argv[1], &num_trials);
  success &= parse_double_arg(argv[0], argv[2], &pulse_dur_ms);
  success &= parse_double_arg(argv[0], argv[3], &isi_ms);
  success &= parse_double_arg(argv[0], argv[4], &jitter_ms);
  success &= parse_uint8_arg(argv[0], argv[5], &volume);
  // num trials
  // pulse duration
  // isi
  // jitter
  // volume
  if (success){
    erp_event_start(num_trials,pulse_dur_ms,isi_ms,jitter_ms,volume);
  }
}

void erp_stop_command(int argc, char **argv)
{
  CHK_ARGC(1,1);

  erp_event_stop();
}
