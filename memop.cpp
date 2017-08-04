#include "stdafx.h"

#define MAX_MEM_READ 20
extern DWORD pid;

char *read_process_memory(int addr, SIZE_T size) {
	SIZE_T dwReadBytes;
	HANDLE ph = OpenProcess(PROCESS_VM_READ|PROCESS_VM_OPERATION, FALSE, pid);
	if (!ph) {
		cerr << "Open For Read Fail" << endl;
		return NULL;
	}
	//GetModuleInformation(ph, ph, 
	char *buf = (char*)malloc(size+1); 
	if( !ReadProcessMemory(ph, (VOID*)addr, buf, size, &dwReadBytes) ) {
		cerr << "Read Process Memory Fail" << endl;
		free(buf);
		buf = NULL;
	}
	else
		cout << "Read Process Memory Succ" << endl;
	CloseHandle(ph);
	return buf;
}

BOOL write_process_memory(int addr, char *buf) {
	SIZE_T dwReadBytes;
	HANDLE ph = OpenProcess(PROCESS_VM_WRITE|PROCESS_VM_OPERATION, FALSE, pid);
	if( WriteProcessMemory(ph, (VOID*)addr, buf, strlen(buf), &dwReadBytes) )
		return TRUE;
	fprintf(stderr, "Write Process Memory Fail\n");
	CloseHandle(ph);
	return FALSE;
}

//show memory in unit of dword and each line contains 4 dwords
void show_process_memory(int addr, SIZE_T size, char *buf) {
	int n = size/4;
	int i,j,k,index,val;
	int row = n%4?(n/4+1):(n/4);
	for(i=0;i<row;i++) {
		cout << hex << addr+16*i << ":";
		j = 0;
		while(j<4 && 4*i+j<n) {
			cout << "\t";
			for(k=3;k>=0;k--) {
				index = (4*i+j)*4+k;
				cout << hex << setw(2) << setfill('0') << (static_cast<short>(buf[index]) & 0xff);
			}
			j++;
		}
		cout << endl;
	}
}