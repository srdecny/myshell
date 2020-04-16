#pragma once
#include "datastructs.h"

void process_line(struct line_s *line);
void process_command(struct command_s *cmd);
void internal_cd(char *location);
void exit_shell();