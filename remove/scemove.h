/*
    a very baremetal libSceMove header file that can be used in any SDK without issues
    doesn't contain any dependencies, just needs either C99 or C++11 at least...
*/

#pragma once
#ifndef _SCEMOVE_H_
#define _SCEMOVE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    unsigned 8 bit integer value
*/
typedef unsigned char sceByte;

/*
    unsigned 16 bit integer value
*/
typedef unsigned short sceUInt16;

/*
    signed two's complement 32 bit integer value
*/
typedef int sceInt32;

/*
    signed two's complement 64 bit integer value
*/
typedef long long sceInt64;

/*
    signed 32 bit IEEE 754 float type
*/
typedef float sceFloat;

/*
    SCE Device Handle.
    >= 0 - valid handle
    <  0 - invalid handle (sceError)
*/
typedef sceInt32 sceHandle;

/*
    Either a
        handle (>=0, sceHandle)
    or
        an error (< 0, sceError).
*/
typedef sceInt32 sceHandleOrError;

/*
    SCE User Id.
    >= 0 - valid user id
    <  0 - invalid user id (usually also an sceError)

    Known constants, not used by sceMove:
    0xFF - SYSTEM user, used when a shared peripheral is specified, e.g. Camera.
    0xFE - EVERYONE user, only used in dialogs and IME.
*/
typedef sceInt32 sceUserId;

typedef enum sceMoveDeviceType {
    sceMoveDeviceTypeStandard       = 0,
    sceMoveDeviceTypeForceInt32     = -2147483648
} sceMoveDeviceType;

typedef enum sceMoveDigitalButton {
    sceMoveDigitalButtonSelect      = (1 << 0),
    sceMoveDigitalButtonT           = (1 << 1),
    sceMoveDigitalButtonMove        = (1 << 2),
    sceMoveDigitalButtonStart       = (1 << 3),
    sceMoveDigitalButtonTriangle    = (1 << 4),
    sceMoveDigitalButtonCircle      = (1 << 5),
    sceMoveDigitalButtonCross       = (1 << 6),
    sceMoveDigitalButtonSquare      = (1 << 7),
    sceMoveDigitalButtonAll         =
        (sceMoveDigitalButtonSelect  |
        sceMoveDigitalButtonT        |
        sceMoveDigitalButtonMove     |
        sceMoveDigitalButtonStart    |
        sceMoveDigitalButtonTriangle |
        sceMoveDigitalButtonCircle   |
        sceMoveDigitalButtonCross    |
        sceMoveDigitalButtonSquare),
    sceMoveDigitalButtonIntercepted = (1 << 15)
} sceMoveDigitalButton;

typedef struct sceMoveDeviceInfo {
    sceFloat fSphereRadius;
    sceFloat faAccelToSphereOffset[3];
} sceMoveDeviceInfo;

typedef struct sceMoveButtonData {
    sceUInt16 uCastToEnumSceMoveDigitalButton;
    sceUInt16 uAnalogTButtonValue;
} sceMoveButtonData;

typedef struct sceMoveExtensionPortData {
    sceUInt16 uStatus;
    sceUInt16 uDigital1;
    sceUInt16 uDigital2;
    sceUInt16 uAnalogRightX;
    sceUInt16 uAnalogRightY;
    sceUInt16 uAnalogLeftX;
    sceUInt16 uAnalogLeftY;
    sceByte baCustom[5];
} sceMoveExtensionPortData;

typedef struct sceMoveData {
    sceFloat faAccelerometer[3];
    sceFloat faGyro[3];
    sceMoveButtonData buttonData;
    sceMoveExtensionPortData unusedExtensionPortData;
    sceInt64 dataTimestamp;
    sceInt32 dataCounter;
    sceFloat sensorTemperature;
} sceMoveData;

/* -- DEPRECATED -- */
typedef struct sceMoveExtensionPortInfo {
    unsigned int uiExtensionPortId;
    unsigned char baExtensionPortDeviceInfo[38];
} sceMoveExtensionPortInfo;

typedef enum sceError {
    SCE_OK = (int)0x0,
    SCE_MOVE_ERROR_ALREADY_INIT = (int)0x80EE0002,
    SCE_MOVE_ERROR_FATAL = (int)0x80EE00FF,
    SCE_MOVE_ERROR_NOT_INIT = (int)0x80EE0001,
    SCE_MOVE_ERROR_INVALID_ARG = (int)0x80EE0003,
    SCE_MOVE_ERROR_MAX_CONTROLLERS_EXCEEDED = (int)0x80EE0005,
    SCE_MOVE_ERROR_ALREADY_OPENED = (int)0x80EE0007,
    SCE_MOVE_ERROR_INVALID_PORT = (int)0x80EE0006,
    SCE_MOVE_ERROR_INVALID_HANDLE = (int)0x80EE0004,
    SCE_MOVE_RETURN_CODE_NO_CONTROLLER_CONNECTED = (int)0x1
} sceError;

/*
    -Type - function pointer typedefs, used in hook continuations
    -Impl - hook implementation prototypes, not to be called directly
*/

typedef sceError(*sceMoveInitType)(
    void
);

extern sceError sceMoveInitImpl(
    void
);

typedef sceError(*sceMoveTermType)(
    void
);

extern sceError sceMoveTermImpl(
    void
);

typedef sceHandleOrError(*sceMoveOpenType)(
    sceUserId userId,
    sceMoveDeviceType deviceType,
    sceInt32 iDeviceIndex
);

extern sceHandleOrError sceMoveOpenImpl(
    sceUserId userId,
    sceMoveDeviceType deviceType,
    sceInt32 iDeviceIndex
);

typedef sceError(*sceMoveCloseType)(
    sceHandle hDeviceHandle
);

extern sceError sceMoveCloseImpl(
    sceHandle hDeviceHandle
);

typedef sceError(*sceMoveReadStateRecentType)(
    sceHandle hDeviceHandle,
    sceInt64 dataTimestampInMicroseconds,
    sceMoveData *paOutMoveData,
    sceInt32 *pOutActualLength
);

extern sceError sceMoveReadStateRecentImpl(
    sceHandle hDeviceHandle,
    sceInt64 dataTimestampInMicroseconds,
    sceMoveData *paOutMoveData,
    sceInt32 *pOutActualLength
);

typedef sceError(*sceMoveReadStateLatestType)(
    sceHandle hDeviceHandle,
    sceMoveData *pOutMoveData
);

extern sceError sceMoveReadStateLatestImpl(
    sceHandle hDeviceHandle,
    sceMoveData *pOutMoveData
);

typedef sceError(*sceMoveSetLightSphereType)(
    sceHandle hDeviceHandle,
    sceByte redColorValue,
    sceByte greenColorValue,
    sceByte blueColorValue
);

extern sceError sceMoveSetLightSphereImpl(
    sceHandle hDeviceHandle,
    sceByte redColorValue,
    sceByte greenColorValue,
    sceByte blueColorValue
);

typedef sceError(*sceMoveSetVibrationType)(
    sceHandle hDeviceHandle,
    sceByte motorValue
);

extern sceError sceMoveSetVibrationImpl(
    sceHandle hDeviceHandle,
    sceByte motorValue
);

typedef sceError(*sceMoveGetDeviceInfoType)(
    sceHandle hDeviceHandle,
    sceMoveDeviceInfo *pOutDeviceInfo
);

extern sceError sceMoveGetDeviceInfoImpl(
    sceHandle hDeviceHandle,
    sceMoveDeviceInfo *pOutDeviceInfo
);

typedef sceError(*sceMoveGetExtensionPortInfoType)( /* -- DEPRECATED -- */
    sceHandle hDeviceHandle,
    sceMoveExtensionPortInfo *pOutPortInfo
);

extern sceError sceMoveGetExtensionPortInfoImpl( /* -- DEPRECATED -- */
    sceHandle hDeviceHandle,
    sceMoveExtensionPortInfo *pOutPortInfo
);

typedef sceError(*sceMoveResetLightSphereType)(
    sceHandle hDeviceHandle
);

extern sceError sceMoveResetLightSphereImpl(
    sceHandle hDeviceHandle
);

typedef sceError(*sceMoveSetExtensionPortOutputType)( /* -- DEPRECATED -- */
    sceHandle hDeviceHandle,
    unsigned char baData[40]
);

extern sceError sceMoveSetExtensionPortOutputImpl( /* -- DEPRECATED -- */
    sceHandle hDeviceHandle,
    unsigned char baData[40]
);

#ifdef __cplusplus
} /* extern "C" { */
#endif /* __cplusplus */

#endif /* _SCEMOVE_H_ */
