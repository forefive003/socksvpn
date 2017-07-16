
#ifndef _SIG_PROC_H
#define _SIG_PROC_H

#include <signal.h>

extern void sigproc(char *dumpFile);
extern void write_dumpinfo(int sig);
extern char g_dumpFile[256];

#endif
