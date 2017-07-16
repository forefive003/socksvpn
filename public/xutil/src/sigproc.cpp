
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "sigproc.h"

char g_dumpFile[256] = {0};

void write_dumpinfo(int sig)
{
    struct tm *tp;
	struct tm timestruct;
//	char tmp[128];
	char ttemp[128+48];
	FILE *plogfile;
	time_t gmt;
    char buf[256] = {0};
    char cmd[256] = {0};
    FILE *fh;
//    unsigned long filesize = -1;
    struct stat statbuff;

    if (sig == SIGINT)  /*如果是由于ctrl+c产生的信号则不记录信息*/
        return;

    stat(g_dumpFile, &statbuff);
    if (statbuff.st_size > 10000000) /*信号异常文件超过10M则清空*/
    {
        snprintf(cmd, sizeof(cmd), "echo > %s", g_dumpFile);
        cmd[255] = 0;
        if (-1 == system(cmd))
		{
			printf("system failed.");
		}
    }

    plogfile= fopen(g_dumpFile, "a+");   /*写入当前收到的异常信号类别和时间信息*/
    if (plogfile != NULL)
    {
        time(&gmt);
        tp = localtime_r(&gmt,&timestruct);

        snprintf(ttemp, 128,"\n\n\n%04d.%02d.%02d %02d:%02d:%02d LOG info: Receive signal, type:%d",
        			tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, sig);
        fprintf(plogfile, "%s\n", ttemp);
        fflush(plogfile);
        fclose(plogfile);
    }

    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());  /*写入当前各线程堆栈信息*/
    buf[255] = 0;

    if((fh = fopen(buf, "r")))
    {
        if(fgets(buf, sizeof(buf), fh))
        {
            if(buf[strlen(buf) - 1] == '\n')
            	buf[strlen(buf) - 1] = 0;

            snprintf(cmd, sizeof(cmd), "gdb %s %d -q -n -ex 'thread apply all bt' -batch >> %s", buf, getpid(), g_dumpFile);
            if (-1 == system(cmd))
            {
            	printf("system failed.");
			}
        }
        fclose(fh);
    }
}

static void urgency_process(int sig)
{
	static int mark=0;
	if (mark)
		return;//防止多个信号同时处理

	mark=1;

    write_dumpinfo(sig); /*写入堆栈信息*/

	exit(0);
}

void sigproc(char *dumpFile)
{
	struct sigaction act;

//	create_file(dumpFile);
	strncpy(g_dumpFile, dumpFile, sizeof(g_dumpFile));

	act.sa_handler = urgency_process;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGSTOP);
	sigaddset(&act.sa_mask, SIGABRT);
	sigaddset(&act.sa_mask, SIGILL);
	sigaddset(&act.sa_mask, SIGTERM);
	//sigaddset(&act.sa_mask, SIGQUIT);
	//sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGSEGV);

	sigaction(SIGILL, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	//sigaction(SIGQUIT, &act, NULL);
	//sigaction(SIGINT, &act, NULL);
	sigaction(SIGABRT, &act, NULL);
	sigaction(SIGSTOP, &act, NULL);

 //    /*忽略SIGPIPE*/
    //struct sigaction ign;
	// ign.sa_handler = SIG_IGN;
	// ign.sa_flags = 0;

	// sigemptyset(&ign.sa_mask);
	// sigaddset(&ign.sa_mask, SIGPIPE);
	// sigaction(SIGPIPE, &ign, NULL);
}
