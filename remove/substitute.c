#include <orbis/libkernel.h>
#include <stdio.h>
#include <string.h>
#include "substitute.h"
#include "remove_module.h"

#define my_printf(...) /* Nothing ! */
#define my_strncpy(_Dest, _Src, _Len) \
    for (unsigned long long _Index = 0; _Index < _Len; ++_Index) \
        if ('\0' == _Src[_Index]) { _Dest[_Index] = '\0'; break; } \
        else _Dest[_Index] = _Src[_Index];

int substitute_hook(substitute_hook_info* hook, const char* module_name, const char* name, void(*hook_function)(void), int flags) {
    // Call mira hook system
    int mira_device = sceKernelOpen("/dev/mira", 02, 0);
    if (mira_device < 0) {
        my_printf("libsubstitute -> mira_hook_iat: can't open mira device (ret 0x%X).\n", mira_device);
        return 1;
    }

    // Setup parameter
    struct substitute_hook_iat param = { -1 };
    param.hook_id = -1; // Pre-set for prevent to get 0 if something append
    param.hook_function = (void*)hook_function;
    param.flags = flags;
    param.chain = hook;
    my_strncpy(param.name, name, sizeof(param.name));
    my_strncpy(param.module_name, module_name, sizeof(param.module_name));
    my_printf("libsubstitute -> mira_hook_iat: trying to hook module name %s function name %s\n", param.module_name, param.name);

    // Do ioctl
    int ret = ioctl(mira_device, MIRA_IOCTL_IAT_HOOK, &param);
    sceKernelClose(mira_device);
    mira_device = -1;

    if (ret != 0) {
        my_printf("libsubstitute -> mira_hook_iat: ioctl error (ret: 0x%X).\n", ret);
        return 1;
    }

    // Check returned data
    if (param.hook_id < 0) {
        my_printf("libsubstitute -> mira_hook_iat: data returned is invalid (ret: 0x%X)!\n", param.hook_id);
        return 1;
    }

    my_printf("libsubstitute -> new hook iat ! hook_id: %i chain_structure: %p\n", param.hook_id, (void*)param.chain);
    return 0;
}

int substitute_statehook(substitute_hook_info* hook, substitute_state state) {
    if (!hook) {
        my_printf("libsubstitute -> mira_hook_iat: invalid parameter.\n");
        return 1;
    }

    // Call mira hook system
    int mira_device = sceKernelOpen("/dev/mira", 02, 0);
    if (mira_device < 0) {
        my_printf("libsubstitute -> mira_hook_iat: can't open mira device (ret: 0x%X).\n", mira_device);
        return 1;
    }

    // Setup parameter
    substitute_state_hook param = { 0 };
    param.hook_id = hook->hook_id;
    param.state = state;
    param.chain = hook;

    // Do ioctl
    int ret = ioctl(mira_device, MIRA_IOCTL_STATE_HOOK, &param);
    sceKernelClose(mira_device);
    mira_device = -1;

    if (ret < 0) {
        my_printf("libsubstitute -> mira_state_hook: ioctl error (ret: 0x%X).\n", ret);
        return 1;
    }

    // Check returned data
    if (param.result < 0) {
        my_printf("libsubstitute -> mira_hook_iat: data returned is invalid (ret: 0x%X) !\n", param.result);
        return 1;
    }

    return 0;
}

int substitute_disable(substitute_hook_info* hook) {
    return substitute_statehook(hook, SUBSTITUTE_STATE_DISABLE);
}

int substitute_enable(substitute_hook_info* hook) {
    return substitute_statehook(hook, SUBSTITUTE_STATE_ENABLE);
}

int substitute_unhook(substitute_hook_info* hook) {
    return substitute_statehook(hook, SUBSTITUTE_STATE_UNHOOK);
}

/* Call this only once. */
int substitute_is_present() {
    int fd;
    
    fd = sceKernelOpen("/dev/mira", 02, 0);
    if (fd < 0) {
        return 0;
    }

    sceKernelClose(fd);
    return 1;
}
