/**
 * @file 08_AI_Video.ino
 * @brief AI Video - capture JPEG frames from the camera-backed AI video pipeline
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 * @note Only supports TUYA_T5AI platform with a camera attached
 */
#include <Arduino.h>
#include "TuyaVideo.h"
#include "Log.h"
#include "board_com_api.h"

void setup()
{
    Serial.begin(115200);
    Log.begin();

    // Print startup banner
    PR_NOTICE("============ Tuya AI Video ==============");
    PR_NOTICE("Compile time:        %s", __DATE__);

    board_register_hardware();

    if (OPRT_OK != TuyaVideo.begin()) {
        PR_ERR("Failed to initialize AI video");
        while (1) {
            delay(1000);
        }
    }

    if (OPRT_OK != TuyaVideo.start()) {
        PR_ERR("Failed to start AI video");
        while (1) {
            delay(1000);
        }
    }
}

void loop()
{
    uint8_t *jpegData = nullptr;
    uint32_t jpegLen  = 0;

    if (OPRT_OK == TuyaVideo.getJpegFrame(&jpegData, &jpegLen)) {
        PR_NOTICE("JPEG frame length: %u", jpegLen);
    }

    TuyaVideo.freeJpegFrame(&jpegData);

    delay(100);
}
