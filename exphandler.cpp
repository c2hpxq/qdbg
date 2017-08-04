#include "stdafx.h"
#include "exphandler.h"
#include "memop.h"
#include "breakpoint.h"

extern BreakPointList bplist;
extern void get_cmd_from_user(DWORD threadID);
SoftBreak * waitfor0xcc;


DWORD exception_handler_breakpoint(LPDEBUG_EVENT pde) {
	UINT addr = (UINT)pde->u.Exception.ExceptionRecord.ExceptionAddress;
	HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, pde->dwThreadId);
	if (!th)
		return -1;

	cout <<"Hit Breakpoint @Address: " << hex << addr << endl;
	//if software bp, change the first byte from '\xCC' to original byte
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);
	
	//set trap flag on, so we can modify it back to 0xCC after original current instruction is executed
	ctx.EFlags |= 0x100;
	ctx.Eip -= 1;
	waitfor0xcc = static_cast<SoftBreak*>(bplist.get_bp_by_addr(ctx.Eip));
	if (waitfor0xcc == NULL){
		cout << "not my breakpoint, maybe system breakpoint " << "@" << hex << ctx.Eip << endl;
		cout << "currently just ignore it" << endl;
		return DBG_CONTINUE;
	}
	char tmp[2] = {waitfor0xcc->get_orig(), '\0'};
	write_process_memory(ctx.Eip, tmp);

	SetThreadContext(th, &ctx);
	CloseHandle(th);
	//record the hardware breakpoint
	//bplist.set_last_breakpoint(sb);
	return DBG_CONTINUE;
}


// single step(int1) is caused by
//		hardware breakpoints;
//		TF set by software breakpoints(to set 0xCC again);
//		TF set by hardware breakpoints(to set CR7 again);
//		TF set by single step command;
DWORD exception_handler_singlestep(LPDEBUG_EVENT pde){
	UINT addr = (UINT)pde->u.Exception.ExceptionRecord.ExceptionAddress;
	cout << "single step addr: 0x" << hex << addr << endl;
	HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, pde->dwThreadId);
	if (!th)
		return -1;
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);

	int slot = 4;
	HardBreak *hb = NULL;
	if ( (ctx.Dr6 & 0x1) && (hb = bplist.get_hb_by_index(0)) )
		slot = 0;
	else if ( (ctx.Dr6 & 0x2) && (hb = bplist.get_hb_by_index(1)) )
		slot = 1;
	else if ( (ctx.Dr6 & 0x3) && (hb = bplist.get_hb_by_index(2)) )
		slot = 2;
	else if ( (ctx.Dr6 & 0x4) && (hb = bplist.get_hb_by_index(3)) )
		slot = 3;
	//hardware breakpoints
	if (slot != 4 && hb) {  
		hb->set_times(hb->get_times() + 1);
		printf("hit hardware breakpoint at 0x%x(%c)", addr, hb->get_cond());
		//record the hardware breakpoint
		bplist.set_last_breakpoint(hb);
		//delete hardware breakpoint temporarily
		bplist.delete_hb_inner(slot, &ctx); 
		//set trap flag
		ctx.EFlags |= 0x100;
		SetThreadContext(th, &ctx);
		CloseHandle(th);
		return DBG_CONTINUE;
	}
	//TF flag
	else {
		BreakPoint *last_bp = bplist.get_last_breakpoint();
		//set by debugger itself
		if(last_bp) {
			int type = last_bp->get_type();
			if (type == SW_BP) {
			}
			else if(type == HW_BP) {
				bplist.add_hb_inner(&ctx, dynamic_cast<HardBreak *>(last_bp));
				get_cmd_from_user(pde->dwThreadId);
			}
		}
		//set by single step command
		else {
			get_cmd_from_user(pde->dwThreadId);
		}
		bplist.set_last_breakpoint(NULL);
	}
	//write_process_memory(waitfor0xcc->get_addr(), "\xCC");
	return DBG_CONTINUE;
}