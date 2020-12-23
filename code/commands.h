#ifndef COMMANDS_H
#define COMMANDS_H

#include "fs.h"

// format command
void format_c();

// ls command
void ls_c(const char*);

// mkdir command
void mkdir_c(const char*);

// touch command
void touch_c(const char*);

// cp command
void cp_c(const char*, const char*);

// exit command
void exit_c();

// tee command
void tee_c(const char*);

// cat command
void cat_c(const char*);

// help command
void help_c();

// stat command
void stat_c(const char*);

// execute a command
void exec(const char*);

#endif