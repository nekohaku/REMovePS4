#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include <orbis/libkernel.h>
#include <orbis/UserService.h>
#include <orbis/Sysmodule.h>

#include "substitute.h"
#include "remove_module.h"
#include "remove_ctx.h"
#include "remove_goldhen_glue.h"

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
// goldhen stuff
#ifdef BUILD_WITH_GOLDHEN_SUPPORT
#define gehook(_OrigFunc) extern void *_OrigFunc##GoldHEN_Addr; extern PRG_Detour _OrigFunc##GoldHEN_HookInfo
#define gdhook(_OrigFunc) void *_OrigFunc##GoldHEN_Addr = 0; PRG_Detour _OrigFunc##GoldHEN_HookInfo = 0
#define gihook(_OrigFunc) ( \
    (!(_OrigFunc##GoldHEN_HookInfo)) ? \
        ( \
            (_OrigFunc##GoldHEN_HookInfo) = RG_Detour_new(), \
            sceKernelDlsym(psmove_prx_id, #_OrigFunc, &(_OrigFunc##GoldHEN_Addr)), \
            RG_Detour_DetourFunction( \
                (_OrigFunc##GoldHEN_HookInfo), \
                (void *)(_OrigFunc##GoldHEN_Addr), \
                (void *)(_OrigFunc##Impl) \
            ) \
        ) \
    : \
    /* already hooked, return 'already' value */ (void*)(0x1) \
    )

gdhook(sceMoveInit);
gdhook(sceMoveTerm);
gdhook(sceMoveOpen);
gdhook(sceMoveClose);
gdhook(sceMoveReadStateRecent);
gdhook(sceMoveReadStateLatest);
gdhook(sceMoveSetLightSphere);
gdhook(sceMoveSetVibration);
gdhook(sceMoveGetDeviceInfo);
gdhook(sceMoveResetLightSphere);
gdhook(sceMoveGetExtensionPortInfo);
gdhook(sceMoveSetExtensionPortOutput);
#endif

int have_mira = 0;
int psmove_prx_id = -1;

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
dhook(sceMoveGetExtensionPortInfo);
dhook(sceMoveSetExtensionPortOutput);

int REMove_InitHooks() {
    if (have_mira) {
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
        ihook(sceMoveGetExtensionPortInfo);
        ihook(sceMoveSetExtensionPortOutput);
    }
    else {
#ifdef BUILD_WITH_GOLDHEN_SUPPORT
        gihook(sceMoveInit);
        gihook(sceMoveTerm);
        gihook(sceMoveOpen);
        gihook(sceMoveClose);
        gihook(sceMoveReadStateRecent);
        gihook(sceMoveReadStateLatest);
        gihook(sceMoveSetLightSphere);
        gihook(sceMoveSetVibration);
        gihook(sceMoveGetDeviceInfo);
        gihook(sceMoveResetLightSphere);
        gihook(sceMoveGetExtensionPortInfo);
        gihook(sceMoveSetExtensionPortOutput);
        return 0;
#else
        return 1;
#endif
    }

    return 0;
}

sceError sceMoveInitImpl() {
    if (Context) {
        return SCE_MOVE_ERROR_ALREADY_INIT;
    }

    IREMoveContext_CreateIfNullctx();
    // will start the server thread and listen for phones...
    return Context->v->Init(Context);
}

sceError sceMoveTermImpl() {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    // `Term` will set the context to NULLCTX for us.
    return Context->v->Term(Context);
}

sceHandleOrError sceMoveOpenImpl(sceUserId userId, sceMoveDeviceType deviceType, sceInt32 iDeviceIndex) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->Open(Context, userId, deviceType, iDeviceIndex);
}

sceHandleOrError sceMoveCloseImpl(sceHandle hDeviceHandle) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->Close(Context, hDeviceHandle);
}

sceError sceMoveReadStateRecentImpl(sceHandle hDeviceHandle, sceInt64 dataTimestampInMicroseconds, sceMoveData *paOutMoveData, sceInt32 *pOutActualLength) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->ReadStateRecent(Context, hDeviceHandle, dataTimestampInMicroseconds, paOutMoveData, pOutActualLength);
}

sceError sceMoveReadStateLatestImpl(sceHandle hDeviceHandle, sceMoveData *pOutMoveData) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->ReadStateLatest(Context, hDeviceHandle, pOutMoveData);
}

sceError sceMoveSetLightSphereImpl(sceHandle hDeviceHandle, sceByte redColorValue, sceByte greenColorValue, sceByte blueColorValue) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->SetLightSphere(Context, hDeviceHandle, redColorValue, greenColorValue, blueColorValue);
}

sceError sceMoveSetVibrationImpl(sceHandle hDeviceHandle, sceByte motorValue) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->SetVibration(Context, hDeviceHandle, motorValue);
}

sceError sceMoveGetDeviceInfoImpl(sceHandle hDeviceHandle, sceMoveDeviceInfo *pOutDeviceInfo) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->GetDeviceInfo(Context, hDeviceHandle, pOutDeviceInfo);
}

sceError sceMoveGetExtensionPortInfoImpl( /* -- DEPRECATED -- */ sceHandle hDeviceHandle, sceMoveExtensionPortInfo *pOutPortInfo) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    /* -- DEPRECATED -- */
    return Context->v->GetExtensionPortInfo(Context, hDeviceHandle, pOutPortInfo);
}

sceError sceMoveResetLightSphereImpl(sceHandle hDeviceHandle) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    return Context->v->ResetLightSphere(Context, hDeviceHandle);
}

sceError sceMoveSetExtensionPortOutputImpl( /* -- DEPRECATED -- */ sceHandle hDeviceHandle, unsigned char baData[40]) {
    if (!Context) {
        return SCE_MOVE_ERROR_NOT_INIT;
    }

    /* -- DEPRECATED -- */
    return Context->v->SetExtensionPortOutput(Context, hDeviceHandle, baData);
}

void REMove_module_start() {
    have_mira = substitute_is_present();
    psmove_prx_id = sceKernelLoadStartModule("libSceMove.sprx", 0, 0, 0, 0, 0);
    if (psmove_prx_id >= 0) {
        REMove_InitHooks();
    }
}

void REMove_module_stop() {
    kprintf("[remove]: Module stop is called...\n");
    // try to free the context just in case...
    if (Context) {
        Context->v->Term(Context);
    }
}

// -- CRT -- //

#pragma region "Our custom CRT since OO's is broken"

static int is_initialized = 0;
static int is_finalized = 0;
#define prxex __attribute__((visibility("default")))

#ifndef BUILD_WITH_GOLDHEN_SUPPORT

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
        REMove_module_stop();
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

#else /* BUILD_WITH_GOLDHEN_SUPPORT: */

/* special GoldHEN plugin CRT */
prxex void _init(); /* provided by crtprx.o */
prxex void _fini(); /* provided by crtprx.o */
/* for some bizzare reason if I don't do this, clang won't link GoldHEN's `crtprx.o` */
prxex void *_init_addr_dummy = (void *)&_init;
prxex void *_fini_addr_dummy = (void *)&_fini;

prxex int module_start(unsigned long long argl, const void *argp) {
    if (!is_initialized) {
        is_initialized = 1;
        REMove_module_start();
    }

    return 0;
}

prxex int module_stop(unsigned long long argl, const void *argp) {
    if (!is_finalized) {
        is_finalized = 1;
        REMove_module_stop();
    }

    return 0;
}

#endif

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

#ifndef my_strncpy
#define my_strncpy(_Dest, _Src, _Len) \
    for (unsigned long long _Index = 0; _Index < _Len; ++_Index) \
        if ('\0' == _Src[_Index]) { _Dest[_Index] = '\0'; break; } \
        else _Dest[_Index] = _Src[_Index];
#endif

int nprintf(const char *fmt, ...) {
    va_list va_l;
    int wrote = 0;
    char buff[512] = { '\0' };
    OrbisNotificationRequest req = { 0 };
    char icon[] = "cxml://psnotification/tex_device_move";

    va_start(va_l, fmt);
    wrote = vsnprintf(buff, sizeof(buff), fmt, va_l);
    va_end(va_l);

    req.type = NotificationRequest;
    req.useIconImageUri = 1;
    req.targetId = -1;
    my_strncpy(req.message, buff, sizeof(req.message));
    my_strncpy(req.iconUri, icon, sizeof(req.iconUri));

    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
    return wrote;
}

#undef my_strncpy
