/*
 * Copyright (C) 2020 Janusz Jankowski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JavaDataReader.h"

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	JNIEnv *env;
	bool wasDetached;
} hipxel_jni;

static JavaVM *getVM(JNIEnv *env) {
	JavaVM *vm = NULL;
	(*env)->GetJavaVM(env, &vm);
	return vm;
}

static JNIEnv *getAttachedJNIEnv(JavaVM *jvm, bool *outWasDetached) {
	JNIEnv *env;

	if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6) == JNI_EDETACHED) {
		*outWasDetached = true;
		(*jvm)->AttachCurrentThread(jvm, &env, NULL);
	} else {
		*outWasDetached = false;
	}
	return env;
}

static hipxel_jni prepareJni(JavaVM *jvm) {
	hipxel_jni jni;
	jni.wasDetached = false;
	if (NULL == jvm) {
		jni.env = NULL;
	} else {
		jni.env = getAttachedJNIEnv(jvm, &(jni.wasDetached));
	}
	return jni;
}

static void releaseJni(hipxel_jni jni, JavaVM *jvm) {
	if (jni.wasDetached)
		(*jvm)->DetachCurrentThread(jvm);
}

typedef struct {
	JavaVM *jvm;
	jobject javaObject;

	jmethodID mid_read;
	jmethodID mid_getSize;
	jmethodID mid_release;

	jbyteArray tmpBuffer;
	jint tmpLength;
} hipxel_JavaDataReader;

static jbyteArray newGlobalByteArray(JNIEnv *env, jint length) {
	jbyteArray arr = (*env)->NewByteArray(env, length);
	jbyteArray globalRef = (jbyteArray) (*env)->NewGlobalRef(env, arr);
	(*env)->DeleteLocalRef(env, arr);
	return globalRef;
}

static hipxel_JavaDataReader *hipxel_JavaDataReader_new(JNIEnv *env, jobject dataReader) {
	hipxel_JavaDataReader *jdr = malloc(sizeof(hipxel_JavaDataReader));

	jdr->jvm = getVM(env);
	if (NULL == jdr->jvm)
		return jdr;

	jdr->javaObject = (*env)->NewGlobalRef(env, dataReader);
	jdr->tmpLength = 1;
	jdr->tmpBuffer = newGlobalByteArray(env, jdr->tmpLength);

	jclass cls = (*env)->FindClass(env, "com/hipxel/flac/DataReader");
	jdr->mid_read = (*env)->GetMethodID(env, cls, "read", "(JJ[B)J");
	jdr->mid_getSize = (*env)->GetMethodID(env, cls, "getSize", "()J");
	jdr->mid_release = (*env)->GetMethodID(env, cls, "release", "()V");
	(*env)->DeleteLocalRef(env, cls);

	return jdr;
}

static void cleanup_(hipxel_JavaDataReader *jdr, JNIEnv *env) {
	if (!(*env)->ExceptionCheck(env))
		(*env)->CallVoidMethod(env, jdr->javaObject, jdr->mid_release);

	(*env)->DeleteGlobalRef(env, jdr->javaObject);
	(*env)->DeleteGlobalRef(env, jdr->tmpBuffer);
}

static void hipxel_JavaDataReader_delete(hipxel_JavaDataReader *jdr) {
	hipxel_jni jni = prepareJni(jdr->jvm);
	JNIEnv *env = jni.env;

	if (NULL != env)
		cleanup_(jdr, env);

	releaseJni(jni, jdr->jvm);

	free(jdr);
}

static jbyteArray prepareBuffer(hipxel_JavaDataReader *jdr, JNIEnv *env, jint length) {
	if (length <= jdr->tmpLength)
		return jdr->tmpBuffer;

	(*env)->DeleteGlobalRef(env, jdr->tmpBuffer);

	jdr->tmpBuffer = newGlobalByteArray(env, length);
	return jdr->tmpBuffer;
}

static int64_t read_(hipxel_JavaDataReader *jdr, JNIEnv *env,
                     int64_t position, int64_t length, void *buffer) {
	jbyteArray jb = prepareBuffer(jdr, env, (jint) length);

	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	jlong ret = (*env)->CallLongMethod(env, jdr->javaObject, jdr->mid_read, position, length, jb);

	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	if (ret < 0)
		return ret;

	(*env)->GetByteArrayRegion(env, jb, 0, (jsize) ret, (jbyte *) buffer);
	return ret;
}

static int64_t hipxel_JavaDataReader_read(void *p, int64_t position, int64_t length, void *buffer) {
	hipxel_JavaDataReader *jdr = (hipxel_JavaDataReader *) p;

	hipxel_jni jni = prepareJni(jdr->jvm);
	JNIEnv *env = jni.env;

	int64_t ret = -1;
	if (NULL != env)
		ret = read_(jdr, env, position, length, buffer);

	releaseJni(jni, jdr->jvm);

	return ret;
}

static int64_t getSize_(hipxel_JavaDataReader *jdr, JNIEnv *env) {
	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	jlong ret = (*env)->CallLongMethod(env, jdr->javaObject, jdr->mid_getSize);

	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	return ret;
}

static int64_t hipxel_JavaDataReader_getSize(void *p) {
	hipxel_JavaDataReader *jdr = (hipxel_JavaDataReader *) p;

	hipxel_jni jni = prepareJni(jdr->jvm);
	JNIEnv *env = jni.env;

	int64_t ret = -1;
	if (NULL != env)
		ret = getSize_(jdr, env);

	releaseJni(jni, jdr->jvm);

	return ret;
}

static void jdr_release(void *p) {
	hipxel_JavaDataReader_delete((hipxel_JavaDataReader *) p);
}

hipxel_DataReader hipxel_JavaDataReader_create(JNIEnv *env, jobject dataReader) {
	hipxel_DataReader v;
	v.read = hipxel_JavaDataReader_read;
	v.getSize = hipxel_JavaDataReader_getSize;
	v.release = jdr_release;
	v.p = hipxel_JavaDataReader_new(env, dataReader);
	return v;
}
