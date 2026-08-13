/**
 * @file lang_config.h
 * @author Tuya Inc.
 * @brief Auto-generated language config (en-US) for Tuya AI components UI
 *
 * This header defines the English (en-US) display strings used by the
 * TuyaAI / ai_picture album UI. It is kept as a superset of upstream
 * TuyaOpen's src/ai_components/assets/include/lang_config.h so that no
 * upstream macro is ever missing here, regardless of include search order.
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __LANGUAGE_CONFIG_H__
#define __LANGUAGE_CONFIG_H__

#ifndef en_us
#define en_us
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LANG_CODE "en-US"

#define VERSION                      "Version "
#define INITIALIZING                 "Initializing..."
#define REGISTERING_NETWORK          "Waiting for network..."
#define CONNECT_SERVER               "Connecting to server..."
#define STANDBY                      "Standby"
#define CONNECT_TO                   "Connect to "
#define CONNECTING                   "Connecting..."
#define CONNECTED_TO                 "Connected to "
#define LISTENING                    "Listening..."
#define SPEAKING                     "Speaking..."
#define HOLD_TALK                    "Hold"
#define TRIG_TALK                    "Press"
#define WAKEUP_TALK                  "Wake up"
#define FREE_TALK                    "Free"
#define ENTERING_WIFI_CONFIG_MODE    "Entering Wi-Fi configuration mode..."
#define VOLUME                       "Volume "
#define MUTED                        "Muted"
#define MAX_VOLUME                   "Max Volume"
#define SYSTEM_MSG_POWER_ON          "Power on"
#define SYSTEM_MSG_WIFI_SSID         "WiFi connected"
#define SYSTEM_MSG_IP                "IP address is"
#define SYSTEM_MSG_WIFI_DISCONNECTED "WiFi disconnected"
#define SYSTEM_MSG_VOLUME            "Volume set to"

/* The macros below come from upstream's ai_picture / album UI
 * (src/ai_components/assets/include/lang_config.h). They are not used by
 * any existing example in this repo yet, but are kept here so this file
 * stays a superset of upstream and TuyaPicture-based sketches can rely on
 * them. Values are English translations of upstream's zh-CN strings. */
#define PROVISIONING  "Provisioning..."
#define UPLOADING     "Uploading..."
#define THINKING      "Thinking..."
#define VIEW_IMAGE    "View Image"
#define NO_IMAGE      "No Image"
#define TAKE_PHOTO    "Take Photo"
#define ADD_IMAGE     "Add Image"
#define PRINT_IMAGE   "Print Image"
#define PRINTING      "Printing..."
#define PRINT_SUCCESS "Print Success"
#define PRINT_FAILED  "Print Failed"
#define CAMERA        "Camera"
#define ALBUM         "Album"
#define CANCEL        "Cancel"
#define ALL_PHOTOS    "All Photos"
#define SELECT_TEXT   "Select"
#define DELETE_TEXT   "Delete"
#define CONFIRM_TEXT  "Confirm"
#define RECOGNIZE_IMAGE_PROMPT                                                                                         \
    "Please explain the content of the uploaded image. Do not trigger MCP skills or call MCP tools."

#ifdef __cplusplus
}
#endif

#endif // __LANGUAGE_CONFIG_H__
