/*
 * GoldHEN Plugin SDK - a prx hook/patch sdk for Orbis OS
 *
 * Credits
 * - OSM <https://github.com/OSM-Made>
 * - jocover <https://github.com/jocover>
 * - bucanero <https://github.com/bucanero>
 * - OpenOrbis Team <https://github.com/OpenOrbis>
 * - SiSTRo <https://github.com/SiSTR0>
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

enum DetourMode {
    x64, x32
};

class Detour {
private:
    DetourMode Mode;
    void *StubPtr = 0;
    size_t StubSize = 0;

    void *FunctionPtr = 0;
    void *HookPtr = 0;

    uint8_t JumpInstructions64[14] = {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};  // jmp QWORD PTR[Address]
    uint8_t JumpInstructions32[05] = {0xE9, 0x00, 0x00, 0x00, 0x00};  // jmp 32

    void WriteJump64(void *Address, uint64_t Destination);

    void WriteJump32(void *Address, uint64_t Destination);

    void *DetourFunction64(uint64_t FunctionPtr, void *HookPtr);

    void *DetourFunction32(uint64_t FunctionPtr, void *HookPtr);

public:
    template<typename result, typename... Args>
    result Stub(Args... args) {
        result (*Stub_internal)(Args... args) = decltype(Stub_internal)(StubPtr);
        return Stub_internal(args...);
    }

    void *DetourFunction(uint64_t FunctionPtr, void *HookPtr);

    void RestoreFunction();

    Detour(DetourMode Mode);

    ~Detour();
};
