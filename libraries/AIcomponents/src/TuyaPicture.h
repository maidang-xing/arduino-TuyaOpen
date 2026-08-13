/**
 * @file TuyaPicture.h
 * @brief TuyaPicture C++ wrapper for the on-device photo album (ai_picture)
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#ifndef __TUYA_PICTURE_H_
#define __TUYA_PICTURE_H_

// Upstream headers already self-guard with extern "C", matching TuyaAI.h
#include "tuya_cloud_types.h"
#include "ai_picture.h"

/***********************************************************
***********************class definition*********************
***********************************************************/

/**
 * @class TuyaPictureClass
 * @brief Wraps the upstream ai_picture component, which stores JPEG
 *        pictures into an on-device photo album (NOT a cloud upload).
 */
class TuyaPictureClass
{
public:
    TuyaPictureClass();
    ~TuyaPictureClass();

    /**
     * @brief Initialize the picture album module
     * @return OPRT_OK on success (also OPRT_OK if already initialized)
     */
    OPERATE_RET begin();

    /**
     * @brief Deinitialize the picture album module
     * @note Upstream ai_picture has no deinit symbol; this only clears
     *       the local initialized flag.
     */
    void end();

    /**
     * @brief Check if the picture album module is initialized
     * @return true if initialized
     */
    bool isInitialized();

    /**
     * @brief Get the name of the current album
     * @return Album name string
     */
    const char *albumName();

    /**
     * @brief Save a JPEG picture to the on-device album
     * @param data       JPEG data buffer
     * @param len        JPEG data length in bytes
     * @param hint       Desired filename; NULL to auto-generate a timestamp-based name
     * @param outName    Optional buffer to receive the actual filename used
     * @param outNameSize Size of outName buffer
     * @return OPRT_OK on success, OPRT_INVALID_PARM on bad arguments
     */
    OPERATE_RET saveToAlbum(const uint8_t *data, uint32_t len, const char *hint = nullptr, char *outName = nullptr,
                            uint32_t outNameSize = 0);

private:
    bool _initialized;
};

// Global instance
extern TuyaPictureClass TuyaPicture;

#endif /* __TUYA_PICTURE_H_ */
