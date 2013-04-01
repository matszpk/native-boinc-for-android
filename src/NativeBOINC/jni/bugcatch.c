/*
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <asm/user.h>
#include <jni.h>

extern char** environ;

static volatile int bugCatchIsReady = 0;

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_bugCatchExec(JNIEnv* env,
                jclass thiz, jstring progPathStr, jstring dirPathStr, jobjectArray argsArray) {
	int pid = 0;
	int waitTrials = 5;
	bugCatchIsReady = 0;
	pid = fork();
	if (pid == 0) { // in new process
		jsize i;
		const char* program = (*env)->GetStringUTFChars(env, progPathStr, 0);
		const char* dirPath = (*env)->GetStringUTFChars(env, dirPathStr, 0);
		jsize argsLength = (*env)->GetArrayLength(env, argsArray);
		jstring argStr;
		char** args;

		args = malloc(sizeof(char*)*(argsLength+2));

		args[0] = program;

		for (i = 0; i < argsLength; i++) {
			 argStr = (*env)->GetObjectArrayElement(env, argsArray, i);
			 args[i+1] = (*env)->GetStringUTFChars(env, argStr, 0);
		}
		args[argsLength+1] = NULL;

		chdir(dirPath);
		usleep(50000); // waiting for parent process for exec
		execv(program, args);
		exit(0);
	}
	return pid;
}

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_bugCatchExecSD(JNIEnv* env,
                jclass thiz, jstring progPathStr, jstring dirPathStr, jobjectArray argsArray) {
	int pid = 0;
	int waitTrials = 5;
	bugCatchIsReady = 0;
	pid = fork();
	if (pid == 0) { // in new process
		jsize i;
		const char* program = (*env)->GetStringUTFChars(env, progPathStr, 0);
		const char* dirPath = (*env)->GetStringUTFChars(env, dirPathStr, 0);
		jsize argsLength = (*env)->GetArrayLength(env, argsArray);
		jstring argStr;
		char** args;
		char** envp;
		size_t envnum, j;
		int found = 0;

		for (envnum = 0;environ[envnum] != NULL; envnum++);

		envp = malloc(sizeof(char*)*(envnum+2));

		for (j = 0;environ[j] != NULL; j++) {
			if (strncmp(environ[j],"LD_PRELOAD=",11) == 0) {
				// we replace if found
				envp[j] = "LD_PRELOAD=/data/data/sk.boinc.nativeboinc/lib/libexecwrapper.so";
				found = 1;
			} else
				envp[j] = environ[j];
		}

		if (!found)
			envp[envnum++] = "LD_PRELOAD=/data/data/sk.boinc.nativeboinc/lib/libexecwrapper.so";
		envp[envnum] = NULL;

		args = malloc(sizeof(char*)*(argsLength+2));

		args[0] = program;

		for (i = 0; i < argsLength; i++) {
			 argStr = (*env)->GetObjectArrayElement(env, argsArray, i);
			 args[i+1] = (*env)->GetStringUTFChars(env, argStr, 0);
		}
		args[argsLength+1] = NULL;

		chdir(dirPath);
		usleep(50000); // waiting for parent process for exec
		execve(program, args, envp);
		exit(0);
	}
	return pid;
}

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_bugCatchInit(JNIEnv* env,
		jclass thiz, jint pid)
{
	int initTrials = 5;
	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL)==-1)
		return -1;

	int options = PTRACE_O_TRACECLONE|PTRACE_O_TRACEVFORK|PTRACE_O_TRACEFORK|PTRACE_O_TRACEEXEC;

	do {
		usleep(1000);
		initTrials--;
	} while (initTrials != 0 && ptrace(PTRACE_SETOPTIONS, pid, 0, options) == -1);

	if (initTrials == 0)
		return -1;

	// continue tracee
	if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1)
		return -1;

	// init mkdir
	mkdir("/data/data/sk.boinc.nativeboinc/files/bugcatch",0755);
	return 0;
}

/**
 * reporting functions
 */

#define MME_NONE 0
#define MME_HEAP 1
#define MME_STACK 2
#define MME_VECTORS 3
#define MME_FILE 4

typedef struct {
    size_t start, end;
    size_t offset;
    mode_t mode;
    char filename[64];
    int type;
} MemMapEntry;

static MemMapEntry* readMemMaps(const char* filename, int* num)
{
    char line[128];
    FILE* file = NULL;
    MemMapEntry* mapents = NULL;
    int mentsCount = 0;
    int allocedMentsNum = 10;

    file = fopen(filename, "rb");
    if (file == NULL)
        return NULL;

    mapents = malloc(sizeof(MemMapEntry)*allocedMentsNum);

    while(fgets(line, 128, file)!=NULL)
    {
        MemMapEntry ment;
        char mode1,mode2,mode3,mode4;
        char devmaj, devmin;
        int inode;
        int linelen;
        int parsedElems;
        linelen = strlen(line);
        if (line[linelen-1] == '\n')
            line[linelen-1] = 0;

        parsedElems = sscanf(line, "%08zx-%08zx %c%c%c%c %08zx %02hhx:%02hhx %d %s",
               &ment.start, &ment.end, &mode1, &mode2, &mode3, &mode4, &ment.offset,
               &devmaj, &devmin, &inode, ment.filename);
        if (strcmp(ment.filename,"[stack]")==0)
        {
            ment.type = MME_STACK;
            ment.filename[0] = 0;
        }
        else if (strcmp(ment.filename,"[heap]")==0)
        {
            ment.type = MME_HEAP;
            ment.filename[0] = 0;
        }
        else if (strcmp(ment.filename,"[vectors]")==0)
        {
            ment.type = MME_VECTORS;
            ment.filename[0] = 0;
        }
        else if (parsedElems == 10)
        {
            ment.type = MME_NONE;
            ment.filename[0] = 0;
        }
        else
            ment.type = MME_FILE;

        ment.mode = ((mode1=='r')?1:0)+((mode2=='w')?2:0)+((mode3=='x')?4:0);


        if (mentsCount >= allocedMentsNum)
        {
            allocedMentsNum += (allocedMentsNum>>1);
            mapents = realloc(mapents, sizeof(MemMapEntry)*allocedMentsNum);
        }
        memcpy(mapents+mentsCount, &ment, sizeof(MemMapEntry));
        mentsCount++;
    }

    *num = mentsCount;
    fclose(file);
    return mapents;
}

#define MEMBUF_SIZE 4096

static void reportMemory(int reportfd, int memfd, off64_t start, off64_t end)
{
    off64_t p;
    unsigned char* buf;
    buf = malloc(MEMBUF_SIZE);
    if (buf == NULL)
    	return; // do nothing

    lseek64(memfd, start, SEEK_SET);

    for (p = start; p < end;)
    {
        int i,readed;
        int written, totalwritten;
        int toread = (end-p>=MEMBUF_SIZE)?MEMBUF_SIZE:end-p;
        readed = read(memfd, buf, toread);
        if (readed != toread)
        {
            free(buf);
            return;
        }
        // write it
        totalwritten = 0;
        while (totalwritten < toread)
        {
        	written = write(reportfd, buf+totalwritten, toread-totalwritten);
        	if (written == -1)
        	{ // error
        		free(buf);
        		return;
        	}
        	totalwritten += written;
        }
        p+=toread;
    }
    free(buf);
}

static void reportCmdLine(FILE* report_file, const char* cmdlinename)
{
	struct stat stbuf;
	int cmdlinefd;
	char* cmdline, *cmdlineend;
	char* it;

	if (stat(cmdlinename,&stbuf) == -1)
		return; // do nothing

	cmdline = malloc(stbuf.st_size);

	cmdlinefd = open(cmdlinename,O_RDONLY);
	if (cmdlinefd == -1)
	{
		free(cmdline);
		return;
	}
	if (read(cmdlinefd, cmdline, stbuf.st_size) != stbuf.st_size)
	{
		close(cmdlinefd);
		free(cmdline);
		return;
	}
	close(cmdlinefd);

	cmdlineend = cmdline + stbuf.st_size;
	fprintf(report_file, "CommandLine=");
	for (it = cmdline; it != cmdlineend && it-1 != cmdlineend;)
	{
		fprintf(report_file," \"%s\"",it);
		it += strlen(it)+1;
	}

	free(cmdline);
}

#define MAX_STACK_SIZE 0x8000

static int reportStupidBug(pid_t pid)
{
    int i;
    siginfo_t siginfo;
    char namebuf[256];
    int usevfp = 1;
    struct pt_regs regs;
    struct user_vfp vfp;
    int mentsCount;
    MemMapEntry* ments;
    size_t stackstart, stackend = 0;
    int memfd;

    time_t report_time;
    FILE* report_file = NULL;
    time(&report_time);

    snprintf(namebuf,256,"/data/data/sk.boinc.nativeboinc/files/bugcatch/%d_content",report_time);
    report_file = fopen(namebuf, "wb");
    if (report_file == NULL)
    	return -1;

    fputs("---- NATIVEBOINC BUGCATCH REPORT HEADER ----\n",report_file);

    fprintf(report_file,"BugReportId:%ld\n", report_time);
    // process info
    snprintf(namebuf,256,"/proc/%d/cmdline");
    reportCmdLine(report_file,namebuf);

    // signal info and CPU state
    if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo) == -1)
        return -1;

    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
        return -1;

    memset(&vfp, 0, sizeof(vfp));
    if (ptrace(PTRACE_GETVFPREGS, pid, NULL, &vfp) == -1)
        usevfp = 0;

    fprintf(report_file,"Pid:%d\nSignal:%d\nErrno:%d\nSignalCode:%d\nPointer:%p\nAddress:%p\n",
           pid, siginfo.si_signo, siginfo.si_errno,
           siginfo.si_code, siginfo.si_ptr, siginfo.si_addr);

    fputs("Registers:\n",report_file);
    for (i = 0; i < 18; i++)
        fprintf(report_file,"  reg%d:%08lx\n",i,regs.uregs[i]);

    if (usevfp)
    {
    	fputs("VFP Registers:\n",report_file);
        for (i = 0; i < 32; i++)
            fprintf(report_file,"  vfp%d:%016llx\n",i,vfp.fpregs[i]);
        fprintf(report_file,"  fpscr:%08lx\n",vfp.fpscr);
    }

    // memory map info
    snprintf(namebuf,64,"/proc/%d/maps",pid);
    ments = readMemMaps(namebuf, &mentsCount);

    stackstart = (regs.uregs[13]); // sp
    if (ments != NULL)
    {
        for (i = 0; i < mentsCount; i++)
        {
        	const char* mtypestr;
        	int mtype = ments[i].type;
            if (ments[i].start <= stackstart && ments[i].end > stackstart)
                stackend = ments[i].end;

            switch(mtype)
            {
            case MME_FILE:
            	mtypestr = "file";
            	break;
            case MME_STACK:
				mtypestr = "stack";
				break;
            case MME_HEAP:
            	mtypestr = "heap";
            	break;
            case MME_VECTORS:
            	mtypestr = "vectors";
            	break;
            case MME_NONE:
            	mtypestr = "none";
            	break;
            default:
            	mtypestr = "unknown";
            	break;
            }
            fprintf(report_file,"MRange:%08zx-%08zx %08zx\nMMode:%o,MType=%s,MFile:%s\n",
               ments[i].start, ments[i].end, ments[i].offset, ments[i].mode,
               mtypestr, ments[i].filename);
        }
        fflush(stdout);
    }
    free(ments);
    // memory stack

    snprintf(namebuf,64,"/proc/%d/mem",pid);
    memfd = open(namebuf,O_RDONLY);
    if (memfd != -1)
    {
        if (stackend!=0)
        {
        	int reportfd;
        	if (stackend-stackstart >= MAX_STACK_SIZE) // cut stack
        		stackend = stackstart += MAX_STACK_SIZE;
            fprintf(report_file,"Process stack:%08zx-%08zx\n",stackstart, stackend);

            snprintf(namebuf,256,"/data/data/sk.boinc.nativeboinc/files/bugcatch/%d_stack",report_time);
            reportfd = open(namebuf, O_WRONLY|O_CREAT);
            if (reportfd!=-1)
            {
            	reportMemory(reportfd, memfd, stackstart, stackend);
            	close(reportfd);
            }
        }
        close(memfd);
    }

    fclose(report_file);
    return 0;
}

/**
 * reporting functions
 */

#define EXIT_STATUS_MASK 0xff00
#define NORMAL_EXIT 0
#define SIGNALED_EXIT 0x100

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_bugCatchWaitForProcess(JNIEnv* env,
		jclass thiz, jint mainPid) {
	FILE* file;
	int status = 0;
	pid_t pid;
	do {
		pid = waitpid(0, &status, __WALL);
		if (pid == -1) {
			if (errno == EINTR) {
				jclass intrclazz = (*env)->FindClass(env, "java/lang/InterruptedException");
				(*env)->ThrowNew(env, intrclazz, "waitpid interrupted");
			}
			return -1;
		}

		if (WIFEXITED(status))
			printf("%d Exited with return %d\n", pid, WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			printf("%d killed by %d\n", pid, WTERMSIG(status));
		else if (WIFSTOPPED(status)) {
			printf("%d stopped by %d\n", pid, WSTOPSIG(status));
			if (WSTOPSIG(status) == SIGTRAP || WSTOPSIG(status) == SIGSTOP)
			{
				if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1)
				{
					perror("PTrace cont");
					return -1;
				}
			}
			else
			{
				int sigid = WSTOPSIG(status);
				if (sigid == SIGSEGV || sigid == SIGILL || sigid == SIGBUS || sigid == SIGFPE ||
					sigid == SIGABRT || sigid == SIGSTKFLT || sigid == SIGSYS)
					reportStupidBug(pid);

				if (ptrace(PTRACE_CONT, pid, NULL, WSTOPSIG(status)) == -1)
				{
					perror("PTrace cont");
					return -1;
				}
			}
		}
	} while (pid != mainPid || (!WIFEXITED(status) && !WIFSIGNALED(status)));
	// handle exits
	if (WIFEXITED(status))
		return NORMAL_EXIT | (WEXITSTATUS(status)&0xff);
	else if (WIFSIGNALED(status))
		return SIGNALED_EXIT | (WTERMSIG(status)&0xff);
	return -1;
}
