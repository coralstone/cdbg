

#include "log.h"
#include <vector>

#include <tlhelp32.h>

#include <iostream>
#include <iomanip>



typedef struct _thread_entry {
    DWORD dwSize;
    DWORD cntUsage;
    DWORD tid;  // 线程ID
    DWORD pid;  // 此线程所属进程的进程ID
    LONG tpBasePri;  // 优先级 0到31
    LONG tpDeltaPri; // 已经废除
    DWORD dwFlags;   // 已经废除
    } thread_entry;

typedef struct _process_entry
{
    DWORD dwSize;
    DWORD cntUsage;   // 此进程的引用计数
    DWORD pid;        // 进程ID
    ULONG_PTR th32DefaultHeapID; // 进程默认堆ID
    DWORD mid;   // 模块ID
    DWORD cntThreads;
    DWORD ppid;  // 父进程
    LONG pcPriClassBase; // 线程优先权
    DWORD dwFlags;  // 保留
    TCHAR szExeFile[MAX_PATH];
} process_entry;

typedef struct _module_entry {
	DWORD dwSize;
	DWORD mid;
	DWORD pid;
	DWORD GlblcntUsage;
	DWORD ProccntUsage;
	BYTE *mAddr;
	DWORD mSize;
	HMODULE hModule;
	char szModule[256];
	char szExePath[MAX_PATH];
}module_entry;


#define SNAP_HEAP       0x1 // 包含所有的堆
#define SNAP_PID        0x2 // 包含所有的进程
#define SNAP_TID        0x4 // 包含所有的线程
#define SNAP_MID        0x8 // 包含所有的模块
#define SNAP_ALL        (SNAP_HEAP|SNAP_PID|SNAP_TID|SNAP_MID)
#define SNAP_INHERIT  0x80000000  // 声明快照句柄是可继承

#define CreateSnapshot    CreateToolhelp32Snapshot
#define FirstProcess      Process32First
#define NextProcess       Process32Next
#define FirstThread       Thread32First
#define NextThread        Thread32Next

#define FirstModule       Module32First
#define NextModule        Module32Next

typedef std::vector<thread_entry> thread_list;
thread_list query_thread_list(HANDLE hSnap,DWORD pid)
{
   thread_list tl;
   thread_entry te = {0};
   te.dwSize = sizeof te;
   BOOL status = FirstThread(hSnap,(LPTHREADENTRY32)&te);
   while (status){
       if(pid == te.pid)
          tl.push_back(te);
	   status = NextThread(hSnap,(LPTHREADENTRY32)&te);
   }
   return tl;
}


DWORD  get_process_name(DWORD pid,char*name,bool isfull);
DWORD  get_process_user(DWORD pid,char*user);
void display_process_list(DWORD flag)
{
   HANDLE hSnap = CreateSnapshot(SNAP_PID|SNAP_TID,0); // 所有进程
   if(hSnap == (HANDLE)-1)
   {
	   log_i("error(%d):create process snapshot failed.",get_err());
	   return;
   }

   process_entry pe = {0};
   pe.dwSize = sizeof pe;

   BOOL status = FirstProcess(hSnap,(LPPROCESSENTRY32)&pe);

   #define OUT_PRINT(val)   std::cout << std::setw(4)<< val <<"  "

   std::cout << std::left ;
   OUT_PRINT("pid");  OUT_PRINT("ppid"); OUT_PRINT("tids");
   OUT_PRINT("user");
   std::cout << std::endl;

   while (status){

	   OUT_PRINT( pe.pid );
	   OUT_PRINT( pe.ppid);
	   OUT_PRINT( pe.cntThreads );

	   char user[MAX_PATH]={0};
	   char name[MAX_PATH]={0};
	   get_process_user(pe.pid,user);
	   std::cout << std::setw(16)  << user;

	   strcpy(name,pe.szExeFile);
	   if(flag & 0x2)
         get_process_name(pe.pid,name,true);

	   std::cout << name << std::endl;

	   thread_list tl = query_thread_list(hSnap,pe.pid);
	   if(tl.size() && (flag & 0x1)) {
         std::cout << "                  ~ ";
         int i=0;
	     for(i=0;i<tl.size() && i < 8;i++)
            std::cout <<   std::setw(4) << tl[i].tid << "  ";
         if(i<tl.size())
            std::cout << "...";
         std::cout << std::endl;
	   }
	   status = NextProcess(hSnap,(LPPROCESSENTRY32)&pe);
   }
   ::CloseHandle(hSnap);
}

int  get_thread_list(DWORD pid,std::vector<DWORD> &otl)
{
   HANDLE hSnap = CreateSnapshot(SNAP_PID|SNAP_TID,pid); // 所有进程
   if(hSnap == (HANDLE)-1)
   {
	   log_i("error(%d):create process snapshot failed.",get_err());
	   return 0;
   }

   thread_list tl = query_thread_list(hSnap,pid);
   for(int i=0;i<tl.size();i++)
      otl.push_back(tl[i].tid);

   ::CloseHandle(hSnap);
   return tl.size();
}

HANDLE open_process(DWORD dwFlag,BOOL  bInherit ,DWORD pid)
{
   HANDLE              hProcess;
   hProcess = ::OpenProcess(dwFlag,bInherit,pid);
   return hProcess;
}

typedef HANDLE (*OPENTHREAD) (DWORD dwFlag, BOOL bInherit, DWORD tid);
HANDLE open_thread(DWORD dwFlag, BOOL  bInherit , DWORD tid)
{
    HANDLE  hThread;
    HMODULE hmod = ::LoadLibrary("Kernel32.dll");
    OPENTHREAD OpenThread = (OPENTHREAD)::GetProcAddress(hmod, "OpenThread");
    hThread = OpenThread(dwFlag,bInherit,tid);
    ::FreeLibrary(hmod);
    return hThread;
}

DWORD get_process_user(DWORD pid,char* user)
{
    HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,pid);
    HANDLE hToken;
    if(!::OpenProcessToken(hProcess,TOKEN_QUERY,&hToken)) {
        ::CloseHandle(hProcess);
        return 0;
    }
    TOKEN_INFORMATION_CLASS cls =  TokenUser;
    DWORD size;
    ::GetTokenInformation(hToken,cls,0,0,&size);

    TOKEN_USER *pToken = (TOKEN_USER*)alloca(size);

    DWORD ulen = 0;
    if(::GetTokenInformation(hToken,cls,pToken,size,&size))
    {

        SID_NAME_USE nu;
        char domain[MAX_PATH];
        DWORD dlen = ulen = MAX_PATH;
        ::LookupAccountSid(NULL,pToken->User.Sid,
                           user,&ulen,domain,&dlen,&nu);


    }

    ::CloseHandle(hToken);
    ::CloseHandle(hProcess);
    return ulen;
}

DWORD get_process_name(DWORD pid,char* name,bool isfull)
{
   name[0]=0;
   HANDLE hSnap = CreateSnapshot(SNAP_PID,0); // 所有进程
   if(hSnap == (HANDLE)-1)
   {
	   log_i("error(%d):create process snapshot failed.",get_err());
	   return 0;
   }

    process_entry pe={0};
    pe.dwSize = sizeof pe;
    BOOL status = FirstProcess(hSnap,(LPPROCESSENTRY32)&pe);
    while (status){
       if(pid == pe.pid)
        {
           strcpy(name,pe.szExeFile);
           break;
       }
      status =  NextProcess(hSnap,(LPPROCESSENTRY32)&pe);
    }

   ::CloseHandle(hSnap);
   return 1;
}

