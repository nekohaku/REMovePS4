#include "remove_goldhen_glue.h"

#ifdef BUILD_WITH_GOLDHEN_SUPPORT
#include <new>
#include "goldhensdk/Detour.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -- public api begin -- */

#ifdef BUILD_WITH_GOLDHEN_SUPPORT

PRG_Detour RG_Detour_new() {
    return reinterpret_cast<PRG_Detour>(new(std::nothrow) Detour(DetourMode::x64));
}

void *RG_Detour_DetourFunction(PRG_Detour self, void *original, void *calltarget) {
    if (self) {
        return reinterpret_cast<Detour *>(self)->DetourFunction(
            reinterpret_cast<uint64_t>(original), /* wtf SiSTRo???? */
            calltarget
        );
    }

    return 0;
}

PRG_Detour RG_Detour_delete(PRG_Detour self) {
    if (self) {
        delete reinterpret_cast<Detour *>(self);
    }

    /*
        this is so the `hook = RG_Detour_delete(hook);` pattern can work,
        both freeing and setting the pointer to null.
    */
    return nullptr;
}

#else /* no goldhen */

PRG_Detour RG_Detour_new() {
    return nullptr;
}

void *RG_Detour_DetourFunction(PRG_Detour self, void *original, void *calltarget) {
    return nullptr;
}

PRG_Detour RG_Detour_delete(PRG_Detour self) {
    return nullptr;
}

#endif /* end */

#ifdef __cplusplus
}
#endif
