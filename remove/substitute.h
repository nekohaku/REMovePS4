#pragma once

#ifndef _SUBSTITUTE_H_
#define _SUBSTITUTE_H_ 1

#define MIRA_IOCTL_IAT_HOOK   0xC2185301
#define MIRA_IOCTL_JMP_HOOK   0xC0185302
#define MIRA_IOCTL_STATE_HOOK 0xC0185303

#define SUBSTITUTE_MAX_NAME 255 // Max lenght for name
#define SUBSTITUTE_MAIN_MODULE "" // Define the main module

typedef enum _tagsubstitute_hook_flags {
  SUBSTITUTE_IAT_NAME = 0,
  SUBSTITUTE_IAT_NIDS = 1
} substitute_hook_flags;

typedef enum _tagsubstitute_state_enum {
  SUBSTITUTE_STATE_DISABLE = 0,
  SUBSTITUTE_STATE_ENABLE = 1,
  SUBSTITUTE_STATE_UNHOOK = 2
} substitute_state;

// IOCTL IN/OUT Structure
struct substitute_state_hook {
  int hook_id;
  int state;
  struct substitute_hook_info* chain;
  int result;
};

struct substitute_hook_iat {
  int hook_id;
  char name[SUBSTITUTE_MAX_NAME];
  char module_name[SUBSTITUTE_MAX_NAME];
  int flags;
  void* hook_function;
  struct substitute_hook_info* chain;
};

// Structure for userland
struct substitute_hook_info {
  int hook_id;
  void* hook_function;
  void* original_function;
  struct substitute_hook_info* next;
};

typedef struct substitute_hook_info substitute_hook_info;
typedef struct substitute_hook_iat substitute_hook_iat;
typedef struct substitute_state_hook substitute_state_hook;

// Thanks Yifan ! (taiHEN source code)
// Chain execution (All chain edition was do by kernel)

#define SUBSTITUTE_WAIT(_HookInfoPointer) while (!(_HookInfoPointer)) { }

#define SUBSTITUTE_CONTINUE(_HookInfoPointer, _FunctionPointerType, ...) \
  ( ! ( (_HookInfoPointer)->next ) ) \
  ? \
    ( ( (_FunctionPointerType) ( (_HookInfoPointer)->original_function  ) ) (__VA_ARGS__) ) \
  : \
    ( ( (_FunctionPointerType) ( (_HookInfoPointer)->next->hook_function) ) (__VA_ARGS__) )

int substitute_hook(substitute_hook_info* OUT_hook, const char* module_name, const char* name, void(*hook_function)(void), int flags);
int substitute_statehook(substitute_hook_info* hook, substitute_state state);
int substitute_disable(substitute_hook_info* hook);
int substitute_enable(substitute_hook_info* hook);
int substitute_unhook(substitute_hook_info* hook);

#endif
