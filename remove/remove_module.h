
#pragma once
#ifndef _REMOVE_MODULE_H_
#define _REMOVE_MODULE_H_ 1

// a-la printf but does a debug kernel log
extern int kprintf(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

// a-la printf but does a notification pop up
extern int nprintf(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

extern int REMove_InitHooks();

extern int have_mira;

#endif
