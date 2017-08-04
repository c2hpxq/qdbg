#include "stdafx.h"

char *read_process_memory(int addr, SIZE_T size);
BOOL write_process_memory(int addr, char *buf);
void show_process_memory(int addr, SIZE_T size, char *buf);