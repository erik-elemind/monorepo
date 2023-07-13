This directory is a pruned copy of the Nordic nRF5_SDK ./examples/ directory.

A few examples were cherry-picked and kept here in git because they seemed 
relevant to coming development. The rest of the examples were not kept, 
because this SDK directory is so large (621 MB).

If you want to see more examples from the Nordic nRF5_SDK, download the
complete SDK zip file from here:

https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/nRF5_SDK_16.0.0_98a08e2.zip

Note that the Direct Test Mode app ./dtm/direct_test_mode/ is a special app that
may be necessary to run FCC tests or other RF signal strength tests on your device.

Here are the examples originally included in SDK v16:

```
examples
├── 802_15_4
│   └── wireless_uart
├── ant
│   ├── ant_advanced_burst
│   ├── ant_async_transmitter
│   ├── ant_background_scanning
│   ├── ant_broadcast
│   ├── ant_continuous_scanning_controller
│   ├── ant_cw_mode
│   ├── ant_debug
│   ├── ant_fs
│   ├── ant_hd_search_and_bs
│   ├── ant_io_demo
│   ├── ant_message_types
│   ├── ant_multi_channels
│   ├── ant_multi_channels_encrypted
│   ├── ant_plus
│   ├── ant_relay_demo
│   ├── ant_scan_and_forward
│   ├── ant_search_sharing
│   ├── ant_search_uplink
│   ├── ant_time_synchronization
│   └── experimental
├── ble_central
│   ├── ble_app_blinky_c
│   ├── ble_app_gatts
│   ├── ble_app_hrs_c
│   ├── ble_app_ias
│   ├── ble_app_ipsp_initiator
│   ├── ble_app_multilink_central
│   ├── ble_app_rscs_c
│   ├── ble_app_uart_c
│   └── experimental
├── ble_central_and_peripheral
│   └── experimental
├── ble_peripheral
│   ├── ble_app_alert_notification
│   ├── ble_app_ancs_c
│   ├── ble_app_beacon
│   ├── ble_app_blinky
│   ├── ble_app_bms
│   ├── ble_app_bps
│   ├── ble_app_buttonless_dfu
│   ├── ble_app_cscs
│   ├── ble_app_cts_c
│   ├── ble_app_eddystone
│   ├── ble_app_gatts_c
│   ├── ble_app_gls
│   ├── ble_app_hids_keyboard
│   ├── ble_app_hids_mouse
│   ├── ble_app_hrs
│   ├── ble_app_hrs_freertos
│   ├── ble_app_hts
│   ├── ble_app_ias_c
│   ├── ble_app_ipsp_acceptor
│   ├── ble_app_proximity
│   ├── ble_app_pwr_profiling
│   ├── ble_app_rscs
│   ├── ble_app_template
│   ├── ble_app_tile
│   ├── ble_app_uart
│   └── experimental
├── connectivity
│   ├── ble_connectivity
│   └── experimental_ant
├── crypto
│   ├── ifx_optiga_custom_example
│   ├── nrf_cc310
│   ├── nrf_cc310_bl
│   └── nrf_crypto
├── dfu
│   ├── open_bootloader
│   ├── secure_bootloader
│   └── secure_dfu_test_images
├── dtm
│   └── direct_test_mode
├── iot
│   ├── bootloader
│   ├── coap
│   ├── dns
│   ├── dtls
│   ├── icmp
│   ├── lwm2m
│   ├── misc
│   ├── mqtt
│   ├── sntp
│   ├── socket
│   ├── tcp
│   ├── tftp
│   └── udp
├── multiprotocol
│   ├── ble_ant_app_hrm
│   └── ble_app_gzll
├── nfc
│   ├── adafruit_tag_reader
│   ├── nfc_uart
│   ├── record_launch_app
│   ├── record_text
│   ├── record_url
│   ├── wake_on_nfc
│   └── writable_ndef_msg
├── peripheral
│   ├── blinky
│   ├── blinky_freertos
│   ├── blinky_rtc_freertos
│   ├── blinky_systick
│   ├── bsp
│   ├── cli
│   ├── cli_libuarte
│   ├── csense
│   ├── csense_drv
│   ├── fatfs
│   ├── flash_fds
│   ├── flash_fstorage
│   ├── flashwrite
│   ├── fpu_fft
│   ├── gfx
│   ├── gpiote
│   ├── i2s
│   ├── led_softblink
│   ├── libuarte
│   ├── low_power_pwm
│   ├── lpcomp
│   ├── nrfx_spim
│   ├── pin_change_int
│   ├── ppi
│   ├── preflash
│   ├── pwm_driver
│   ├── pwm_library
│   ├── pwr_mgmt
│   ├── qdec
│   ├── qspi
│   ├── qspi_bootloader
│   ├── radio
│   ├── radio_test
│   ├── ram_retention
│   ├── rng
│   ├── rtc
│   ├── saadc
│   ├── serial
│   ├── serial_uartes
│   ├── simple_timer
│   ├── spi
│   ├── spi_master_using_nrf_spi_mngr
│   ├── spis
│   ├── temperature
│   ├── template_project
│   ├── timer
│   ├── twi_master_using_nrf_twi_mngr
│   ├── twi_master_with_twis_slave
│   ├── twi_scanner
│   ├── twi_sensor
│   ├── uart
│   ├── uicr_config
│   ├── usbd
│   ├── usbd_audio
│   ├── usbd_ble_uart
│   ├── usbd_ble_uart_freertos
│   ├── usbd_cdc_acm
│   ├── usbd_hid_composite
│   ├── usbd_hid_generic
│   ├── usbd_msc
│   └── wdt
├── proprietary_rf
│   ├── esb_low_power_prx
│   ├── esb_low_power_ptx
│   ├── esb_prx
│   ├── esb_ptx
│   └── gzll
└── usb_drivers

170 directories
```
