// my_debugger_new.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "breakpoint.h"
#include "exphandler.h"
#include "memop.h"
#include "regop.h"

#define UNINITIALIZED 0xFFFFFFFF
#define GET_CMD ((DWORD)0x00100)

#define MAX_CMD_LEN 60
#define MAX_CMD_N 5
#define CMD_L 10

DWORD pid, tid;
char* process_name;
HANDLE h_process;
DWORD base_addr;
HANDLE t_handle;

BreakPointList bplist;
BOOL isrunning = FALSE;

void get_cmd_from_user(DWORD threadID);
extern char *read_process_memory(int addr, SIZE_T size);
extern void show_process_memory(int addr, SIZE_T size, char *buf);
DWORD get_base_addr(DWORD processID, char* processName);

void run() {
	DEBUG_EVENT de;
	DWORD dwContinueStatus;
	//BOOL debugger_active = TRUE;
	DWORD exception;
	DWORD exception_address;
	while(1) {
		if( !WaitForDebugEvent(&de, INFINITE) )
			break;
		//cout << "press a key to continue..." << endl
		dwContinueStatus = DBG_CONTINUE;
		switch( de.dwDebugEventCode ) {
		case CREATE_PROCESS_DEBUG_EVENT:
			cout << "CREATE_PROCESS_DEBUG_EVENT" << endl;
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			cout << "CREATE_THREAD_DEBUG_EVENT" << endl;
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			cout << "EXIT_THREAD_DEBUG_EVENT" << endl;
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			cout << "EXIT_PROCESS_DEBUG_EVENT" << endl;
			break;
		case EXCEPTION_DEBUG_EVENT:
			cout << "\nEXCEPTION_DEBUG_EVENT" << endl;
			exception = de.u.Exception.ExceptionRecord.ExceptionCode;
			//de.u.Exception.ExceptionRecord.ExceptionAddress;
			switch( exception ) {
			case EXCEPTION_BREAKPOINT:
				cout << "\tEXCEPTION_BREAKPOINT" << endl;;
				dwContinueStatus = exception_handler_breakpoint(&de);
				break;
			case EXCEPTION_SINGLE_STEP:
				cout << "\tEXCEPTION_SINGLE_STEP" << endl;
				dwContinueStatus = exception_handler_singlestep(&de);
				break;
			case EXCEPTION_ACCESS_VIOLATION:
				//cout << "\tEXCEPTION_ACCESS_VIOLATION\n";
				break;
			default:
				dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
				break;
			}
			break;
		case LOAD_DLL_DEBUG_EVENT:
			//printf("LOAD_DLL_DEBUG_EVENT\n");
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			//printf("UNLOAD_DLL_DEBUG_EVENT\n");
			break;
		}
		if(de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			cout << "[process exited normally]" << endl;
			break;
		}
		if(dwContinueStatus == GET_CMD) {
			get_cmd_from_user(de.dwThreadId);
			dwContinueStatus = DBG_CONTINUE;
		}
		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
	}
}
DWORD *enumerate_threads(DWORD pid) {
	DWORD *threads = (DWORD*)(1);
	return threads;
}

// get command from user
void get_cmd_from_user(DWORD threadID) {
	char str[MAX_CMD_LEN];
	const char *d = " ";
	char *p;
	char *argv[MAX_CMD_N];
	UINT addr,len;
	char cond;
	SIZE_T size;
	while(1) {
		cout << "\nmy-debugger$ ";
		memset(str, 0, MAX_CMD_LEN);
		gets_s(str);
		if(!str[0])
			continue;
		argv[0] = strtok_s(str, d, &p);

		// quit the debugger
		if( strcmp(argv[0], "q")==0 || strcmp(argv[0], "quit")==0 )
			exit(0);

		// software breakpoint: bp addr
		else if( strcmp(argv[0], "bp")==0 ) {
			argv[1] = strtok_s(NULL, d, &p);
			sscanf_s(argv[1],"%x", &addr);
			bplist.set_software_bp(addr);
		}

		// hardware breakpoint: ba r/w/e len addr
		else if( strcmp(argv[0], "ba")==0 ) {
			argv[1] = strtok_s(NULL, d, &p); //cond
			cond = argv[1][0];
			argv[2] = strtok_s(NULL, d, &p); //len
			len = atoi(argv[2]);
			argv[3] = strtok_s(NULL, d, &p); //addr
			sscanf_s(argv[3],"%x", &addr);
			//HardBreak(argv[1], atoi(argv[2]), addr);
			bplist.set_hardware_bp(addr, len, cond, threadID);
		}

		// memory breakpoint: bm addr len
		else if( strcmp(argv[0], "bm")==0 ) {
			argv[1] = strtok_s(NULL, d, &p);
			sscanf_s(argv[1],"%x", &addr);
			argv[2] = strtok_s(NULL, d, &p);
			bplist.set_memory_bp(addr, atoi(argv[2]));
		}

		// list breakpoints: bl
		else if( strcmp(argv[0], "bl")==0 )
			bplist.list_breakpoints();

		// delete breakpoints by id: bc id
		else if( strcmp(argv[0], "bc")==0 ) {
			argv[1] = strtok_s(NULL, d, &p);
			bplist.delete_bp_by_id(atoi(argv[1]));
		}

		// show memory(dword): dd size addr
		// here: size is the num of dword
		else if( strcmp(argv[0], "dd")==0 ) {
			argv[1] = strtok_s(NULL, d, &p);
			size = atoi(argv[1]);
			argv[2] = strtok_s(NULL, d, &p);
			sscanf_s(argv[2],"%x", &addr);
			char *buf = read_process_memory(addr, 4*size);
			if(buf==NULL)
				continue;
			show_process_memory(addr,4*size,buf);
			free(buf);
		}

		// single step into
		else if( strcmp(argv[0], "s")==0 ) {
			if (isrunning)
				break;
		}

		// single step over (next)
		else if( strcmp(argv[0], "n")==0 ) {
			HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadID);
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_ALL;
			GetThreadContext(th, &ctx);
			//set trap flag
			ctx.EFlags |= 0x100;
			SetThreadContext(th, &ctx);
			CloseHandle(th);
			if (isrunning)
				break;
		}

		// i r  show all regs
		else if( strcmp(argv[0], "i")==0 ) {
			argv[1] = strtok_s(NULL, d, &p);
			if( strcmp(argv[1], "r")==0 ) {
				show_all_regs(threadID);
			}
			else
				cout << "error cmd" << endl;
		}

		// start to run the process
		else if( strcmp(argv[0], "r")==0 || strcmp(argv[0], "run")==0 ){
			isrunning = TRUE;
			cout << "run" << endl;
			run();
		}

		// continue
		else if( strcmp(argv[0], "c")==0 ){
			if (isrunning)
				break;
			else cout << "no process running" << endl;
		}

		else
			cout << "error cmd" << endl;
	}
}

DWORD get_base_addr(DWORD threadID) {
	HANDLE th;
	if(!(th=OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadID))) {
		cerr << "OpenThread fail" << endl;
		return -1;
	}
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(th, &ctx);
	char *base = read_process_memory(ctx.Ebx+8,4);  //ctx.Ebx = PEB
	DWORD entry_point = ctx.Eax;
	DWORD base_addr = ((int(base[3])*256+int(base[2]))*256+int(base[1]))*256+int(base[0]);
	free(base);
	cout << "Base Addr: " << hex << base_addr << endl;
	cout << "Entry_point: " << hex << entry_point << endl;
	return base_addr;
}


DWORD get_base_addr_bak(DWORD processID, char* processName) {
	DWORD  processBaseAddress   = UNINITIALIZED;
	HANDLE moduleSnapshotHandle = INVALID_HANDLE_VALUE;
	MODULEENTRY32 moduleEntry;

	/* Take snapshot of all the modules in the process */
	cout << "PID: " << processID << endl;
    moduleSnapshotHandle = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, processID);
	/* Snapshot failed */
    if( moduleSnapshotHandle == INVALID_HANDLE_VALUE ) {
		cout << "Module Snapshot Error" << endl;
		printf("error: %d", GetLastError());
		return -1;
	}
	/* Size the structure before usage */
    moduleEntry.dwSize = sizeof( MODULEENTRY32 );
	
	/* Retrieve information about the first module */
    if( !Module32First( moduleSnapshotHandle, &moduleEntry ) )
    {
        cout << "First module not found" << endl;  
        CloseHandle( moduleSnapshotHandle );    
        return -1;
    }

	 /* Find base address */
    while(processBaseAddress == UNINITIALIZED)
    {
        /* Find module of the executable */
        do
        {
            /* Compare the name of the process to the one we want */
			char tmp[50];
			size_t len;
			wcstombs_s(&len, tmp, 50, moduleEntry.szModule, wcslen(moduleEntry.szModule));
            if( !strcmp(tmp, processName) )
            {
                /* Save the processID and break out */
                processBaseAddress = (unsigned int)moduleEntry.modBaseAddr;
                break ;
            }

        } while( Module32Next( moduleSnapshotHandle, &moduleEntry ) );

        if( processBaseAddress == UNINITIALIZED )
        {
            system("CLS");
            cout << "Failed to find module" << processName << endl;
            Sleep(200);
        }
    }
	
	return processBaseAddress;
}

/*
DWORD get_base_addr1(HANDLE hProcess, DWORD processID, char* processName) {
	HMODULE hMod;
	DWORD cbNeeded;
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod),
        &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT)) {
		GetModuleBaseName(hProcess, hMod, szProcessName,
            sizeof(szProcessName) / sizeof(TCHAR));
		char tmp[50];
		size_t len;
		wcstombs_s(&len, tmp, 50, szProcessName, wcslen(szProcessName));
		printf("tmp: %s\n",tmp);
		if( !strcmp(tmp, processName) ) 
			printf("0x%p\n",hMod);
	}
	else
		cout << "Fail to Enum" << endl;
	return (DWORD)hMod;
}
*/

/*
extern PLOADED_IMAGE ImageLoad(PCSTR, PCSTR);
extern BOOL ImageUnload(PLOADED_IMAGE);

DWORD get_default_base_addr(char *filename) {
	PIMAGE_NT_HEADERS pNTHeader;  
    DWORD ImageBase;  
    PLOADED_IMAGE pImage;  
    pImage = ImageLoad(filename, NULL);  
    if(pImage == NULL)  
		return -1;  
    pNTHeader = pImage->FileHeader;  
    ImageBase = pNTHeader->OptionalHeader.ImageBase;  
    ImageUnload(pImage);  
    return ImageBase;  
}
*/

char* get_process_name(char* fullpath) {
	const char *d = "\\";
	char *p;
	char *str, *pname=NULL;
	str = strtok_s((char*)fullpath, d, &p);
	while(str) {
		pname = str;
		str = strtok_s(NULL, d, &p);
	}
	return pname;
}


int main(int argc, char* argv[]) {
	if(argc < 3) {
		fprintf(stderr,"%s -o <sample.exe>  or\n", argv[0]);
		fprintf(stderr,"%s -a <PID>\n", argv[0]);
		return 1;
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	MODULEINFO mi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(STARTUPINFO);

	// open executable
	if(strcmp(argv[1], "-o") == 0) {
		if( !CreateProcessA(
				NULL, argv[2], NULL, NULL, FALSE, 
				NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED | DEBUG_PROCESS, 
				//CREATE_SUSPENDED,
				NULL, NULL, &si, &pi) ) {
			fprintf(stderr, "Create Process Fail\n");
			return -1;
		}
		printf("Create Process Success\n");
		h_process = pi.hProcess;
		//GetModuleInformation(h_process, 
		pid = pi.dwProcessId;
		t_handle = pi.hThread;
		tid = pi.dwThreadId;
		process_name = get_process_name(argv[2]);
		cout << "PID: " << pid << endl;
		cout << "ProcessName: " << process_name << endl;
		//cout << "base" << hex << get_default_base_addr(argv[2]) << endl;
		if((base_addr=get_base_addr(tid)) == -1)
			return -1;
		SYMBOL_INFO sym;
		DWORD64 paddr;
		//cout << "sym: " << SymFromAddr(h_process, base_addr, &paddr,&sym) << endl;
		//cout << "Module: " << SymGetModuleBase64(h_process, base_addr) << endl;
		//cout << "Base Addr: " << hex << base_addr << endl;
		ResumeThread(pi.hThread);
		get_cmd_from_user(tid);
	}

	// attach process by PID
	else if(strcmp(argv[1],"-a") == 0) {
		// check if we can open the process
		HANDLE hProcess;
		DWORD pid = atoi(argv[2]);
		printf("pid: %d\n",pid);
		if( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) ) {
			fprintf(stderr, "Open Process Fail\n");
			return -1;
		}
		printf("Open Process Success\n");
		// attach
		if( !DebugActiveProcess(pid) ) {
			fprintf(stderr, "Attach Process Fail\n");
			return -1;
		}
		printf("Attach Process Success\n");
		//get_cmd_from_user(pid);
		// detach
		if( !DebugActiveProcessStop(pid) ) {
			fprintf(stderr, "Detach Process Fail\n");
			return -1;
		}
	}

	else {
		fprintf(stderr,"Wrong Options\n");
		fprintf(stderr,"%s -o <sample.exe>  or\n", argv[0]);
		fprintf(stderr,"%s -a <PID>\n", argv[0]);
		return -1;
	}

	return 0;
}