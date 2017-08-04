#define SW_BP  0
#define HW_BP  1
#define MEM_BP 2

#define HW_ACCESS  'r'
#define HW_WRITE   'w'
#define HW_EXECUTE 'e'

class BreakPoint {
public:
	BreakPoint(UINT id, UINT addr) {
		this->id = id;
		this->addr = addr;
		this->times = 0;
	}
	/*
	void set_id(UINT id) {
		this->id = id;
	}*/
	UINT get_id() const {
		return id;
	}
	/*
	void set_addr(UINT addr) {
		this->addr = addr;
	}*/
	UINT get_addr() const {
		return addr;
	}
	void set_times(UINT times) {
		this->times = times;
	}
	UINT get_times() const {
		return times;
	}
	void set_type(int type) {
		this->type = type;
	}
	int get_type() const {
		return type;
	}
	virtual void list_breakpoint(){
	};
private:
	UINT id;
	UINT addr;
	UINT times;
	int type; //SW_BP, HW_BP, MEM_BP
};


class SoftBreak: public BreakPoint {
public:
	SoftBreak(UINT id, UINT addr, char orig):BreakPoint(id, addr){
		this->set_type(SW_BP);
		this->orig = orig;
	}
	char get_orig() const {
		return orig;
	}
	virtual void list_breakpoint() {
		// Num    Type    Addr   Hit_Times
		cout << setiosflags(ios::left) << setw(8) << get_id() << setw(20) << "breakpoint" << "0x" << setw(14) << hex << get_addr() << setw(10) << get_times() << endl;
	}
private:
	char orig;
};
ostream& operator<< (ostream& out, const SoftBreak& t);

class HardBreak: public BreakPoint {
public:
	HardBreak(UINT id, UINT addr, int len, char cond):BreakPoint(id, addr) {
		this->set_type(HW_BP);
		this->len = len;
		this->cond = cond;
	}
	int get_len() const {
		return len;
	}
	char get_cond() const {
		return cond;
	}
	virtual void list_breakpoint() {
		string bp_type;
		switch (get_cond()) {
		case 'r':
			bp_type = "read ";
			break;
		case 'w':
			bp_type = "write ";
			break;
		case 'e':
			bp_type = "exec ";
			break;
		}
		bp_type += "watchpoint";
		// Num    Type    Addr   Hit_Times
		cout << setiosflags(ios::left) << setw(8) << get_id() << setw(20) << bp_type << "0x" << setw(14) << hex << get_addr() << setw(10) << get_times() << endl;
	}
private:
	int len;	//1, 2, 4
	char cond;	//read, write, execute
};
ostream& operator<< (ostream& out, const HardBreak& t);

class MemBreak: public BreakPoint {
public:
	MemBreak(UINT id, UINT addr, UINT len):BreakPoint(id, addr) {
		this->set_type(MEM_BP);
		this->len = len;
	}
	UINT get_len() const {
		return len;
	}
	virtual void list_breakpoint() {
	}
private:
	UINT len;
};
ostream& operator<< (ostream& out, const MemBreak& t);

class BreakPointList{
public:
	BreakPointList(){
		id = 0;
		last_breakpoint = NULL;
	}
	void set_software_bp(UINT addr);
	void set_hardware_bp(UINT addr, UINT len, char cond, DWORD threadID);
	void set_memory_bp(UINT addr, UINT len);
	void list_breakpoints();
	BOOL delete_bp_by_id(UINT id);
	BOOL add_hb_inner(LPCONTEXT lpctx, HardBreak* hb);
	BOOL delete_hb_inner(int slot, LPCONTEXT lpctx);
	BOOL is_sb_duplicate(UINT addr);
	BOOL is_hb_duplicate(UINT addr, char cond);
	BreakPoint * get_bp_by_addr(UINT addr);
	HardBreak * get_hb_by_index(int index);
	void set_last_breakpoint(BreakPoint *);
	BreakPoint * get_last_breakpoint();
	
private:
	UINT id;
	list<BreakPoint*> bp_list;
	map<int,HardBreak*> hb_map;
	BreakPoint * last_breakpoint;
};