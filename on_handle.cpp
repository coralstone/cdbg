

#include "on_handle.h"
#include "dbg_engine.h"


dbg dbger;

void on_start(const Cmds& cmds)
{
    static string name;

    if(cmds.size() >= 2)
       name = cmds[1];

	if(name.empty())
    { cout << "file not find "<< endl; return;}

	dbger.start(name.c_str());
}

void on_stop(const Cmds& cmds)
{
	dbger.stop();
}
void on_go(const Cmds& cmds)
{
   BOOL go = TRUE;
   if(cmds.size() == 2)
      if(cmds[1] == "c")
        go = FALSE;

    dbger.run(go);
}
void on_dump(const Cmds& cmds)
{


    DWORD addr = 0;
    DWORD len = 128;
    if(cmds.size() ==1)
        addr = dbger.get_current_addr(0);
    if(cmds.size() >1)
        addr = strtoul(cmds[1].c_str(),NULL,16);
    if(cmds.size() >2)
        len  = strtoul(cmds[2].c_str(),NULL,16);

    dbger.display_data(addr,len);
}

void on_mode(const Cmds& cmds){
   if(cmds.size() == 1)
    return;
   if(cmds[1]== "source")
    dbger.set_mode(mode_source);
   if(cmds[1]== "asm" )
    dbger.set_mode(mode_asm);

}
void on_break(const Cmds& cmds)
{
     if(cmds.size() == 1)
        return dbger.display_breaks();

     DWORD addr = 0;
     BOOL del = 0,all = 0,line=0;
     if(cmds.size() >= 2)
     {
         if(cmds[1] == "d")
            {
                del = 1;
                if(cmds[2] == "*")
                    all = 1;
                else
                    addr = strtol(cmds[2].c_str(), NULL, 16);
            }
         else if(cmds[1] == "l")         {
            line = 1;
            if(line && cmds.size() >= 3)
            {
             DWORD num = strtol(cmds[2].c_str(), NULL, 10);
             addr = dbger.from_line_to_addr(num);
            }
         }
         else
          addr = strtol(cmds[1].c_str(), NULL, 16);

     }



     if(del)
     {
         dbger.bp_del(addr,all);
         dbger.line_del(addr,all);
         return;
     }


      dbger.bp_set(addr);
      dbger.line_add(addr);
}
void on_step_in(const Cmds& cmds)
{
     dbger.step_in();
}
void on_step_over(const Cmds& cmds)
{
     dbger.step_over();
}
void on_step_out(const Cmds& cmds)
{
     dbger.step_out();
}

void on_show_regs(const Cmds& cmds)
{
    CONTEXT ctx;
    BOOL r = dbger.get_thread_context(ctx);
    if(!r) {log_i("thread context error:%d",get_err()); return;}

    dbger.display_regs(ctx);
}
void on_show_locals(const Cmds& cmds)
{
	if (cmds.size() >= 2)
		dbger.show_local_var( cmds[1].c_str() );

}
void on_show_globals(const Cmds& cmds)
{
	if (cmds.size() >= 2)
		dbger.show_global_var( cmds[1].c_str() );
}
void on_show_track(const Cmds& cmds)
{
   dbger.display_track();
}
void on_netstat(const Cmds& cmds)
{
   display_netstat();
   //get_process_name(0,0,false);
}

void on_show_source(const Cmds&cmds)
{
    int before = -5;
    int after  = 5;
    int num    = 0;
    if(cmds.size() >1)
        num = atoi(cmds[1].c_str());
    if (num < 0) {
        before = num;
        num =0;
    }


    if(cmds.size() > 2)
    {
        int  tmp  = atoi(cmds[2].c_str());

        if(cmds.size() == 3){
            if(tmp<0)
              before = tmp;
            else
              after  = tmp;
            }else{
              before = tmp;
              tmp  = atoi(cmds[3].c_str());
              if(tmp>0)
                after = tmp;
            }
    }


    dbger.display_source(num,before,after);
}

void on_show_asm(const Cmds &cmds)
{
    DWORD addr = 0;
    DWORD len = 10;
    if(cmds.size() ==1)
        addr = dbger.get_current_addr(0);
    if(cmds.size() >1)
        addr = strtoul(cmds[1].c_str(),NULL,16);
    if(cmds.size() >2)
        len  = strtoul(cmds[2].c_str(),NULL,16);

    dbger.display_asm(addr,len);
}
void on_ps(const Cmds&cmds)
{
   if(cmds.size() == 1)
      display_process_list(0);
  // if(cmds.size() == 2 && cmds[1] == "-t")
   DWORD flag=0;
   for(int i=1;i<cmds.size();i++){
      if(cmds[i] == "-t")
        flag |= PF_TID;
      if(cmds[i] == "-m")
        flag |= PF_MID;
   }
   display_process_list(flag);
}
