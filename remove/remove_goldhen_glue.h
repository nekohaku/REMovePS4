#ifndef _REMOVE_GOLDHEN_GLUE_H_
#define _REMOVE_GOLDHEN_GLUE_H_ 1
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// REMove <3 GoldHEN extern-C glue.
// since I refuse to use C++ APIs in public.

// opaque struct handle that can be passed to C and C++ equally.
typedef struct RG_Detour RG_Detour, *PRG_Detour;

PRG_Detour RG_Detour_new();

void *RG_Detour_DetourFunction(PRG_Detour self, void *original, void *calltarget);

PRG_Detour RG_Detour_delete(PRG_Detour self);

#ifdef __cplusplus
}
#endif
#endif
