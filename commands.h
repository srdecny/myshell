#pragma once
#include "tailq.h"

void process_command(struct cmd_s *cmd);
void internal_cd(char *location);
void exit_shell();