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

#include <sys/stat.h>
#include <jni.h>

JNIEXPORT jboolean JNICALL Java_sk_boinc_nativeboinc_util_Chmod_chmod(JNIEnv* env,
                jclass thiz, jstring pathStr, jint mode)
{
  const char* path = (*env)->GetStringUTFChars(env, pathStr, 0);
  int status = 0;
  status = chmod(path, mode);
  (*env)->ReleaseStringUTFChars(env, pathStr, path);
  return (status>=0) ? 1 : 0;
}
