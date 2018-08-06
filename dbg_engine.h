

#pragma once

#include "log.h"
#include "dbghelp.h"
#include "ps.h"
#include "asm.h"
#include "sym.h"
#include "ns.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>


#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define UL_PTR DWORD64
#else
#define UL_PTR DWORD
#endif // _IMAGEHLP64


#define ds_none 1
#define ds_ok  2
#define ds_int 3


#define BP_INIT 1       //初始断点
#define BP_CODE 2       //代码中的断点
#define BP_USER 3       //用户设置的断点
#define BP_STEP_OVER 4  //StepOver使用的断点
#define BP_STEP_OUT 5   //StepOut使用的断点

#define DBG_PASSED DBG_EXCEPTION_NOT_HANDLED
#define hex_out(str,val)   str << "=" << setw(8) << hex << setfill('0') << val << std::endl

typedef struct _bp{

	DWORD adr;     //断点地址
	BYTE  val;     //原指令第一个字节
  _bp(DWORD a=0,BYTE v=0):adr(a),val(v){}
  bool operator !=( _bp in)
  {
      return (in.adr != adr &&
              in.val != val);
  }
} break_point;

typedef struct {
   string file;
   DWORD  line;
   DWORD  addr;
}line_info;

enum dismode{
    mode_source,
    mode_asm
};

typedef vector<break_point> bps_t;
typedef bps_t::iterator     bps_iter;

typedef vector<line_info> lns_t;
typedef lns_t::iterator   lns_iter;

typedef map<DWORD,string> mods_t;

#define SS_NONE     0
#define SS_IN       1
#define SS_NEXT     2
#define SS_OUT      3
#define SS_INI      4
#define SS_NEXTI    5
#define SS_OUTI     6
typedef struct{
line_info   ln;
break_point bp;
int status;
DWORD last;
}dbg_step;


class dbg
{
	public:
	int status;
	HANDLE process,thread;
	int pid,tid;
	int dbg_status;
	private:
	dbg_step    step_info;


	bps_t bp_list;
	lns_t ln_list;
	//mods_t mod_list;
	string   image_name;
	dismode  show_state;
	public:
	  dbg():status(ds_none),process(0),thread(0),pid(0),tid(0)
	  {
		  dbg_status = DBG_PASSED;
		  show_state  = mode_asm;
	  }
	  ~dbg(){}

      void reset()
      {
          ::CloseHandle(thread);
          ::CloseHandle(process);
          thread=process=0;
          status = ds_none;
      }

	  void load_file(const char* file)
	  {
		STARTUPINFO   si = { 0 };
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi = { 0 };

		if (CreateProcess(
			file,NULL,NULL,NULL,FALSE,
			DEBUG_ONLY_THIS_PROCESS
			| CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
			NULL,NULL,
			&si,&pi) == FALSE) {

			cout <<  "ProcessBegin failed: " << file <<" error:"<< GetLastError() << endl;
			return;
			}

        pid = pi.dwProcessId;
        tid = pi.dwThreadId;

        #if 1
        process = pi.hProcess;
        thread  = pi.hThread;
        #else
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        process = open_process(PROCESS_ALL_ACCESS,FALSE,pid );
        thread  = open_thread(THREAD_ALL_ACCESS,FALSE,tid);
        #endif

        image_name = file;
        status = ds_ok;
        cout << "loaded file: " << file << endl;
	  }

      void load_pid(DWORD _pid)
      {
         if(!::DebugActiveProcess(_pid)) return;

         vector<DWORD> tl ;
         int n = get_thread_list(_pid,tl);

         pid = _pid;
         tid = tl[0];

         process = open_process(PROCESS_ALL_ACCESS,FALSE,pid );
         thread  = open_thread(THREAD_ALL_ACCESS,FALSE,tid);
         status  = ds_ok;
         cout << "loaded pid: " << pid << endl;
      }
      #define check_status() do {if(ds_none != status) {cout << "debug has runed !" << endl; return; } } while(0)
      void start(const char* val)
      {
          check_status();
          if(is_num_str(val))
            load_pid((DWORD)atoi(val));
          else
            load_file(val);

          //::WaitForInputIdle(process,WAIT_TIMEOUT);
          continue_dbg();
      }
	  void stop(){
		  if(::TerminateProcess(process,-1) == TRUE)
		  {
			  continue_dbg();
			  return;
		  }
		  cout << "stop failed :" << GetLastError() << endl;
	  }
      void run(BOOL go){

           dbg_status = go ? DBG_CONTINUE : DBG_PASSED ;
           continue_dbg();
      }

	  void continue_dbg(){
		  if(status == ds_none) return;

		  if(status == ds_ok) {
			  ::ResumeThread(thread);
		  }else{
			  ::ContinueDebugEvent(pid,tid,dbg_status);
		   //	  status = ds_ok;
		  }

		  DEBUG_EVENT event;
		  while(::WaitForDebugEvent(&event,WAIT_TIMEOUT)){
              if(!do_dbg_event(event))
                break;
              ::ContinueDebugEvent(pid,tid,dbg_status);
		  }
	  }

	  BOOL do_dbg_event(DEBUG_EVENT& event){
		  int code = event.dwDebugEventCode;
		  //cout << "on debug event: " << code << endl;

		  switch(code){
              case EXCEPTION_DEBUG_EVENT: // 1
              return on_debug(event);

              case CREATE_THREAD_DEBUG_EVENT:   // 2
              return on_thread_create(event);
			  case CREATE_PROCESS_DEBUG_EVENT:  //3
			  return on_process_create(event);

			  case EXIT_THREAD_DEBUG_EVENT:  //4
			  return on_exit_thread(event);
              case EXIT_PROCESS_DEBUG_EVENT:  //5
              return on_exit_process(event);

              case OUTPUT_DEBUG_STRING_EVENT:   // 8
              return on_output_string(event);

              case LOAD_DLL_DEBUG_EVENT:   // 6
              return on_load_dll(event,TRUE);
              case UNLOAD_DLL_DEBUG_EVENT: // 7
              return on_load_dll(event,FALSE);

			  default:
			  cout << "unknown debug event: " << code << endl;
		  }
		  return FALSE;
	  }


      #define out_except(rec) cout \
           << hex_out ("address",rec.ExceptionAddress) \
           << hex_out ("code",rec.ExceptionCode) \
           << hex_out ("flags",rec.ExceptionFlags) \
           << hex_out ("params",rec.NumberParameters)

      void on_break_point(EXCEPTION_DEBUG_INFO& edi){
           cout << "on break point " << edi. dwFirstChance << endl;
           EXCEPTION_RECORD & rec  = edi.ExceptionRecord;
           //out_except(rec);


           DWORD addr =  get_eip() ;
           addr -= 1 ; // 退回 中断IP
           if(has_bp(addr)){
                bp_unset(addr); // 恢复 中断点
                set_eip(addr);
           }

           break_point& bp = step_info.bp;
           if(bp.adr == (DWORD)rec.ExceptionAddress )
           {
               break_reset(bp);
               bp.adr = 0;
               bp.val = 0;
               set_eip(addr);
           }

           dbg_status = DBG_CONTINUE;
           status = ds_int;

      }

      void on_step_trap(EXCEPTION_DEBUG_INFO& edi){
/**/
           cout << "on step trap " << edi. dwFirstChance << endl;
           EXCEPTION_RECORD & rec  = edi.ExceptionRecord;
//         out_except(rec);

           dbg_status = DBG_CONTINUE;
           DWORD addr = (DWORD)rec.ExceptionAddress;
           if(step_info.status == SS_IN)
           {
              if(show_state == mode_source){
                  DWORD num =
                  from_addr_to_line(addr);
                 // same line
                 if(step_info.ln.line == num)
                 {
                     set_tf(true);
                     status = ds_ok;
                     return;
                 }

               }
           }

           if(step_info.last == addr){
            // like "rep xxx"
            set_tf(true);
            status = ds_ok;
            return ;
           }


           step_info.last = addr;

           if(show_state == mode_asm)
              display_asm(addr,1);
           if(show_state == mode_source)
           {
              DWORD num =
              from_addr_to_line(addr);
              display_source(num,0,1);
           }


           status = ds_int;
      }

      BOOL on_debug(DEBUG_EVENT& event)
      {
          EXCEPTION_DEBUG_INFO& edi = event.u.Exception;
          EXCEPTION_RECORD & rec  = edi.ExceptionRecord;
          switch (rec.ExceptionCode){
            case  EXCEPTION_BREAKPOINT:
                on_break_point(edi);
                break;
            case EXCEPTION_SINGLE_STEP:
                on_step_trap(edi);
                break;
            default:
                cout << "An exception was occured at ";
                cout << rec.ExceptionAddress << endl;
                cout << "Exception code:" ;
                cout << rec.ExceptionCode << endl;
                return FALSE;
          }
          if(status == ds_int)
            return FALSE;

          return TRUE;
      }

	  BOOL on_process_create(DEBUG_EVENT& event){

		if ( !::SymInitialize(process, NULL, FALSE))
        {
          return FALSE;
        }
        // for SymGetlinefromname
        SymSetOptions(SYMOPT_LOAD_LINES);

        //加载模块的调试信息
        CREATE_PROCESS_DEBUG_INFO& pdi = event.u.CreateProcessInfo;
        display_module(pdi.fUnicode, (DWORD)pdi.lpImageName);

        //get_module_name(pdi.fUnicode,)
		DWORD module = ::SymLoadModule(
			process,
			pdi.hFile,
			NULL,
			NULL,
			(DWORD)pdi.lpBaseOfImage,
			0);

		if (module != 0) {

			//SetBreakPointAtEntryPoint();
			DWORD addr = get_entry_addr();
			if(addr != 0){
				//set_bp(addr,TRUE);
				bp_set(addr);
				line_add(addr);
			}
		}
		else {
			 cout <<  "SymLoadModule64 failed: " << get_err() <<  endl;
		}

		::CloseHandle(pdi.hFile);
        ::CloseHandle(pdi.hFile);
		::CloseHandle(pdi.hThread);
	    ::CloseHandle(pdi.hProcess);
        //status = ds_int;
	    return TRUE;
	  }

      BOOL on_exit_process(DEBUG_EVENT& event)
      {
          DWORD code = event.u.ExitProcess.dwExitCode;
          cout << hex_out("process exit code",code);

          	//清理符号信息
          ::SymCleanup(process);

          reset();
          ::ContinueDebugEvent(pid,tid,DBG_CONTINUE);
          return FALSE;
      }

	  BOOL on_thread_create(DEBUG_EVENT& event)
	  {
          CREATE_THREAD_DEBUG_INFO &tdi = event.u.CreateThread;
          cout << "event create thread!" <<endl;

          //dbg_status = DBG_CONTINUE;
          return TRUE;
	  }

      BOOL on_exit_thread(DEBUG_EVENT& event)
      {
          DWORD code = event.u.ExitThread.dwExitCode;
          cout << hex_out("thread exit code",code);

          return FALSE;
      }
      BOOL  on_output_string(DEBUG_EVENT& event)
      {
          OUTPUT_DEBUG_STRING_INFO& osi = event.u.DebugString;
          DWORD   len     = osi.nDebugStringLength;
          const char* adr = osi.lpDebugStringData;
          if(len){
             void* data = alloca(len);
             read_memory((DWORD)adr,len,data);
             cout << "Debug msg: " << (const char*) data << endl;
          }

          return TRUE;
      }

      BOOL on_load_dll(DEBUG_EVENT& event,BOOL dll){
          if(dll){
              LOAD_DLL_DEBUG_INFO& ldi = event.u.LoadDll;
              DWORD modAddress = SymLoadModule(
                        process,
                        ldi.hFile,
                        NULL,
                        NULL,
                        (DWORD)ldi.lpBaseOfDll,0);

              if(modAddress==0)
                 return FALSE;

              display_module(event.u.LoadDll.fUnicode,
                         (DWORD)event.u.LoadDll.lpImageName);

          }else{

             SymUnloadModule(process, (DWORD)event.u.UnloadDll.lpBaseOfDll);

          }



          return TRUE;
      }
      void display_module(WORD unic , DWORD image)
      {
          if(image == 0) return ;

          char name[MAX_PATH];
          get_module_name(unic,image,name,MAX_PATH);

          cout  << "load " << name << endl;
      }

      int get_module_name(WORD unic , DWORD image, void *name,int len)
      {
          DWORD addr =0,size=0;
          size = read_memory(image,4, &addr);
          size = read_memory(addr,len,name);
          if(unic){
              char* buff = (char*)alloca(size);
              strcpy((char*)name,w2char((wchar_t*)name,buff));
          }

          return (int)size;
      }

      BOOL break_set(break_point& bp){

		  BYTE val=INT3;
		  DWORD addr = bp.adr;
		  read_memory(addr,1,&val);
          if(val != INT3){
            bp.val = val;
            val = INT3;
            write_memory(addr,1,&val);
            return TRUE;
          }
          return FALSE;
      }

      BOOL break_reset(break_point& bp){
          BYTE  val=0;
          DWORD addr = bp.adr;
		  read_memory(addr,1,&val);
		  if(val == INT3){
            val = bp.val;
            write_memory(addr,1,&val);
            return TRUE;
		  }
          return FALSE;
      }
/**/
      BOOL has_bp(DWORD addr)
      {
          break_point bp(addr);
          if(get_bp(bp))
            return TRUE;
          return FALSE;
      }

        void bp_del(DWORD addr,BOOL all)
        {
             if(all){
               bp_del_all();
               return;
             }

             break_point bp(addr);
             if(get_bp(bp)){
               break_reset(bp);
               pop_bp(bp);
             }
            cout << "del break at" << addr << endl;
        }

        void bp_del_all()
        {
          bps_iter itr = bp_list.begin();
          for(;itr!= bp_list.end();itr++){
               break_point &bp = *itr;
               break_reset(bp);
          }
          bp_list.clear();
        }

        void bp_set(DWORD addr)
        {
          break_point bp(addr);
          bool has = get_bp(bp);
          if(break_set(bp)){
             if(!has)
                push_bp(bp);
             cout << "set break at "<< std::hex << addr << endl;
          }
        }

	  void bp_unset(DWORD addr)
	  {
	      break_point bp(addr);
		  bool has = get_bp(bp);
		  if(has){
            break_reset(bp);
            cout << "reset break at " << std::hex ;
            cout <<  bp.adr <<":" << (DWORD)bp.val << endl;
		  }
	  }

      bool get_bp(break_point& bp) {
          bps_iter itr = bp_list.begin();
          for(;itr!= bp_list.end();itr++)
            if((*itr).adr == bp.adr){
                bp = *itr;
                return true;
            }
         return false;
      }

      void push_bp(break_point bp){
           bp_list.push_back(bp);
      }
      void pop_bp(break_point& bp){
          bps_iter itr = bp_list.begin();
          for(;itr!= bp_list.end();itr++)
            if((*itr).adr == bp.adr){
               bp_list.erase(itr);
               return ;
            }
      }

      void set_tf(bool v){
          CONTEXT ctx;
          get_thread_context(ctx);
          if(v)
            ctx.EFlags |= 0x100;
          else
            ctx.EFlags &= ~0x100;

          set_thread_context(ctx);
      }

      DWORD get_eip(){
          CONTEXT ctx;
          get_thread_context(ctx);
          return ctx.Eip;
      }

      void set_eip(DWORD addr){
          CONTEXT ctx;
          get_thread_context(ctx);
          ctx.Eip = addr;
          set_thread_context(ctx);
      }


	  DWORD get_entry_addr(){
		  const char* names[] = {"main","wmain","WinMain","wWinMain"};
		  SYMBOL_INFO si = { 0 };
	      si.SizeOfStruct = sizeof(SYMBOL_INFO);
		  int len = sizeof(names)/sizeof(const char*);
		  for(int i=0;i<len;i++){
			if (::SymFromName(process, names[i], &si) ) {
				return (DWORD)si.Address;
			}
		  }
		  return 0;
	  }

      void line_add(DWORD addr){
        lns_iter itr = ln_list.begin();
        for(;itr!= ln_list.end();itr++){
            if((*itr).addr == addr)
            {
                return ;
            }
        }

        line_info  ln;
        ln.addr = addr;
        if(get_line_by_addr(ln))
           ln_list.push_back(ln);

      }

      string line_at(DWORD addr){

        lns_iter itr = ln_list.begin();
        for(;itr!= ln_list.end();itr++){
            if((*itr).addr == addr)
            {
                line_info ln = *itr;
                return ln.file + ":" + itoa(ln.line,(char*)alloca(32),10);
            }
        }
         return "unknown file";
      }

      void line_del(WORD addr,BOOL all){
        if(all){
          ln_list.clear();
          return ;
        }

        lns_iter itr = ln_list.begin();
        for(;itr!= ln_list.end();itr++){
            if((*itr).addr == addr)
            {
                ln_list.erase(itr);
                break ;
            }
        }
        return;
      }

      BOOL check_retn(DWORD addr,DWORD& ret){
         BYTE  code[1];
         read_memory(addr,1,code);
         switch (code[0]){
            case 0xC3:
            case 0xCB:
                ret = 1 ; break;
            case 0xC2:
            case 0xCA:
                ret = 3 ; break;
            default:
                return FALSE;
         }
         return TRUE;
      }

      BOOL check_call(DWORD addr, DWORD& ret){
         BYTE  code[4];
         read_memory(addr,4,code);
         switch(code[0]){
            case 0xE8: ret = 5 ; break; // call
            case 0x9A: ret = 7 ; break; // call
            case 0xFF:
                switch(code[1]){
                    case 0x10:      // call ptr [EAX]
                    case 0x11:      // call ptr [ECX]
                    case 0x12:      // call ptr [EDX]
                    case 0x13:      // call ptr [EBX]
                    case 0x16:      // call ptr [ESI]
                    case 0x17:      // call ptr [EDI]
                    case 0xD0:      // call EAX
                    case 0xD1:      // call ECX
                    case 0xD2:      // call EDX
                    case 0xD3:      // call EBX
                    case 0xD4:      // call ESP
                    case 0xD5:      // call EBP
                    case 0xD6:      // call ESI
                    case 0xD7:      // call EDI
                        ret = 2 ; break;
                    case 0x14:      // call [reg*s + base]
                    case 0x50:      // call ptr [EAX + xx]
                    case 0x51:
                    case 0x52:
                    case 0x53:
                    case 0x54:
                    case 0x55:
                    case 0x56:
                    case 0x57:
                        ret = 3 ; break;
                    case 0x15:
                    case 0x90:     // call [EAX+xxxxxxx]
                    case 0x91:
                    case 0x92:
                    case 0x93:
                    case 0x95:
                    case 0x96:
                    case 0x97:
                        ret = 6 ; break;
                    case 0x94:
                        ret = 7;  break;
                    default:
                        return FALSE;
                }
                break;
            default:
                return FALSE;
         }
         return TRUE;
      }

      void step_set(int ss)
      {
          line_info ln;
          ln.addr = get_eip();
          if(get_line_by_addr(ln)){
            step_info.ln = ln;
          }
          step_info.status = ss;
      }

      void step_break_set(DWORD addr){
        break_point bp(addr);
        break_set(bp);

        break_point &
        old = step_info.bp;
        if( old.adr !=0 && bp != old)
        {
            break_reset(old);
        }
        old = bp;
      }

      void step_in(){
          step_set(SS_IN);
          set_tf(true);
          continue_dbg();
      }

      void step_over(){
          step_set(SS_NEXT);

          //检查当前指令是否CALL指令
          DWORD len,addr=get_eip();
          if(check_call(addr,len)){
            // 设置CALL 下一条指令为断点
            addr += len;
            step_break_set(addr);

            cout << "set call next addr " << addr << endl;

          }else{
            set_tf(true);
          }
          continue_dbg();

      }

      void step_out(){
          step_set(SS_OUT);

          DWORD addr;
          //get_ret_inst_addr(addr);
          get_call_ret_addr(addr);
          if(addr)
          {
              step_break_set(addr);
              cout << "set call ret addr " << addr << endl;
          }

          continue_dbg();
      }

      void get_ret_inst_addr(DWORD& addr)
      {   // 感觉不对谱？

            DWORD64 disp;
            SYMBOL_INFO symBol = { 0 };
            symBol.SizeOfStruct = sizeof(SYMBOL_INFO);

            if (SymFromAddr(
                process,
                get_eip(),
                &disp,
                &symBol) == FALSE) {
                log_i("SymFromAddr failed!");
                addr = 0;
                return ;
            }

            DWORD endaddr = symBol.Address + symBol.Size;
            addr = endaddr - 1;
            log_i(" addr %x size %x",symBol.Address, symBol.Size);
            DWORD len;
            if(check_retn(addr,len))
                return;

            addr = endaddr - 3;
            if(check_retn(addr,len))
                return;

            addr = 0;

      }

      void get_call_ret_addr(DWORD& addr)
      {
          CONTEXT ctx;
          get_thread_context(ctx);
          read_memory(ctx.Ebp+4,4,&addr);

      }

      DWORD from_line_to_addr(DWORD num){
           line_info ln;
           ln.addr = get_eip();
           get_line_by_addr(ln);
           ln.line = num;
           get_addr_by_line(ln);
           return ln.addr;
      }

      DWORD from_addr_to_line(DWORD addr){
          line_info ln;
          ln.addr = addr;
          get_line_by_addr(ln);
          return ln.line;
      }

	  BOOL get_addr_by_line(line_info& l){

         IMAGEHLP_LINE li = { 0 };
         li.SizeOfStruct = sizeof li;
         LONG pos ;
         DWORD line  = l.line;
         const char* name = l.file.c_str();
         if(FALSE == ::SymGetLineFromName(process,NULL,name,line,&pos,&li))
         {
           cout << dec   ;
           cout << image_name << " get address failed! error: "  << get_err() <<  endl;
           cout << name << ":" << line <<  endl;

           return FALSE;
         }
         l.addr = li.Address;
         if(pos != 0)
            return FALSE;
         return TRUE;
	  }

	  BOOL get_line_by_addr(line_info& l){

        //获取源文件以及行信息
        IMAGEHLP_LINE li = { 0 };
        li.SizeOfStruct = sizeof li ;
        DWORD pos = 0;
        DWORD addr =l.addr;
        if(!::SymGetLineFromAddr(process, addr, &pos,&li)) {

            DWORD code = GetLastError();
            string msg;
            switch (code) {
                // 126 表示还没有通过SymLoadModule64加载模块信息
                case 126:
                    msg = "Debug info in current module has not loaded." ;
                    break;
                // 487 表示模块没有调试符号
                case 487:
                    msg = "No debug info in current module." ;
                    break;

                default:
                    msg = "Unknown error: " ;
                    msg += itoa(code,(char*)alloca(64),10);
                    break;
            }
            cout << msg  << endl;
            return FALSE;
        }
        l.file = li.FileName;
        l.line = li.LineNumber;
        l.addr = addr;
        return TRUE;
	  }

	  DWORD read_memory(DWORD adr,DWORD len, void* data){
		  DWORD size = 0;
		  ::ReadProcessMemory(process, (const void*)adr, data, len, &size);
		  return size;
	  }

	  DWORD write_memory(DWORD adr,DWORD len, void* data){
		  DWORD size = 0;
		  ::WriteProcessMemory(process, (void*)adr, data, len, &size);
		  return size;
	  }

	  BOOL get_thread_context(CONTEXT& ctx){
          ctx.ContextFlags = CONTEXT_FULL;
	     return ::GetThreadContext(thread,&ctx);
	  }

	  BOOL set_thread_context(CONTEXT& ctx){
	     return ::SetThreadContext(thread,&ctx);
	  }

      DWORD get_current_addr(int offset)
      {
        CONTEXT ctx;
        BOOL r = get_thread_context(ctx);
        if(!r)
        {
           log_i("thread context error:%d",get_err());
           return 0;
        }

        DWORD addr = ctx.Eip + offset;
        return addr;
      }
      char  byte_to_char(char c)
      {
         if(c>=0x20 && c<=0x7e )
            return c;
         return '.';
      }

      int is_num_str(const char* str)
      {
          int len = strlen(str);
          for(int i=0;i<len;i++)
            if(str[i] < 0x30 || str[i] > 0x39 )
                return 0;
          return 1;
      }
      #define SA_END "   "
      #define SH_END "   "
      #define SH_WITHE "   "
	  void display_data(DWORD addr,DWORD len){


        char cl[17];
		DWORD bl = addr % 16;
		DWORD start = addr - bl;

		std::ostringstream out;
		out << std::hex << std::uppercase;

		DWORD i=0,j=0,k=0;
		// first space;
		if(bl > 0)
		{
			out << std::setw(8)<< std::setfill('0');
			out << start << SA_END;
			start += 16;


			for( i=0,j=0;i<bl;i++,j++){
			   out <<  SH_WITHE;
			   cl[i] = ' ';
			}
		}

		for(k=0;k<len;k++){
			DWORD cur_addr = addr +  k;
			// addr area
			if(j % 16 == 0)
			{
				j = 0;
			    out << std::setw(8)<< std::setfill('0');
			    out << start << SA_END;
				start += 16;
			}
			// hex area
			char byte = '.';

			if(read_memory(cur_addr,1,&byte)){
               int d = byte;
               d &= 0xff;
			   out << std::setw(2) << std::setfill('0');
			   out << d << ' ';
			}
			else {
               out << "?? ";
			}

            cl[i] = byte_to_char(byte);
			i++;

			// char area
			if(i% 16 == 0)
			{
				cl[i] = 0;
				i = 0;

				out << SH_END << cl << std::endl;
			}
			j++;

		}

		// last space
		if(j%16 != 0 ) {
			for (;j<16;j++){
				out << SH_WITHE ;
			}
		}
		if(i%16 !=0){
		   cl[i]=0;
		   out << SH_END << cl << std::endl;
		}

        cout << out.str() ;
	  }


	  void display_regs(CONTEXT& ctx)
	  {
		 ostringstream out;
		 out << std::hex << std::uppercase;

		 out << hex_out("EFL",ctx.EFlags);
		 out << hex_out("EAX",ctx.Eax);
		 out << hex_out("EBX",ctx.Ebx);
		 out << hex_out("ECX",ctx.Ecx);
		 out << hex_out("EDX",ctx.Edx);
		 out << hex_out("ESI",ctx.Esi);
		 out << hex_out("EDI",ctx.Edi);

		 out << hex_out("EBP",ctx.Ebp);
		 out << hex_out("ESP",ctx.Esp);

		 out << hex_out("EIP",ctx.Eip);

		 DWORD flag = ctx.EFlags;
		 if(flag&0x1)   out << "CF ";
		 if(flag&0x4)   out << "PF ";
		 if(flag&0x10)  out << "AF ";
		 if(flag&0x40)  out << "ZF ";
		 if(flag&0x80)  out << "SF ";
		 if(flag&0x100) out << "TF ";
		 if(flag&0x200) out << "IF ";
		 if(flag&0x400) out << "DF ";
		 if(flag&0x800) out << "OF ";
		 out << std::endl;

         cout << out.str()  ;
	  }

      void display_breaks()
      {
         int i=0;
         for(;i<bp_list.size();i++){
            cout << i << "  break at " << bp_list[i].adr ;
            cout << " :" << line_at(bp_list[i].adr) <<endl;
         }
         if (i==0)
            cout << "No break points" <<endl;
      }


	  void display_source(int num,int before,int after)
	  {

        line_info li;
        li.addr = get_eip();
        if(!get_line_by_addr(li))
           return;

        if(num == 0)
            num = li.line;


        display_lines(li.file,num,before,after);

	  }


	  void display_lines(string& file,int num,int before,int after){


        line_info ln;
        ln.file = file;

        if(after < before)
        std::swap(after,before);

        int line_start = num  +  before;
        int line_end   = num  +  after;
        if (line_start <1) line_start=1;

        std::ifstream is(file.c_str());
        if(is.fail()){
        cout << "file open failed. " << file << endl;
        return;
        }
        cout << dec << file << ":" << num << "," << before << ",+" << after << endl;
        std::string text;
        for(int i=1;i<line_end;i++){
            if(!std::getline(is,text))
                break;
            // 跳过不显示的行
            if(i<line_start)
                continue;

            // 显示地址
            cout << std::setw(6) << std::setfill(' ');
            ln.line = i;
            if(get_addr_by_line(ln))
              cout << std::hex << ln.addr;
            else
              cout << " ";

            // 显示当前位置
            if(i == num)
                cout << " ->";
            else
                cout << "   ";

            // 显示行号
            cout <<std::setw(4)<< std::setfill(' ') ;
            cout << std::dec << i << "  ";
            // 显示内容
            cout <<text << endl;

        }
	  }


	  void display_asm(DWORD addr,DWORD len)
	  {
	      ulong i, l,offset;
          t_dasm da;
          t_masm am;
          #if 0
	      const  char* buff  ;
          ideal=1; lowercase=1; putdefseg=0;
          //buff = "\x81\x05\xE0\x5A\x47\x00\x01\x00\x00\x00\xcc\xcc";
          buff = "\xf3\xab";
          l=DecAsm((void*)buff,strlen(buff),0x400000,&da,DISASM_CODE);
          printf("%3i  %-24s  %-24s   (MASM)\n",l,da.dump,da.text);

          ideal=0; lowercase=0;
          l=DecAsm((void*)buff,strlen(buff),0x400000,&da,DISASM_CODE);
          printf("%3i  %-24s  %-24s   (MASM)\n",l,da.dump,da.text);

          buff ="ADD [DWORD 475AE0],1";
          printf("%s:\n",buff);
          l=EncAsm(buff,strlen(buff),0x400000,&am,0,0);
          printf("%3i %-24s \n",l,am.dump);
          #else
          CONTEXT ctx={0};
          get_thread_context(ctx);
          DWORD cur = ctx.Eip;

          DWORD size = len*16;
          char* buff = (char*)alloca(size);
          if(!read_memory(addr,size,buff))
             return;

          cout << std::hex << std::setfill(' ');
          l=offset = 0;
          for(i=0;i<len;i++){

            l = DecAsm(buff+offset,size-offset,addr+offset,&da,DISASM_CODE);
            if(l==0)
                break;


            cout << "  " ;
            cout << std::setw(8) << std::uppercase << addr +offset  << "  ";
            cout << std::setw(16)<< da.dump << "  ";
            cout << da.text << endl;

            offset+=l;

          }
          #endif
	  }


       static BOOL CALLBACK enum_module_proc(PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext) {

        mods_t * mods = (mods_t*)UserContext;

        (*mods)[(DWORD)ModuleBase] = ModuleName;

        return TRUE;
        }

	   int get_module_map(mods_t& mods){


         EnumerateLoadedModules(process,enum_module_proc,&mods);
         return mods.size();

	  }

      #define M_X86 IMAGE_FILE_MACHINE_I386
	  #define stack_walk(p,t,f,c) \
      ::StackWalk(M_X86,p,t,f,c,0,::SymFunctionTableAccess,::SymGetModuleBase,0)

	  void display_track()
	  {
	      CONTEXT ctx;
	      get_thread_context(ctx);


	      STACKFRAME frame = {
          /*frame.AddrPC    =*/ {ctx.Eip,0,AddrModeFlat},
          /*frame.AddrReturn=*/ {ctx.Eax,0,AddrModeFlat},
          /*frame.AddrFrame =*/ {ctx.Ebp,0,AddrModeFlat},
          /*frame.AddrStack =*/ {ctx.Esp,0,AddrModeFlat},
	      };

          #define SYM_INFO  IMAGEHLP_SYMBOL
	      typedef struct _STACK_INFO{
	       SYM_INFO sym;
	       char buf[MAX_PATH];
	      }STACK_INFO;

          STACK_INFO si={0};
	      SYM_INFO* psi = (SYM_INFO*)&si;
          psi->SizeOfStruct= sizeof (*psi);
          psi->MaxNameLength  = MAX_PATH;

          mods_t mods;
          get_module_map(mods);
          while(stack_walk(process,thread,&frame,&ctx))
          {
              UL_PTR ip =  frame.AddrPC.Offset;
              UL_PTR disp = 0;

              if(frame.AddrFrame.Offset == 0)
                break;

              if(!::SymGetSymFromAddr(process,ip,&disp,psi))
                break;
              //::SymFromAddr(process,ip,&disp,psi);

              cout << hex;
              cout << "" << psi->Address << "  ";


              DWORD modBase = (DWORD)SymGetModuleBase(process, ip);
              const string & modName = mods[modBase];
              if(modName.length()>0)
                cout << modName;

              cout << "@" <<psi->Name ;
              cout <<  " ";


              IMAGEHLP_LINE line={0};
              line.SizeOfStruct = sizeof(line);
              if(!::SymGetLineFromAddr(process, ip, &disp, &line))
              {

                  cout << endl ;
                  break;
              }
              cout << dec ; //<< "file: ";
              cout << line.FileName  << ":" ;
              cout << line.LineNumber<< " " ;
              cout << endl;

          }
	  }

      typedef struct _var_info{
         DWORD  id;
         UL_PTR addr;
         DWORD  symsize;
         UL_PTR mod;
         char name[MAX_PATH];
      }var_info;
      typedef vector<var_info> var_list_t;
      typedef struct _var_context_t
      {
        var_list_t vars;
        DWORD      base;
      }var_context_t;

      #define DUMP_SYM_INFO(psi) \
           cout << hex_out("typeid",psi->TypeIndex); \
           cout << hex_out("name",psi->Name); \
           cout << hex_out("addr",psi->Address); \
           cout << hex_out("size",psi->Size);

      static BOOL CALLBACK enum_vars_proc(SYMBOL_INFO* psi,ULONG len, void* pud)
      {

          var_context_t *ctx = (var_context_t*)pud;


          if(psi->Tag != SymTagData) {
             cout << "other Enum Tag:" << psi->Tag << endl;
             return FALSE;
          }

          //DUMP_SYM_INFO(psi);

          DWORD offset = ctx->base;
          if((psi->Flags & SYMFLAG_REGREL)==0)
              offset = 0;

          var_info vi = {0};
          vi.addr = psi->Address + offset;
          vi.id   = psi->TypeIndex;
          vi.mod  = psi->ModBase;
          vi.symsize=len;
          //if(psi->NameLen>0)
          strcpy(vi.name,psi->Name);

          ctx->vars.push_back(vi);

          return TRUE;
      }

      DWORD get_frame_offset(){

          CONTEXT ctx;
          get_thread_context(ctx);

          DWORD64 disp;
	      SYMBOL_INFO si = { 0 };
	      si.SizeOfStruct = sizeof(si);

          ::SymFromAddr(process,ctx.Eip,&disp,&si);

		  if(disp == 0)
             return (DWORD)(ctx.Esp-4);
          else
             return (DWORD)(ctx.Ebp);

      }

      void display_vars(var_list_t & vars)
      {

          Sym sym(process,0);
          char type[MAX_PATH];
          char value[MAX_PATH];
          for(int i=0;i<vars.size();i++){
             var_info& vi = vars[i];
             sym.mod  = vi.mod;
             DWORD id = vi.id;
             DWORD addr = vi.addr;
             DWORD len  = vi.symsize;

             //log_i("addr=%x,size=%d",addr,len);
             //sym.dump(id);
             cout << "<" << addr << "," << len <<">" <<endl;
             sym.getTypeName(id,type);
             cout << type << "   " << vi.name;
             cout << "="  ;

            // continue;
             void* data = alloca(len);
             read_memory(addr,len,data);
             sym.getValue(id,value,addr,data);
             cout << value << endl;
          }
      }

	  void show_local_var(const char* expr)
	  {

          UL_PTR addr = get_eip();

          //设置栈帧
	      IMAGEHLP_STACK_FRAME frame = { 0 };
	      frame.InstructionOffset = addr;

          if(!::SymSetContext(process,&frame,0)){
            if(get_err()!=ERROR_SUCCESS){
                    cout << "SymSetContext error:" << get_err() << endl;

               return ;
            }
          }

          var_context_t ctx;
          ctx.base = get_frame_offset();

          if(!::SymEnumSymbols(process,0,expr,enum_vars_proc,&ctx))
          {
              cout << "no vars be found expr=" << expr << endl;
              return ;
          }

          display_vars(ctx.vars);

	  }

	  void show_global_var(const char* expr)
	  {
          UL_PTR addr = get_eip();

          UL_PTR mod ;
          mod = SymGetModuleBase(process,addr);

          var_context_t ctx;
          ctx.base = get_frame_offset();

          if(!::SymEnumSymbols(process,mod,expr,enum_vars_proc,&ctx))
          {
              cout << "no vars be found expr=" << expr << endl;
              return ;
          }

          display_vars(ctx.vars);
	  }

	  void set_mode(dismode mode){
	     show_state = mode;
	  }
};
