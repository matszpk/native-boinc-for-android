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

#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <jni.h>

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_exec(JNIEnv* env,
                jclass thiz, jstring progPathStr, jstring dirPathStr, jobjectArray argsArray) {
	int pid = 0;
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
		execv(program, args);
	}
	return pid;
}

#define EXIT_STATUS_MASK 0xff00
#define NORMAL_EXIT 0
#define SIGNALED_EXIT 0x100

JNIEXPORT jint JNICALL Java_sk_boinc_nativeboinc_util_ProcessUtils_waitForProcess(JNIEnv* env,
		jclass thiz, jint pid) {
	int status = 0;
	if (waitpid(pid, &status, 0) == -1) {
		if (errno == EINTR) {
			jclass intrclazz = (*env)->FindClass(env, "java/lang/InterruptedException");
			(*env)->ThrowNew(env, intrclazz, "waitpid interrupted");
		}
		return -1;
	}



	if (WIFEXITED(status))
		return NORMAL_EXIT | (WEXITSTATUS(status)&0xff);
	else if (WIFSIGNALED(status))
		return SIGNALED_EXIT | (WTERMSIG(status)&0xff);
	return -1;
}
