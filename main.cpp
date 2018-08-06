

#include "log.h"

#include "on_handle.h"
#include <sstream>

void splitcmd( vector<string> &lst,string &cmd)
{
	stringstream ss(cmd);

    string partialArg;
	string fullArg;
	int isFullArg = 1;

	ss >> partialArg;
	lst.push_back(partialArg);

	while (ss >> partialArg) {

		if (partialArg.at(0) ==  '\"') {

			isFullArg = FALSE;

			if (partialArg.at(partialArg.length() - 1) !=  '\"') {

				fullArg.append(partialArg);
				fullArg.append( " ");
				continue;
			}
		}

		if (isFullArg == 0) {

			if (partialArg.at(partialArg.length() - 1) == '\"') {

				isFullArg = TRUE;
				fullArg.append(partialArg);
				fullArg.erase(0, 1);
				fullArg.erase(fullArg.length() - 1, 1);
				lst.push_back(fullArg);
			}
			else {

				fullArg.append(partialArg);
				fullArg.append( " ");
			}

			continue;
		}

		lst.push_back(partialArg);
	}
}
void on_help(const Cmds &cmds)
{
    cout << "debug command list:" << endl;
    cout << "\t s <name|pid> \t :open process by name or pid" << endl;
    cout << "\t e \t\t :exit" << endl;
    cout << "\t g \t\t :go" << endl;
    cout << "\t d [addr] [len]  :dump memory" << endl;
    cout << "\t i \t\t :step into" << endl;
    cout << "\t n \t\t :step over" << endl;
    cout << "\t o \t\t :step out" << endl;
    cout << "\t b [addr] | [l num]  :break point set" << endl;
    cout << "\t b [d addr | *] |    :break point del or del all" << endl;
    cout << "\t m [addr num] \t :show disasm code" << endl;
    cout << "\t l [num -berore +affter] :show source code" << endl;
    cout << "\t r \t\t :show register info" << endl;
    cout << "\t k \t\t :show callstack info" << endl;

    cout << "\t ps \t\t :show process list info" << endl;
    cout << "\t ns \t\t :show netstat info" << endl;
    cout << "\t lo \t\t :show local vars" << endl;
    cout << "\t gl \t\t :show global vars" << endl;
}
int parsecmd( vector<string>  & cmds) {

	typedef  struct {
		const char* cmd;
		void (*on_handle)(const vector<string> &);
	} cmdentry;

	  static cmdentry cmdMap[] = {

		{  "s", on_start},
		{  "e", on_stop },
		{  "g", on_go },
		{  "d", on_dump },
        {  "i", on_step_in },
		{  "n", on_step_over },
		{  "o", on_step_out },
		{  "b", on_break },
		{  "l", on_show_source },
        {  "m", on_show_asm    },
		{  "r", on_show_regs },
		{  "loc", on_show_locals },
		{  "glo", on_show_globals },
		{  "k", on_show_track },
		{  "ps", on_ps },
		{  "ns", on_netstat},
		{  "mode",on_mode},
		{  "help",on_help},
		{ NULL, NULL },
	};

	if (cmds.size() == 0) {
		return 1;
	}
	else if (cmds[0] ==  "q") {
		return 0;
	}

	for (int i = 0; cmdMap[i].cmd != NULL; ++i) {

		if (cmds[0] == cmdMap[i].cmd) {

			cmdMap[i].on_handle(cmds);
			return 1;
		}
	}

    cout <<  "Invalid command." <<  endl;
	return 1;
}

int main()
{
    cout << "\tWelcome to use Common Debugger by Coral.Stone! " << endl;

    string cmd;
    vector<string> cmds;
   // log_i("info");

    while(true)
    {
       cout << "$> ";

       getline(cin,cmd);

       splitcmd(cmds,cmd);

       if (!parsecmd(cmds)) return 0;

       cmds.clear();
    }
    return 0;
}
