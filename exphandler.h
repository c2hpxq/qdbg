#include "stdafx.h"

#define GET_CMD ((DWORD)0x00100)

DWORD exception_handler_breakpoint(LPDEBUG_EVENT pde);
DWORD exception_handler_singlestep(LPDEBUG_EVENT pde);