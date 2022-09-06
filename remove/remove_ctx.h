#ifndef _REMOVE_CTX_H_
#define _REMOVE_CTX_H_ 1
#pragma once

#include "scemove.h"
#include "remove_module.h"

// private: do not use.
typedef struct IREMoveContextData IREMoveContextData;

// public:
typedef struct IREMoveContext IREMoveContext;
typedef struct IREMoveContextVtbl IREMoveContextVtbl;

struct IREMoveContextVtbl {
    // virtual functions: -- public interface
    sceError(*Init)(IREMoveContext *self);
    sceError(*Term)(IREMoveContext *self);
    sceHandleOrError(*Open)(IREMoveContext *self, sceUserId userId, sceMoveDeviceType deviceType, sceInt32 iDeviceIndex);
    sceError(*Close)(IREMoveContext *self, sceHandle hDeviceHandle);
    sceError(*ReadStateRecent)(IREMoveContext *self, sceHandle hDeviceHandle, sceInt64 dataTimestampInMicroseconds, sceMoveData *paOutMoveData, sceInt32 *pOutActualLength);
    sceError(*ReadStateLatest)(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveData *pOutMoveData);
    sceError(*SetLightSphere)(IREMoveContext *self, sceHandle hDeviceHandle, sceByte redColorValue, sceByte greenColorValue, sceByte blueColorValue);
    sceError(*SetVibration)(IREMoveContext *self, sceHandle hDeviceHandle, sceByte motorValue);
    sceError(*GetDeviceInfo)(IREMoveContext *self, sceHandle hDeviceHandle, sceMoveDeviceInfo *pOutDeviceInfo);
    sceError(*GetExtensionPortInfo)( /* -- DEPRECATED -- */ IREMoveContext *self, sceHandle hDeviceHandle, sceMoveExtensionPortInfo *pOutPortInfo);
    sceError(*ResetLightSphere)(IREMoveContext *self, sceHandle hDeviceHandle);
    sceError(*SetExtensionPortOutput)( /* -- DEPRECATED -- */ IREMoveContext *self, sceHandle hDeviceHandle, unsigned char baData[40]);
};

struct IREMoveContext {
    // virtual function table: -- do not modify
    IREMoveContextVtbl *v;

    // context data: -- do not use, private.
    IREMoveContextData *Data;
};

// do not modify, use IREMoveContext_CreateIfNullctx
extern IREMoveContext *Context;

#define NULLCTX ((IREMoveContext *)(0))

extern void IREMoveContext_CreateIfNullctx();

#endif /* _REMOVE_CTX_H_ */

