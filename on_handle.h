#pragma once

#include <string>
#include <vector>
#include <iostream>

using namespace std;

typedef vector<string> Cmds;

void on_start(const Cmds& cmds);
void on_stop(const Cmds& cmds);
void on_go(const Cmds& cmds);
void on_dump(const Cmds& cmds);
void on_break(const Cmds& cmds);
void on_step_in(const Cmds& cmds);
void on_step_over(const Cmds& cmds);
void on_step_out(const Cmds& cmds);

void on_show_regs(const Cmds& cmds);
void on_show_locals(const Cmds& cmds);
void on_show_globals(const Cmds& cmds);
void on_show_track(const Cmds& cmds);
void on_show_source(const Cmds& cmds);
void on_show_asm(const Cmds& cmds);

void on_ps(const Cmds& cmds);
void on_mode(const Cmds& cmds);
void on_netstat(const Cmds& cmds);
