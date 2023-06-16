#include <shell_recv.h>
#define CONFIG_SHELL_FREERTOS 1

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "loglevels.h"
#include "FreeRTOS.h"
#include "task.h"

#include "commands.h"
#include "command_parser.h"
#include "app_commands.h"
#include "audio_commands.h"
#include "ble_debug_commands.h"
#include "ble_uart_commands.h"
#include "ble_fs_commands.h"
#include "flash_commands.h"
#include "nand_commands.h"
#include "dhara_commands.h"
#include "fs_commands.h"
#include <fatfs_commands.h>
#include "lpc_commands.h"
#include "eeg_commands.h"
#include "data_log_commands.h"
#include "sleep_therapy_commands.h"
#include "datetime_commands.h"
#include "als_commands.h"
#include "led_commands.h"
#include "command_helpers.h"
#include "hrm_commands.h"
#include "charger_commands.h"
#include "accel_commands.h"
#include "dhara_ppstress.h"
#include "settings_commands.h"
#include "compression_commands.h"
#include "filehash_commands.h"
#include "memory_commands.h"
#include "mic_commands.h"
#include "als_commands.h"
#include "skin_temp_commands.h"
#include "pmic_commands.h"
#include "ml.h"
#include "memfault_commands.h"

//static const char *TAG = "commands";  // Logging prefix for this module

// This one needs to be defined after the declaration of shell_commands:
static void shell_list(int argc, char **argv);
static void shell_help(int argc, char **argv);


static void
log_level_command(int argc, char **argv) {
  uint8_t log_level;

  if (argc < 2) {
    printf(" Available log_levels:\r\n\r\n"
    "    NONE (0)\r\n"
    LOG_COLOR_E"   ERROR (1)\r\n"LOG_RESET_COLOR
    LOG_COLOR_W"    WARN (2)\r\n"LOG_RESET_COLOR
    LOG_COLOR_I"    INFO (3)\r\n"LOG_RESET_COLOR
    LOG_COLOR_D"   DEBUG (4)\r\n"LOG_RESET_COLOR
    LOG_COLOR_V" VERBOSE (5)\r\n"LOG_RESET_COLOR
    "\r\n Current log_level: %d\r\n", g_runtime_log_level);
    return;
  }

  bool success = true;
  success &= parse_uint8_arg(argv[0], argv[1], &log_level);

  if(success){
    if (log_level > LOG_VERBOSE) {
      printf("%s: log_level too high, must not exceed %d\r\n", argv[0], LOG_VERBOSE);
      return;
    }

    // Set the global variable in utils.c:
    taskENTER_CRITICAL();
    g_runtime_log_level = log_level;
    taskEXIT_CRITICAL();

    printf("%s: log_level now set to %d\r\n", argv[0], g_runtime_log_level);
  }

  return;
}

static void
version_command(int argc, char **argv)
{
  print_version();
}

static void
version_command_full(int argc, char **argv)
{
  print_version_full();
}


static void
reset_command(int argc, char **argv)
{
  NVIC_SystemReset();
}

const shell_command_t commands[] = {

    // Application-specific commands
    { P_ALL, "app_event", app_event_command, "Send event to app task"},
	{ P_ALL, "app_enter_isp_mode", app_enter_isp_mode, "Put device into ISP mode"},

#ifdef FLASH_COMMANDS_H
    // Flash test commands
    { P_ALL, "flash_hello", flash_hello, "Simple single-register read to test SPI Flash" },
    { P_ALL, "flash_id", flash_id, "Read SPI Flash ID register" },
    { P_ALL, "flash_status", flash_status, "Read SPI Flash ID status registers" },

    { P_ALL, "flash_unlock", flash_unlock, "Unlock SPI Flash ID for writes" },
    { P_ALL, "flash_mux_select", flash_mux_select, "Flash MUX select"},
    { P_ALL, "flash_read_page", flash_read_page, "Read SPI Flash page" },
    { P_ALL, "flash_write_page", flash_write_page, "Write SPI Flash page with test data" },
    { P_ALL, "flash_check_block", flash_check_block, "Check flash for a specific bad blocks" },
    { P_ALL, "flash_check_blocks", flash_check_blocks, "Check flash for bad blocks" },
    { P_ALL, "flash_check_blocks_ecc", flash_check_blocks_ecc, "Check flash for bad blocks using ECC readback. Optionally erase and re-write the Bad Block Mark" },
    { P_ALL, "flash_erase_block", flash_erase_block, "Erase flash block" },
    { P_ALL, "flash_erase_blocks", flash_erase_blocks, "Erase flash block" },
    { P_ALL, "flash_mark_bad_block", flash_mark_bad_block, "Marks flash block as bad" },
    { P_ALL, "flash_test_complete", flash_test_complete, "Run continuous flash test" },
    { P_ALL, "flash_test_speed", flash_test_speed, "Run flash speed test" },
    { P_ALL, "flash_test_spi", flash_test_spi, "Run continuous flash SPI-only test" },
#if (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))
    { P_ALL, "noise_test_flash_start", noise_test_flash_start_command, "Run periodic flash test. Takes 1 argument string of characters: 'r'/'w' cache read/writes, 'R'/'W' uffs read/writes" },
    { P_ALL, "noise_test_flash_stop", noise_test_flash_stop_command, "Stop flash test" },
#endif // (defined(ENABLE_NOISE_TEST) && (ENABLE_NOISE_TEST > 0U))
#endif // FLASH_COMMANDS_H

	{ P_ALL, "pmic_status", pmic_status_command, "" },

#ifdef NAND_COMMANDS_H
    { P_ALL, "nand_id", nand_id_command, "Read SPI Flash ID register" },
    { P_ALL, "nand_info", nand_info_command, "Read SPI Flash ID status registers" },
    { P_ALL, "nand_safe_wipe_all_blocks", nand_safe_wipe_all_blocks_command, "" },
    { P_ALL, "nand_read_all_blocks", nand_read_all_blocks_command, "" },
    { P_ALL, "nand_write_read", nand_write_read_command, "" },
    { P_ALL, "nand_copy_pages", nand_copy_pages_command, "" },
#endif // NAND_COMMANDS_H

#ifdef DHARA_COMMANDS_H
    { P_ALL, "dhara_write_read", dhara_write_read_command, "Perform a write/read test via dhara" },
    { P_ALL, "dhara_sync", dhara_sync_command, "Commit pending changes to disk" },
    { P_ALL, "dhara_clear", dhara_clear_command, "Delete all logical sectors" },
#endif // DHARA_COMMANDS_H

#if (defined(ENABLE_DHARA_PPSTRESS) && (ENABLE_DHARA_PPSTRESS > 0U))
    { P_ALL, "dhara_stat", dhara_cmd_stat, "Show dhara map/journal status" },
    { P_ALL, "dhara_ppstress", dhara_cmd_ppstress, "Run FTL stress test" },
    { P_ALL, "dhara_ppnand", dhara_cmd_ppnand, "Run FTL nand driver test" },
    { P_ALL, "dhara_bbmap", dhara_cmd_bbmap, "List bad block map" },
    { P_ALL, "dhara_blkerase", dhara_cmd_blkerase, "Block erase" },
    { P_ALL, "dhara_blkmark", dhara_cmd_blkmark, "Mark bad blocks" },
    { P_ALL, "dhara_eraseall", dhara_cmd_eraseall, "Erase all blocks (unsafe!)" },
#endif // ENABLE_DHARA_PPSTRESS

#ifdef FATFS_COMMANDS_H
    // From fatfs_commandsc:
    { P_ALL, "format_disk", format_disk_command, "" },
    { P_ALL, "filetest", filetest_command, "" },
    { P_ALL, "ls", ls_command, "" },
    { P_ALL, "rm", rm_command, "" },
    { P_ALL, "cat", cat_command, "" },
    { P_ALL, "head", head_command, "" },
    //    {"ymodem", ymodem_command},
#endif // FATFS_COMMANDS_H

    // EEG commands:
    { P_ALL, "eeg_on", eeg_on_command, "Turn on the EEG" },
    { P_ALL, "eeg_off", eeg_off_command, "Turn off the EEG" },
    { P_ALL, "eeg_start", eeg_start_command, "Start the EEG sampling" },
    { P_ALL, "eeg_stop", eeg_stop_command, "Stop the EEG sampling" },
    { P_ALL, "eeg_gain", eeg_gain_command, "Set EEG Gain, args: eeg_gain(1,2,3,4,6,8,12)" },
    { P_ALL, "eeg_info", eeg_info_command, "Print out verbose log-level info on the EEG to debug console" },
    { P_ALL, "eeg_sqw", eeg_sqw_command, "EEG square wave, args: freq (0-100) duty cycle (0-100)" },
    { P_ALL, "eeg_gain?", eeg_get_gain_command, "Prints the EEG gain." },

    // Heart rate monitor commands:
    { P_ALL, "hrm_off", hrm_off, "Turn off heart rate monitor" },
    { P_ALL, "hrm_on", hrm_on, "Turn on heart rate monitor" },

	// ML commands
	{ P_ALL, "ml_enable", ml_enable, "Enables ML inference" },
	{ P_ALL, "ml_disable", ml_disable, "Disables ML inference" },
	{ P_ALL, "ml_stop", ml_event_stop, "Resets ML state machine"},

    // Accel commands:
    { P_ALL, "accel_start_sample", accel_start_sample_command, "Run the accel in sample mode" },
    { P_ALL, "accel_start_motion", accel_start_motion_detect_command, "Run the accel in motion-detection mode" },
    { P_ALL, "accel_stop", accel_stop_command, "Turn off the accelerometer" },

    // Ambient light sensor commands:
    { P_ALL, "als", als_read_once_command, "Read Ambient light sensor lux" },
    { P_ALL, "als_start_sample", als_start_sample_command, ""},
    { P_ALL, "als_stop", als_stop_command, ""},

    // MEMS commands
    { P_ALL, "mic", mic_read_once_command, "Read mems microphone" },
    { P_ALL, "mic_start_sample", mic_start_sample_command, ""},
    { P_ALL, "mic_start_thresh", mic_start_thresh_command, ""},
    { P_ALL, "mic_stop", mic_stop_command, ""},

    // Skin Temp commands
    { P_ALL, "skin_temp_start_sample", skin_temp_start_sample_command, ""},
    { P_ALL, "skin_temp_stop", skin_temp_stop_command, ""},

    // LED animations
    { P_ALL, "led_state", led_state_command, "Set LED state <state pattern [0-11]> " },

    // Sleep therapy commands:
    { P_ALL, "therapy_start", therapy_start_command, "Start sleep therapy" },
    { P_ALL, "therapy_stop", therapy_stop_command, "Stop sleep therapy" },
    { P_ALL, "therapy_enable_line_filters", therapy_enable_line_filters_command, "Enable line filter for all EEG channels."},
    { P_ALL, "therapy_enable_az_filters"  , therapy_enable_az_filters_command  , "Enable autozero filter for all EEG channels."},
    { P_ALL, "therapy_config_line_filters", therapy_config_line_filters_command, "configure line filter, args: order, cutfreq" },
    { P_ALL, "therapy_config_az_filters"  , therapy_config_az_filters_command  , "configure autozero filter, args: order, cutfreq"},
    { P_BLE | P_SHELL, "therapy_start_script", therapy_start_script_command, "Start running the therapy file from \"scripts\" directory args: name-of-file"},
    { P_BLE | P_SHELL, "therapy_stop_script", therapy_stop_script_command, "Stop running the therapy, if there is a therapy running, otherwise does nothing."},
    { P_ALL, "therapy_delay", therapy_delay_command, "Pause script execution until time expires, args: time-in-ms."},
    { P_ALL, "therapy_start_timer1", therapy_start_timer1_command, "Start timer1, args: time-in-ms."},
    { P_ALL, "therapy_wait_timer1", therapy_wait_timer1_command, "Pause script execution until timer expires, no-args."},
    { P_ALL, "therapy_enable_alpha_switch", therapy_config_enable_switch_command, "Enable alpha switch, args: enable_boolean (0=disable or 1=enable)."},
    { P_ALL, "therapy_config_alpha_switch", therapy_config_alpha_switch_command, "Configure alpha switch, args:  dur1_sec, dur2_sec, timedLockOut_sec, minRMSPower_uV, alphaThr_dB."},

    { P_ALL, "echt_config", echt_config_command, "NOT YET IMPLEMENTED."},
    { P_ALL, "echt_config_simple", echt_config_simple_command, "Configure echt with center frequency, args: center_freq_hz."},
    { P_ALL, "echt_set_channel", echt_set_channel_command, "Sets the EEG channel used by echt args: channel-int(0-7)."},
    { P_ALL, "echt_set_min_max_phase", echt_set_min_max_phase_command, "Sets the min-max phase used for stimulation, args: min_phase_deg, max_phase_deg."},
    { P_ALL, "echt_start", echt_start_command, "Start computing ECHT and modulating pink noise."},
    { P_ALL, "echt_stop", echt_stop_command, "Stop computing ECHT."},

    // Filesystem commands:
    { P_ALL, "fs_write", fs_write_command, "FS write test" },
    { P_ALL, "fs_read", fs_read_command, "FS read test" },
    { P_ALL, "fs_format", fs_format_command, "Format FS flash" },
    { P_ALL, "fs_touch", fs_touch_command, "touch <filename>" },
    { P_ALL, "fs_mkdir", fs_mkdir_command, "mkdir <filename>" },
    { P_ALL, "fs_ls", fs_ls_command, "ls <path>" },
    { P_ALL, "fs_rm", fs_rm_command, "rm <path>" },
    { P_ALL, "fs_rm_datalogs", fs_rm_datalogs_command, "Removes all datalogs" },
    { P_ALL, "fs_rm_all", fs_rm_all_command, "Removes a file or a folder, or removes all contents of root if no argument is provided, args: <path>" },
    { P_ALL, "fs_mv", fs_mv_command, "mv <oldpath> <newpath>" },
    { P_ALL, "fs_cp", fs_cp_command, "cp <oldpath> <newpath>" },
    { P_ALL, "fs_cat", fs_cat_command, "cat <filename>" },
    { P_ALL, "fs_info", fs_info_command, "Show filesystem info" },
    { P_ALL, "fs_ymodem_recv", fs_ymodem_recv_command, "Receive file via ymodem" },
    { P_ALL, "fs_ymodem_send", fs_ymodem_send_command, "Send file via ymodem" },
    { P_ALL, "fs_ymodem_recv_test", fs_ymodem_recv_test_command, "Ymodem expects a file with 'abc...z', 100 times." },
    { P_ALL, "fs_ymodem_send_test", fs_ymodem_send_test_command, "Ymodem sends a file with 'abc...z', 100 times." },
    { P_ALL, "fs_zmodem_recv", fs_zmodem_recv_command, "Receive file via Zmodem" },
    { P_ALL, "fs_zmodem_send", fs_zmodem_send_command, "Send file via Zmodem" },
    { P_ALL, "fs_zmodem_send_test", fs_zmodem_send_test_command, "Zmodem sends a file with 'abc...z', 100 times." },
    { P_ALL, "fs_zmodem_recv_test", fs_zmodem_recv_test_command, "Zmodem expects a file with 'abc...z', 100 times." },
#if (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))
    { P_ALL, "fs_test_speed", fs_test_speed_command, "Test UFFS speed" },
    { P_ALL, "fs_test_mixedrw_speed", fs_test_mixedrw_speed_command, "Test UFFS speed with interleaved R/W" },
    { P_ALL, "fs_test_filling_write", fs_test_filling_write_read_command, "Test UFFS 2 filling writes with read verification." },
#endif // (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))
    { P_ALL, "filehash_sha256", filehash_sha256_command, "print sha256 hash (32 characters) for the specific file or print 'error #', 1 arg: [filepath]"},

    // Filesystem commands:
    // These duplicate the commands above and are preserved
    // for backwards compatibility with the Morpheus Controller tool.
    // Once the tool is updated, the following can be removed.
    { P_ALL, "uffs_write", fs_write_command, "FS write test" },
    { P_ALL, "uffs_read", fs_read_command, "FS read test" },
    { P_ALL, "uffs_format", fs_format_command, "Format FS flash" },
    { P_ALL, "uffs_touch", fs_touch_command, "touch <filename>" },
    { P_ALL, "uffs_mkdir", fs_mkdir_command, "mkdir <filename>" },
    { P_ALL, "uffs_ls", fs_ls_command, "ls <path>" },
    { P_ALL, "uffs_rm", fs_rm_command, "rm <path>" },
    { P_ALL, "uffs_rm_datalogs", fs_rm_datalogs_command, "Removes all datalogs" },
    { P_ALL, "uffs_mv", fs_mv_command, "mv <oldpath> <newpath>" },
    { P_ALL, "uffs_cp", fs_cp_command, "cp <oldpath> <newpath>" },
    { P_ALL, "uffs_cat", fs_cat_command, "cat <filename>" },
    { P_ALL, "uffs_info", fs_info_command, "Show filesystem info" },
    { P_ALL, "uffs_ymodem_recv", fs_ymodem_recv_command, "Receive file via ymodem" },
    { P_ALL, "uffs_ymodem_send", fs_ymodem_send_command, "Send file via ymodem" },
    { P_ALL, "uffs_ymodem_recv_test", fs_ymodem_recv_test_command, "Ymodem expects a file with 'abc...z', 100 times." },
    { P_ALL, "uffs_ymodem_send_test", fs_ymodem_send_test_command, "Ymodem sends a file with 'abc...z', 100 times." },

    // Data log commands:
    { P_ALL, "data_log_open" , data_log_open_command, "Open data log" },
    { P_ALL, "data_log_close", data_log_close_command, "Close data log" },
    { P_ALL, "user_metrics_log_open", user_metrics_log_open_command, "Open user metrics log" },
    { P_ALL, "user_metrics_log_close", user_metrics_log_close_command, "Close user metrics log" },
#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))
    { P_ALL, "data_log_compress", data_log_compress_command, "Compress existing datalog file. Takes filename of existing data log." },
    { P_ALL, "data_log_compress_status", data_log_compress_status_command, "" },
    { P_ALL, "data_log_compress_abort", data_log_compress_abort_command, "" },
#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

    // Audio commands:
    { P_ALL, "audio_power_on",   audio_power_on_command, "Power on and initialize the audio driver." },
    { P_ALL, "audio_power_off",  audio_power_off_command, "Power off the audio driver." },
    { P_ALL, "audio_pause",      audio_pause, "Pause playing all audio" },
    { P_ALL, "audio_unpause",    audio_unpause, "Unpause playing all audio" },
    { P_ALL, "audio_stop",       audio_stop_command, "Stop playing all audio" },
    { P_ALL, "audio_set_volume", audio_set_volume_command, "Set audio volume, args: volume_int(0-255)" },
    { P_ALL, "audio_inc_volume", audio_inc_volume_command, "Increase audio volume" },
    { P_ALL, "audio_dec_volume", audio_dec_volume_command, "Decrease audio volume" },
    { P_ALL, "audio_mute",       audio_mute_command, "Mute audio" },
    { P_ALL, "audio_unmute",     audio_unmute_command, "Unmute audio" },

    { P_ALL, "audio_fg_fade_in",  audio_fg_fade_in_command, "Fade in foreground, args: duration_ms_int" },
    { P_ALL, "audio_fg_fade_out", audio_fg_fade_out_command, "Fade out foreground, args: duration_ms_int" },

    { P_ALL, "audio_bgwav_play",  audio_bgwav_play_command, "Play background wav file, args: filename_string loop_bool_0/1" },
    { P_ALL, "audio_bgwav_stop",  audio_bgwav_stop_command, "Stop background wav file." },
    { P_ALL, "audio_bg_fade_in",  audio_bg_fade_in_command, "Fade in background, args: duration_ms_int" },
    { P_ALL, "audio_bg_fade_out", audio_bg_fade_out_command, "Fade out background, args: duration_ms_int" },
    { P_ALL, "audio_bg_volume",   audio_bg_volume_command, "background audio volume" },

    { P_ALL, "audio_mp3_play", audio_mp3_play_command, "Play test mp3 <filename> loop_bool_0/1" },
    { P_ALL, "audio_mp3_stop", audio_mp3_stop_command, "Stop mp3 playback" },
    { P_ALL, "audio_mp3_fade_in",  audio_mp3_fade_in_command, "Fade in mp3, args: duration_ms_int" },
    { P_ALL, "audio_mp3_fade_out", audio_mp3_fade_out_command, "Fade out mp3, args: duration_ms_int" },

    { P_ALL, "audio_pink_play",     audio_pink_play_command, "Play pink audio" },
    { P_ALL, "audio_pink_stop",     audio_pink_stop_command, "Stop pink audio" },
    { P_ALL, "audio_pink_fade_in",  audio_pink_fade_in_command, "Fade in pink noise, args: duration_ms_int" },
    { P_ALL, "audio_pink_fade_out", audio_pink_fade_out_command, "Fade out pink noise, args: duration_ms_int" },
    { P_ALL, "audio_pink_mute",     audio_pink_mute_command, "Mute pink audio" },
    { P_ALL, "audio_pink_unmute",   audio_pink_unmute_command, "Unmute pink audio" },
    { P_ALL, "audio_pink_volume",   audio_pink_volume_command, "Pink audio volume" },

    { P_ALL, "audio_play_test", audio_play_test_command, "Play test sine wave, right channel has higher freq than left." },
    { P_ALL, "audio_stop_test", audio_stop_test_command, "Stop test sine wave" },

//    audio_set_bg_volume

    // Audio ERP Test
    { P_ALL, "erp_start", erp_start_command, "Start the audio ERP experiment, 5 args: <num_trials> <pulse_dur_ms> <isi_ms> <jitter_ms> <volume>"},
    { P_ALL, "erp_stop", erp_stop_command, "Stop the audio ERP experiment."},

    // Included commands:
    { P_ALL, "help", shell_help, .description = "List commands with descriptions"  },
    { P_ALL, "?", shell_list, "List commands" },

    { P_ALL, "log_level", log_level_command, "Display or set the log level squelching (ERROR, WARN, INFO, etc.)" },
    { P_ALL, "version", version_command, "Print out the current firmware version" },
	{ P_ALL, "version_full", version_command_full, "Print out all details of the current firmware version" },
    { P_ALL, "reset", reset_command, "Perform a software reset." },

    // Platform commands
    { P_ALL, "memfree", memfree, "Display free memory" },
    { P_ALL, "i2c_sensor_scan", i2c_sensor_scan, "Scan the Flexcomm (accel, als, audio, hrm) I2C bus. Prints address and state code of active devices." },
    { P_ALL, "i2c_batt_scan", i2c_batt_scan, "Scan the Flexcomm (batt) I2C bus. Prints address and state code of active devices." },
    { P_ALL, "i2c_sensor_read_byte", i2c_sensor_read_byte, "Read byte from I2C device on Flexcomm (accel, als, audio, hrm) bus " },
    { P_ALL, "i2c_sensor_write_byte", i2c_sensor_write_byte, "Write byte to I2C device on Flexcomm (accel, als, audio, hrm) bus" },
    { P_ALL, "i2c_batt_read_byte", i2c_batt_read_byte, "Read byte from I2C device on Flexcomm (batt) bus" },
    { P_ALL, "i2c_batt_write_byte", i2c_batt_write_byte, "Write byte to I2C device on Flexcomm (batt) bus" },
    { P_ALL, "bq_charge_enable", bq_charge_enable, "Enable battery charging on BQ25887 charge controller" },
    { P_ALL, "bq_charge_disable", bq_charge_disable, "Disable battery charging on BQ25887 charge controller" },
    { P_ALL, "bq_status", bq_status, "Get battery charging status from BQ25887 charge controller" },
	{ P_ALL, "bq_adc_enable", bq_adc_enable, "Set BQ25887 ADC on or off, used to read voltages in bq_status" },
    { P_ALL, "hrm_off", hrm_off, "Turn on heart rate monitor" },
    { P_ALL, "hrm_on", hrm_on, "Turn off heart rate monitor" },
    { P_ALL, "gpio_read", gpio_read, "Read GPIO pin (pin must already be configured as input)" },
    { P_ALL, "gpio_write", gpio_write, "Write GPIO pin (pin must already be configured as output)" },
    { P_ALL, "power_off", power_off_command, "Power off the LPC55S69 processor." },

    // DateTime commands
    { P_ALL, "datetime_update", datetime_update_command, "Set the datetime, 1 argument: num of seconds since Unix epoch."},
    { P_ALL, "datetime_get", datetime_get_command, "Get the datetime"},
    { P_ALL, "datetime_string_update", datetime_human_update_command, "Human readable date-time string, in the format yyyy_MM_dd_HH_mm_ss_z."},

    // Settings commands
    { P_ALL, "settings_get", settings_get_command, "Get a setting. args: key. If no key is specified, it prints all."},
    { P_ALL, "settings_set", settings_set_command, "Set a setting. args: key, value"},
    { P_ALL, "settings_del", settings_delete_command, "Delete a setting. args: key"},

    // BLE commands
    { P_ALL, "ble_ping", ble_ping_command, "Device will respond with 4 character \"pong\" followed by a newline character."},

    { P_ALL, "ble_reset", ble_reset_debug_command, "Reset BLE chip (no DFU)" },
    { P_ALL, "ble_dfu", ble_dfu_debug_command, "Put BLE chip into DFU mode" },
    { P_ALL, "ble_debug_electrode_quality", ble_electrode_quality_debug_command, "Send electrode quality update to BLE chip, 8 numbers 0-255" },
    { P_ALL, "ble_debug_blink_status", ble_blink_status_debug_command, "Send blink status update to BLE chip, 10 numbers 0-2" },
    { P_ALL, "ble_debug_volume", ble_volume_debug_command, "Send volume update to BLE chip (doesn't change actual volume)" },
    { P_ALL, "ble_debug_power", ble_power_debug_command, "Send power state update to BLE chip " "(doesn't change actual power state)" },
    { P_ALL, "ble_debug_therapy", ble_therapy_debug_command, "Send therapy state update to BLE chip " "(doesn't change actual therapy state)" },
    { P_ALL, "ble_debug_heart_rate", ble_heart_rate_debug_command, "Send heart rate update to BLE chip " },
    { P_ALL, "ble_debug_battery_level", ble_battery_level_debug_command, "Send battery_level update to BLE chip " },
    { P_ALL, "ble_debug_quality_check", ble_quality_check_debug_command, "Send quality check update to BLE chip" },
    { P_ALL, "ble_debug_alarm_check", ble_alarm_debug_command, "Send alarm update to BLE chip, all args are 0 or 1, except arg 9 which is an unsigned integer, 9args: [on][sun][mon][tue][wed][thu][fri][sat][seconds-after-midnight]" },
    { P_ALL, "ble_debug_sound_check", ble_sound_debug_command, "Send sound update to BLE chip, 1 arg: [0/1/2]" },
    { P_ALL, "ble_debug_time_check", ble_time_debug_command, "Send time update to BLE chip, 1 arg: [uint64:unix-epoch-sec]" },
	{ P_ALL, "ble_debug_charger_status", ble_charger_status_debug_command, "Send charger status update to BLE chip, 1 number 0-3" },
	{ P_ALL, "ble_debug_settings", ble_settings_debug_command, "Send settings update to BLE chip " "(doesn't change actual settings)" },
	{ P_ALL, "ble_debug_memory_level", ble_memory_level_debug_command, "Send memory level update to BLE chip, 1 number 0-2 " },
	{ P_ALL, "ble_debug_factory_reset", ble_factory_reset_debug_command, "Send factory reset to BLE chip, 1 numbers 0-2" },
	{ P_ALL, "ble_debug_sound_control", ble_sound_control_debug_command, "Send sound control update to BLE chip, 1 number 0-2" },

	{ P_BLE, "ble_battery_level?", battery_level_request, ""},
    { P_BLE, "ble_serial_number?", serial_number_request, ""},
    { P_BLE, "ble_software_version?", software_version_request, ""},
    { P_BLE, "ble_electrode_quality?", electrode_quality_request, ""},
    { P_BLE, "ble_volume?", volume_request, ""},
    { P_BLE, "ble_volume", volume_command, ""},
    { P_BLE, "ble_power?", power_request, ""},
    { P_BLE, "ble_power", power_command, ""},
    { P_BLE, "ble_therapy?", therapy_request, ""},
    { P_BLE, "ble_therapy", therapy_command, ""},
    { P_BLE, "ble_heart_rate?", heart_rate_request, ""},
    { P_BLE, "ble_quality_check?", quality_check_request, ""},
    { P_BLE, "ble_quality_check", quality_check_command, ""},
    { P_BLE, "ble_alarm?", alarm_request, ""},
    { P_BLE, "ble_alarm" , alarm_command, ""},
    { P_BLE, "ble_sound?", sound_request, ""},
    { P_BLE, "ble_sound" , sound_command, ""},
    { P_BLE, "ble_time?" , time_request, ""},
    { P_BLE, "ble_time"  , time_command, ""},
    { P_BLE, "ble_addr"  , addr_command, ""},
    { P_ALL, "ble_print_addr", ble_print_addr_command, "Prints the BLE address"},
	{ P_BLE, "ble_charger_status?", charger_status_request, ""},
	{ P_BLE, "ble_settings?", settings_request, ""},
	{ P_BLE, "ble_settings" , settings_command, ""},
	{ P_BLE, "ble_memory_level?", memory_level_request, ""},
	{ P_BLE, "ble_factory_reset?", factory_reset_request, ""},
	{ P_BLE, "ble_factory_reset" , factory_reset_command, ""},
	{ P_BLE, "ble_sound_control?", sound_control_request, ""},
	{ P_BLE, "ble_sound_control" , sound_control_command, ""},

    { P_BLE, "ble_fs_ls", ble_fs_ls_command, "ls <path>" },
    { P_BLE, "ble_fs_rm", ble_fs_rm_command, "rm <path>" },
    { P_BLE, "ble_fs_mkdir", ble_fs_mkdir_command, "mkdir <filename>" },
    { P_BLE, "ble_fs_mv", ble_fs_mv_command, "mv <oldpath> <newpath>" },
    { P_BLE, "ble_fs_ymodem_recv", ble_fs_ymodem_recv_command, "Receive file via ymodem" },
    { P_BLE, "ble_fs_ymodem_send", ble_fs_ymodem_send_command, "Send file via ymodem" },
    { P_BLE, "ble_filehash_sha256", ble_filehash_sha256_command, "print sha256 hash (32 characters) for the specific file or print 'error #', 1 arg: [filepath]"},

    { P_BLE, "ble_uffs_ls", ble_fs_ls_command, "(DEPRECATED) ls <path>" },
    { P_BLE, "ble_uffs_rm", ble_fs_rm_command, "(DEPRECATED) rm <path>" },
    { P_BLE, "ble_uffs_mkdir", ble_fs_mkdir_command, "(DEPRECATED) mkdir <filename>" },
    { P_BLE, "ble_uffs_mv", ble_fs_mv_command, "(DEPRECATED) mv <oldpath> <newpath>" },
    { P_BLE, "ble_uffs_ymodem_recv", ble_fs_ymodem_recv_command, "(DEPRECATED) Receive file via ymodem" },
    { P_BLE, "ble_uffs_ymodem_send", ble_fs_ymodem_send_command, "(DEPRECATED) Send file via ymodem" },

	{ P_BLE, "ble_connected", ble_connected, ""},
	{ P_BLE, "ble_disconnected", ble_disconnected, ""},
	{ P_BLE, "ble_ota_start", ble_ota_started, ""},

	// Memfault Tests
	{P_SHELL, "memfault_test_logging", memfault_test_logging_command, "Memfault Logging Test"},
    {P_SHELL, "memfault_info_dump", memfault_info_dump_command, "Memfault Build and Device Info Dump"},
	{P_SHELL, "memfault_test_export", memfault_test_export_command, "Memfault Export Test"},
    {P_SHELL, "memfault_test_coredump_storage", memfault_test_coredump_storage_command, "Memfault Coredump Storage Test"},
	{P_SHELL, "memfault_test_heartbeat", memfault_test_heartbeat_command, "Memfault Heartbeat Test"},
	{P_SHELL, "memfault_test_trace", memfault_test_trace_command, "Memfault Trace Test"},
	{P_SHELL, "memfault_test_reboot", memfault_test_reboot_command, "Memfault Reboot Test"},
	{P_SHELL, "memfault_test_assert", memfault_test_assert_command, "Memfault Assert Test"},
	{P_SHELL, "memfault_test_fault", memfault_test_fault_command, "Memfault Fault Test"},
	{P_SHELL, "memfault_test_hang", memfault_test_hang_command, "Memfault Hang Test"},
	{P_SHELL, "memfault_check_coredump", memfault_check_coredump, "Memfault Check Coredump Validity"},
	{P_SHELL, "memfault_check_coredump_nand", memfault_check_coredump_nand, "Memfault Check coredump NAND"},
	{P_SHELL, "memfault_test_coredump_nand_write", memfault_test_coredump_nand_write, "Memfault Test coredump NAND write"},
	{P_SHELL, "memfault_test_coredump_nand_erase", memfault_test_coredump_nand_erase, "Memfault Test coredump NAND erase"},


    // Misc low level tests
#if (defined(ENABLE_STREAM_MEMORY_TEST_COMMANDS) && (ENABLE_STREAM_MEMORY_TEST_COMMANDS > 0U))
    { P_ALL, "stream_memory_test", stream_memory_test_command, "Runs Test for Streaming Memory" },
#endif

#if (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))
    { P_ALL, "parser_test", parse_double_quote_test_command, "" },
#endif

#if (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
    { P_ALL, "parse_dot_notation_test", parse_dot_notation_test_command, "" },
#endif

#if (defined(ENABLE_COMPRESSION_TEST_TASK) && (ENABLE_COMPRESSION_TEST_TASK > 0U))
    { P_ALL, "comp_test1", comp_test1_command, "Runs the original compression algorithm implementation."},
    { P_ALL, "comp_test2", comp_test2_command, "Runs a comparison between original and new cdt97 transform."},
    { P_ALL, "comp_test3", comp_test3_command, "Runs the new compression algorithm implementation."},
#endif // (defined(ENABLE_COMPRESSION_TEST_TASK) && (ENABLE_COMPRESSION_TEST_TASK > 0U))
#if (defined(ENABLE_COMPRESSION_TEST) && (ENABLE_COMPRESSION_TEST > 0U))
    { P_ALL, "comp_test4", comp_test4_command, "Runs the original compression algorithm implementation in the shell task."},
#endif // (defined(ENABLE_COMPRESSION_TEST) && (ENABLE_COMPRESSION_TEST > 0U))

#if 0
    { P_ALL, "gpioq", gpioq_command, "Probe GPIO register state" },
    { P_ALL, "spiq", spiq_command, "Probe SPI register state" },
#endif

};

// Check number of commands
static_assert(ARRAY_SIZE(commands) <= MAX_COMMANDS,
  "Too many commands--increase MAX_COMMANDS");

// Export the length of the array to other modules, especially shell.c:
const int commands_count = ARRAY_SIZE(commands);


static void
shell_list(int argc, char **argv)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(commands); i++) {
    printf("%s\n", commands[i].name);
  }
}

static void
shell_help(int argc, char **argv)
{
  unsigned int i;
  char buffer[CMD_BUFFER_LEN];

  for (i = 0; i < ARRAY_SIZE(commands); i++) {
    snprintf(buffer, sizeof(buffer), "%10s: %s\n", commands[i].name, commands[i].description);
    printf(buffer);
  }
}
