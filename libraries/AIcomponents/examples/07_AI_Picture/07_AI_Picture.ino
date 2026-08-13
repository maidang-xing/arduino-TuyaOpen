/**
 * @file 07_AI_Picture.ino
 * @brief Minimal on-device photo album (ai_picture) usage example
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 * @note Only supports TUYA_T5AI platform
 *
 * Features:
 * - Initialize the on-device photo album (TuyaPicture)
 * - Print the current album name
 *
 * This example does NOT capture a photo itself: a real caller would feed
 * JPEG bytes captured from the Camera library (or any other JPEG source)
 * into TuyaPicture.saveToAlbum(). Since this repo's Camera library API has
 * not been re-verified as part of this change, that capture step is left
 * as a comment instead of being fabricated here.
 */

#include <Arduino.h>
#include "TuyaPicture.h"

void setup()
{
    Serial.begin(115200);

    OPERATE_RET rt = TuyaPicture.begin();
    if (rt != OPRT_OK) {
        Serial.print("TuyaPicture.begin failed, rt=");
        Serial.println(rt);
        return;
    }

    Serial.print("Album name: ");
    Serial.println(TuyaPicture.albumName());

    // A real caller feeds JPEG bytes (e.g. captured from the Camera
    // library) into saveToAlbum(), for example:
    //
    //   char savedName[64] = {0};
    //   OPERATE_RET rt = TuyaPicture.saveToAlbum(jpegData, jpegLen, nullptr,
    //                                            savedName, sizeof(savedName));
}

void loop()
{
    delay(1000);
}
