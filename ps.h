
#include <vector>

#pragma once

#define PF_TID 0x1
#define PF_MID 0x2
void display_process_list(DWORD flag);
int  get_thread_list(DWORD pid,std::vector<DWORD> &tl);
HANDLE open_thread(DWORD dwFlag,BOOL  bInherit ,DWORD tid);
HANDLE open_process(DWORD dwFlag,BOOL  bInherit ,DWORD pid);
DWORD  get_process_name(DWORD pid,char*name,bool isfull);
