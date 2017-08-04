#include "stdafx.h"

void show_all_regs(DWORD threadID) {
	HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadID);
	if(!th)
		return;
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);
	cout << setiosflags(ios::left) << "eax: 0x" << setw(12) << hex << ctx.Eax <<
								      "ebx: 0x" << setw(12) << hex << ctx.Ebx <<
								      "ecx: 0x" << setw(12) << hex << ctx.Ecx <<
								      "edx: 0x" << setw(12) << hex << ctx.Edx << endl;
	cout << setiosflags(ios::left) << "esi: 0x" << setw(12) << hex << ctx.Esi <<
								      "edi: 0x" << setw(12) << hex << ctx.Edi <<
								      "esp: 0x" << setw(12) << hex << ctx.Esp <<
								      "ebp: 0x" << setw(12) << hex << ctx.Ebp << endl;
	cout << setiosflags(ios::left) << "eip: 0x" << setw(12) << hex << ctx.Eip << endl;
}