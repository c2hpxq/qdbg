#include "stdafx.h"
#include "breakpoint.h"
#include "memop.h"

ostream& operator<< (ostream& out, const SoftBreak& t){
	out << t.get_id() <<  '\t' << "Software Breakpoint" << hex << t.get_addr() << '\t' << t.get_times();
	return out;
}

void BreakPointList::set_software_bp(UINT addr) {
	printf("Wanna Set SoftBreak at addr: 0x%x\n", addr);
	if(is_sb_duplicate(addr)) {
		fprintf(stderr, "software breakpoint at 0x%x has been set\n",addr);
		return;
	}
	char *buf;
	if( !(buf = read_process_memory(addr,1)) )
		return;
	if( !write_process_memory(addr,"\xCC") )
		return;
	SoftBreak* psb = new SoftBreak(id++, addr, buf[0]);
	free(buf);
	bp_list.push_back(psb);
}

void BreakPointList::set_hardware_bp(UINT addr, UINT len, char cond, DWORD threadID) {
	printf("Wanna Set HardBreak at addr: 0x%x(%c)\n", addr, cond);
	if(is_hb_duplicate(addr,cond)) {
		fprintf(stderr, "hardware breakpoint at 0x%x(%c) has been set\n",addr,cond);
		return;
	}
	if( !(len==1 || len==2 || len==4) ||
		!(cond==HW_ACCESS || cond==HW_WRITE || cond==HW_EXECUTE) ) {
		cerr << "Set hardware breakpoint fail" << endl;
		return;
	}
	
	HardBreak* hb = new HardBreak(id++, addr, len, cond);
	HANDLE th;
	if(!(th=OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadID))) {
		cerr << "OpenThread fail" << endl;
		return;
	}
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);
	if( !add_hb_inner(&ctx, hb) ) {
		CloseHandle(th);
		return;
	}
	SetThreadContext(th, &ctx);
	CloseHandle(th);
	bp_list.push_back(hb);
}

void BreakPointList::set_memory_bp(UINT addr, UINT len){
}

BOOL BreakPointList::delete_bp_by_id(UINT id){
	list<BreakPoint*>::iterator it;
	for (it = bp_list.begin(); it != bp_list.end(); ++it){
		if ((*it)->get_id() == id){
			bp_list.erase(it);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL BreakPointList::add_hb_inner(LPCONTEXT lpctx, HardBreak* hb) {
	int i, available = 4;
	map<int,HardBreak*>::const_iterator it;
	for(i=0;i<4;i++) {
		if(!hb_map.count(i)) {
			available = i;
			break;
		}
	}
	if(available == 4) {
		cerr << "No more hardware breakpoint can be set" << endl;
		return FALSE;
	}
	//cout << "add bp available: " << available << endl;
	lpctx->Dr7 |= 1 << (available*2);
	if(available == 0)
		lpctx->Dr0 = hb->get_addr();
	else if(available == 1)
		lpctx->Dr1 = hb->get_addr();
	else if(available == 2)
		lpctx->Dr2 = hb->get_addr();
	else if(available == 3)
		lpctx->Dr3 = hb->get_addr();
	int cond_bit;
	switch (hb->get_cond()) {
	case 'r':
		cond_bit = 3;  //11 read/write
		break;
	case 'w':
		cond_bit = 1;  //01 write
		break;
	case 'e':
		cond_bit = 0;  //00 execute
		break;
	}
	//set the breakpoint condition
	lpctx->Dr7 |= cond_bit << (available*4 + 16);
	//set the length
	lpctx->Dr7 |= (hb->get_len()-1) << (available*4 + 18);
	hb_map.insert(make_pair(available,hb));
	return TRUE;
}

BOOL BreakPointList::delete_hb_inner(int slot, LPCONTEXT lpctx) {
	lpctx->Dr7 &= ~(1 << (slot*2));
	if (slot == 0)
        lpctx->Dr0 = 0x00000000;
	else if (slot == 1)
        lpctx->Dr1 = 0x00000000;
	else if (slot == 2)
        lpctx->Dr2 = 0x00000000;
	else if (slot == 3)
        lpctx->Dr3 = 0x00000000;
	else
		return FALSE;
    //Remove the condition flag
    lpctx->Dr7 &= ~(3 << ((slot * 4) + 16));
    //Remove the length flag
    lpctx->Dr7 &= ~(3 << ((slot * 4) + 18));
	hb_map.erase(slot);
	return TRUE;
}

//check whether addr has been set with software breakpoint
BOOL BreakPointList::is_sb_duplicate(UINT addr) {
	list<BreakPoint*>::iterator it;
	for(it = bp_list.begin(); it != bp_list.end(); ++it) {
		if((*it)->get_addr() == addr)
			return TRUE;
	}
	return FALSE;
}

//check whether addr has been set with hardware breakpoint
BOOL BreakPointList::is_hb_duplicate(UINT addr, char cond) {
	list<BreakPoint*>::iterator it;
	for(it = bp_list.begin(); it != bp_list.end(); ++it) {
		if((*it)->get_type() == HW_BP) {
			HardBreak * hb = dynamic_cast<HardBreak *>(*it);
			if( hb->get_addr()==addr && hb->get_cond()==cond)
				return TRUE;
		}
	}
	return FALSE;
}

void BreakPointList::list_breakpoints() {
	//list all breakpoints
	list<BreakPoint*>::iterator it;
	UINT type;
	cout << setiosflags(ios::left) << setw(8) << "Num" << setw(20) << "Type" << setw(16) << "Addr" << setw(10) << "Hit_Times" << endl;
	for(it = bp_list.begin(); it != bp_list.end(); ++it) {
		(*it)->list_breakpoint();
	}
}

BreakPoint * BreakPointList::get_bp_by_addr(UINT addr){
	list<BreakPoint*>::iterator it;
	for(it = bp_list.begin(); it != bp_list.end(); ++it) {
		if ((*it)->get_addr() == addr)
			return *it;
	}
	return NULL;
}

HardBreak * BreakPointList::get_hb_by_index(int index) {
	if(hb_map.count(index))
		return hb_map[index];
	return NULL;
}

void  BreakPointList::set_last_breakpoint(BreakPoint *bp) {
	last_breakpoint = bp;
}

BreakPoint * BreakPointList::get_last_breakpoint() {
	return last_breakpoint;
}

