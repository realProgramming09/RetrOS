#pragma once
#include "kernel/sys.h"

void handleSyscall(uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);