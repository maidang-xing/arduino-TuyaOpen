/**
 * @file TuyaPicture.cpp
 * @brief TuyaPicture class implementation
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "TuyaPicture.h"
#include <string.h>

/***********************************************************
***********************class implementation*****************
***********************************************************/

// Global instance
TuyaPictureClass TuyaPicture;

TuyaPictureClass::TuyaPictureClass()
{
    _initialized = false;
}

TuyaPictureClass::~TuyaPictureClass() {}

OPERATE_RET TuyaPictureClass::begin()
{
    if (_initialized) {
        return OPRT_OK;
    }

    OPERATE_RET rt = ai_picture_init();
    if (rt != OPRT_OK) {
        return rt;
    }

    _initialized = true;
    return OPRT_OK;
}

void TuyaPictureClass::end()
{
    // Upstream ai_picture has no deinit symbol; just clear the flag.
    _initialized = false;
}

bool TuyaPictureClass::isInitialized()
{
    return _initialized;
}

const char *TuyaPictureClass::albumName()
{
    return ai_picture_get_album_name();
}

OPERATE_RET TuyaPictureClass::saveToAlbum(const uint8_t *data, uint32_t len, const char *hint, char *outName,
                                          uint32_t outNameSize)
{
    if (!_initialized || data == nullptr || len == 0) {
        return OPRT_INVALID_PARM;
    }

    // Upstream expects a fixed-size array of AI_PICTURE_NAME_MAX_LEN + 1
    // bytes and writes up to that size; passing a caller-supplied shorter
    // buffer directly would overflow it, so always use a local buffer here.
    char nameBuf[AI_PICTURE_NAME_MAX_LEN + 1] = {0};

    OPERATE_RET rt = ai_picture_save_to_album(const_cast<uint8_t *>(data), len, hint, nameBuf);
    if (rt != OPRT_OK) {
        return rt;
    }

    if (outName != nullptr && outNameSize > 0) {
        strncpy(outName, nameBuf, outNameSize - 1);
        outName[outNameSize - 1] = '\0';
    }

    return OPRT_OK;
}
