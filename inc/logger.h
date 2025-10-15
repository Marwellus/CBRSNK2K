#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "inc/types.h"

extern bool __DEBUG;

void Log_Info(char* format, ...);
void Log_Dump();

#ifdef __cplusplus
}
#endif

#endif
