#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include <orbis/libkernel.h>
#include <orbis/UserService.h>
#include <orbis/Sysmodule.h>

#include "substitute.h"
#include "remove_module.h"
#include "remove_ctx.h"

// -- REMove Main Module -- //

/* extern hook */
#define ehook(_OrigFunc) extern substitute_hook_info _OrigFunc##HookInfo
/* define hook */
#define dhook(_OrigFunc) substitute_hook_info _OrigFunc##HookInfo = { -1 }
/* initialize hook */
#define ihook(_OrigFunc) ( ((_OrigFunc##HookInfo).hook_id < 0) ? \
    substitute_hook( /* do the Mira magic */ \
        &(_OrigFunc##HookInfo), \
        SUBSTITUTE_MAIN_MODULE, \
        #_OrigFunc, \
        ((void(*)())&(_OrigFunc##Impl)), \
        SUBSTITUTE_IAT_NAME \
    ) : /* already hooked, return success */ (0) )
/* continue hook */
#define chook(_OrigFunc, ...) \
    SUBSTITUTE_CONTINUE( \
        &(_OrigFunc##HookInfo), \
        _OrigFunc##Type, \
        __VA_ARGS__ \
    )

dhook(sceMoveInit);
dhook(sceMoveTerm);
dhook(sceMoveOpen);
dhook(sceMoveClose);
dhook(sceMoveReadStateRecent);
dhook(sceMoveReadStateLatest);
dhook(sceMoveSetLightSphere);
dhook(sceMoveSetVibration);
dhook(sceMoveGetDeviceInfo);
dhook(sceMoveResetLightSphere);

// -- VERY VERY UGLY HACK BEGIN -- //
typedef sceError(*sceUserServiceInitializeType)(const void *pOptInitParameters);
dhook(sceUserServiceInitialize);
sceError sceUserServiceInitializeImpl(const void *pOptInitParameters) {
    // :(
    if (sceSysmoduleIsLoaded(ORBIS_SYSMODULE_MOVE) < 0) {
        if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_MOVE) >= 0) {
            REMove_InitHooks();
        }
    }
    // carry on:
    return chook(sceUserServiceInitialize, pOptInitParameters);
}
// -- VERY VERY UGLY HACK  END  -- //

int REMove_InitHooks() {
    kprintf("[remove]: REMove - PlayStation Move API emulator by nik\n");

    ihook(sceMoveInit);
    ihook(sceMoveTerm);
    ihook(sceMoveOpen);
    ihook(sceMoveClose);
    ihook(sceMoveReadStateRecent);
    ihook(sceMoveReadStateLatest);
    ihook(sceMoveSetLightSphere);
    ihook(sceMoveSetVibration);
    ihook(sceMoveGetDeviceInfo);
    ihook(sceMoveResetLightSphere);

    kprintf("[remove]: Initialized sceMove hooks without issues\n");
    return 0;
}

sceError sceMoveInitImpl() {
    if (Context) {
        return SCE_MOVE_ERROR_ALREADY_INIT;
    }

    IREMoveContext_CreateIfNullctx();
    // will start the server thread and listen for phones...
    return Context->Init(Context);
}

sceError sceMoveTermImpl() {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    // `Term` will set the context to NULLCTX for us.
    return Context->Term(Context);
}

sceHandleOrError sceMoveOpenImpl(sceUserId userId, sceMoveDeviceType deviceType, sceInt32 iDeviceIndex) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->Open(Context, userId, deviceType, iDeviceIndex);
}

sceHandleOrError sceMoveCloseImpl(sceHandle hDeviceHandle) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->Close(Context, hDeviceHandle);
}

sceError sceMoveReadStateRecentImpl(sceHandle hDeviceHandle, sceInt64 dataTimestampInMicroseconds, sceMoveData *paOutMoveData, sceInt32 *pOutActualLength) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->ReadStateRecent(Context, hDeviceHandle, dataTimestampInMicroseconds, paOutMoveData, pOutActualLength);
}

sceError sceMoveReadStateLatestImpl(sceHandle hDeviceHandle, sceMoveData *pOutMoveData) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->ReadStateLatest(Context, hDeviceHandle, pOutMoveData);
}

sceError sceMoveSetLightSphereImpl(sceHandle hDeviceHandle, sceByte redColorValue, sceByte greenColorValue, sceByte blueColorValue) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->SetLightSphere(Context, hDeviceHandle, redColorValue, greenColorValue, blueColorValue);
}

sceError sceMoveSetVibrationImpl(sceHandle hDeviceHandle, sceByte motorValue) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->SetVibration(Context, hDeviceHandle, motorValue);
}

sceError sceMoveGetDeviceInfoImpl(sceHandle hDeviceHandle, sceMoveDeviceInfo *pOutDeviceInfo) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->GetDeviceInfo(Context, hDeviceHandle, pOutDeviceInfo);
}

sceError sceMoveGetExtensionPortInfoImpl( /* -- DEPRECATED -- */ sceHandle hDeviceHandle, sceMoveExtensionPortInfo *pOutPortInfo) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    /* -- DEPRECATED -- */
    return Context->GetExtensionPortInfo(Context, hDeviceHandle, pOutPortInfo);
}

sceError sceMoveResetLightSphereImpl(sceHandle hDeviceHandle) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->ResetLightSphere(Context, hDeviceHandle);
}

sceError sceMoveSetExtensionPortOutputImpl( /* -- DEPRECATED -- */ sceHandle hDeviceHandle, unsigned char baData[40]) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    /* -- DEPRECATED -- */
    return Context->SetExtensionPortOutput(Context, hDeviceHandle, baData);
}

void REMove_module_start() {
    ihook(sceUserServiceInitialize);
}

// -- CRT -- //

#pragma region "Our custom CRT since OO's is broken"

static int is_initialized = 0;
static int is_finalized = 0;

extern void(*__preinit_array_start[])(void);
extern void(*__preinit_array_end[])(void);
extern void(*__init_array_start[])(void);
extern void(*__init_array_end[])(void);
extern void(*__fini_array_start[])(void);
extern void(*__fini_array_end[])(void);

// sce_module_param
__asm__(
".intel_syntax noprefix \n"
".align 0x8 \n"
".section \".data.sce_module_param\" \n"
"_sceProcessParam: \n"
    // size
"	.quad 	0x18 \n"
    // magic
"	.quad   0x13C13F4BF \n"
    // SDK version
"	.quad 	0x4508101 \n"
".att_syntax prefix \n"
);

// data globals
__asm__(
".intel_syntax noprefix \n"
".align 0x8 \n"
".data \n"
"_sceLibc: \n"
"	.quad 	0 \n"
".att_syntax prefix \n"
);

const void *const __dso_handle __attribute__ ((__visibility__ ("hidden"))) = &__dso_handle;

#define prxex __attribute__((visibility("default")))

prxex void _init() {
    if (!is_initialized) {
        is_initialized = 1;

        /* usually preinit start==end so it's not used, but who knows? */
        for(void(**__i)(void) = __preinit_array_start; __i != __preinit_array_end; __i++) {
            __i[0]();
        }

        for(void(**__i)(void) = __init_array_start; __i != __init_array_end; __i++) {
            __i[0]();
        }

        /* call your entry point here: */
        REMove_module_start();
    }
}

prxex void _fini() {
    if (!is_finalized) {
        is_finalized = 1;

        for(void(**__i)(void) = __fini_array_start; __i != __fini_array_end; __i++) {
            __i[0]();
        }

        /* call your quit point (LOL) here: */
    }
}

prxex int module_start(unsigned long long argl, const void *argp) {
    _init();
    return 0;
}

prxex int module_stop(unsigned long long argl, const void *argp) {
    _fini();
    return 0;
}

#pragma endregion /* end of CRT */

int kprintf(const char *fmt, ...) {
    va_list va_l;
    int wrote = 0;
    char buff[4096] = { '\0' };

    va_start(va_l, fmt);
    wrote = vsnprintf(buff, sizeof(buff), fmt, va_l);
    va_end(va_l);

    sceKernelDebugOutText(0, buff);
    return wrote;
}
