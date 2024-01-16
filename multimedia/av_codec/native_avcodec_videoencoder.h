/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NATIVE_AVCODEC_VIDEOENCODER_H
#define NATIVE_AVCODEC_VIDEOENCODER_H

#include <stdint.h>
#include <stdio.h>
#include "native_avcodec_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a video encoder instance from the mime type, which is recommended in most cases.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param mime mime type description string, refer to {@link AVCODEC_MIME_TYPE}
 * @return Returns a Pointer to an OH_AVCodec instance
 * @since 9
 * @version 1.0
 */
OH_AVCodec *OH_VideoEncoder_CreateByMime(const char *mime);

/**
 * @brief Create a video encoder instance through the video encoder name. The premise of using this interface is to
 * know the exact name of the encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param name Video encoder name
 * @return Returns a Pointer to an OH_AVCodec instance
 * @since 9
 * @version 1.0
 */
OH_AVCodec *OH_VideoEncoder_CreateByName(const char *name);

/**
 * @brief Clear the internal resources of the encoder and destroy the encoder instance
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Destroy(OH_AVCodec *codec);

/**
 * @brief Set the asynchronous callback function so that your application can respond to the events generated by the
 * video encoder. This interface must be called before Prepare is called.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param callback A collection of all callback functions, see {@link OH_AVCodecAsyncCallback}
 * @param userData User specific data
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @deprecated since 11
 * @useinstead OH_VideoEncoder_RegisterCallback
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_SetCallback(OH_AVCodec *codec, OH_AVCodecAsyncCallback callback, void *userData);

/**
 * @brief Set the asynchronous callback function so that your application can respond to the events generated by the
 * video encoder. This interface must be called before Prepare is called.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param callback A collection of all callback functions, see {@link OH_AVCodecCallback}
 * @param userData User specific data
 * @return Returns AV_ERR_OK if the execution is successful, otherwise returns a specific error code, refer to {@link
 * OH_AVErrCode}
 * @since 11
 */
OH_AVErrCode OH_VideoEncoder_RegisterCallback(OH_AVCodec *codec, OH_AVCodecCallback callback, void *userData);

/**
 * @brief To configure the video encoder, typically, you need to configure the description information of the
 * encoded video track. This interface must be called before Prepare is called.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param format A pointer to an OH_AVFormat that gives the description of the video track to be encoded
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Configure(OH_AVCodec *codec, OH_AVFormat *format);

/**
 * @brief To prepare the internal resources of the encoder, the Configure interface must be called before
 * calling this interface.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Prepare(OH_AVCodec *codec);

/**
 * @brief Start the encoder, this interface must be called after the Prepare is successful. After being
 * successfully started, the encoder will start reporting NeedInputData events.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Start(OH_AVCodec *codec);

/**
 * @brief Stop the encoder. After stopping, you can re-enter the Started state through Start.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Stop(OH_AVCodec *codec);

/**
 * @brief Clear the input and output data buffered in the encoder. After this interface is called, all the Buffer
 * indexes previously reported through the asynchronous callback will be invalidated, make sure not to access the
 * Buffers corresponding to these indexes.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Flush(OH_AVCodec *codec);

/**
 * @brief Reset the encoder. To continue coding, you need to call the Configure interface again to
 * configure the encoder instance.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_Reset(OH_AVCodec *codec);

/**
 * @brief Get the description information of the output data of the encoder, refer to {@link OH_AVFormat} for details.
 * It should be noted that the life cycle of the OH_AVFormat instance pointed to by the return value * needs to
 * be manually released by the caller.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns a pointer to an OH_AVFormat instance
 * @since 9
 * @version 1.0
 */
OH_AVFormat *OH_VideoEncoder_GetOutputDescription(OH_AVCodec *codec);

/**
 * @brief Set dynamic parameters to the encoder. Note: This interface can only be called after the encoder is started.
 * At the same time, incorrect parameter settings may cause the encoding to fail.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param format OH_AVFormat handle pointer
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_SetParameter(OH_AVCodec *codec, OH_AVFormat *format);

/**
 * @brief Get the input Surface from the video encoder, this interface must be called before Prepare is called.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param window A pointer to a OHNativeWindow instance, see {@link OHNativeWindow}, the application is responsible for
 * managing the life cycle of the window, call OH_NativeWindow_DestroyNativeWindow() when done.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_GetSurface(OH_AVCodec *codec, OHNativeWindow **window);

/**
 * @brief Return the processed output Buffer to the encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param index The index value corresponding to the output Buffer
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @deprecated since 11
 * @useinstead OH_VideoEncoder_FreeOutputBuffer
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_FreeOutputData(OH_AVCodec *codec, uint32_t index);

/**
 * @brief Notifies the video encoder that the input stream has ended. It is recommended to use this interface to notify
 * the encoder of the end of the stream in surface mode
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 9
 * @version 1.0
 */
OH_AVErrCode OH_VideoEncoder_NotifyEndOfStream(OH_AVCodec *codec);

/**
 * @brief Submit the input buffer filled with data to the video encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param index Enter the index value corresponding to the Buffer
 * @param attr Information describing the data contained in the Buffer
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @deprecated since 11
 * @useinstead OH_VideoEncoder_PushInputBuffer
 * @since 10
 */
OH_AVErrCode OH_VideoEncoder_PushInputData(OH_AVCodec *codec, uint32_t index, OH_AVCodecBufferAttr attr);

/**
 * @brief Submit the input buffer filled with data to the video encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param index Enter the index value corresponding to the Buffer
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 11
 */
OH_AVErrCode OH_VideoEncoder_PushInputBuffer(OH_AVCodec *codec, uint32_t index);

/**
 * @brief Return the processed output Buffer to the encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param index The index value corresponding to the output Buffer
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 11
 */
OH_AVErrCode OH_VideoEncoder_FreeOutputBuffer(OH_AVCodec *codec, uint32_t index);

/**
 * @brief Get the input data description of the encoder after call {@OH_VideoEncoder_Configure},
 * refer to {@link OH_AVFormat} for details. It should be noted that the life cycle of the OH_AVFormat
 * instance pointed to by the return value needs to be manually released by the caller.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @return Returns a pointer to an OH_AVFormat instance
 * @since 10
 */
OH_AVFormat *OH_VideoEncoder_GetInputDescription(OH_AVCodec *codec);

/**
 * @brief Check whether the current codec instance is valid. It can be used fault recovery or app
 * switchback from the background
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec Pointer to an OH_AVCodec instance
 * @param isValid Output Parameter. A pointer to a boolean instance, it is true if the codec instance is valid,
 * false if the codec instance is invalid
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
 */
OH_AVErrCode OH_VideoEncoder_IsValid(OH_AVCodec *codec, bool *isValid);

/**
 * @brief The bitrate mode of video encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @since 9
 * @version 1.0
 */
typedef enum OH_VideoEncodeBitrateMode {
    /* constant bit rate mode. */
    CBR = 0,
    /* variable bit rate mode. */
    VBR = 1,
    /* constant quality mode. */
    CQ = 2,
} OH_VideoEncodeBitrateMode;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVCODEC_VIDEOENCODER_H