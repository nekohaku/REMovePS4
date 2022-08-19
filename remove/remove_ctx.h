
#pragma once
#ifndef _REMOVE_CTX_H_
#define _REMOVE_CTX_H_ 1

#include "scemove.h"
#include "remove_module.h"

// private: do not use.
typedef struct IREMoveContextData IREMoveContextData;

// public:
typedef struct IREMoveContext IREMoveContext;

struct IREMoveContext {
    // functions: -- public interface
    sceError(*Init)(IREMoveContext *self);
    sceError(*Term)(IREMoveContext *self);
    sceHandleOrError(*Open)(IREMoveContext *self, sceUserId userId, sceMoveDeviceType deviceType, sceInt32 iDeviceIndex);
    sceError(*Close)(IREMoveContext *self, sceHandle hDeviceHandle);
    sceError(*ReadStateRecent)(IREMoveContext *self, sceHandle hDeviceHandle, sceInt64 dataTimestampInMicroseconds, sceMoveData *paOutMoveData, sceInt32 *pOutActualLength);
    sceError(*ReadStateLatest)(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveData *pOutMoveData);
    sceError(*SetLightSphere)(IREMoveContext *self, sceHandle hDeviceHandle, sceByte redColorValue, sceByte greenColorValue, sceByte blueColorValue);
    sceError(*SetVibration)(IREMoveContext *self, sceHandle hDeviceHandle, sceByte motorValue);
    sceError(*GetDeviceInfo)(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveDeviceInfo *pOutDeviceInfo);
    sceError(*ResetLightSphere)(IREMoveContext *self, sceHandle hDeviceHandle);

    // context data: -- do not use, private.
    IREMoveContextData *Data;
};

extern IREMoveContext *Context;

#define NULLCTX ((IREMoveContext *)(0))

extern void IREMoveContext_CreateIfNullctx();

#endif /* _REMOVE_CTX_H_ */

