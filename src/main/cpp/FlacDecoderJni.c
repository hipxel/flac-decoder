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

#include "FlacDecoder.h"
#include "JavaDataReader.h"

#include <jni.h>
#include <stdlib.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
	return JNI_VERSION_1_6;
}

JNIEXPORT jobject JNICALL
Java_com_hipxel_flac_FlacDecoder_create(JNIEnv *env, jobject thiz, jobject dataReader) {
	hipxel_DataReader jdr = hipxel_JavaDataReader_create(env, dataReader);
	hipxel_FlacDecoder *ptr = hipxel_FlacDecoder_new(jdr);

	if (!ptr->initialized) {
		hipxel_FlacDecoder_delete(ptr);
		return NULL;
	}

	return (*env)->NewDirectByteBuffer(env, ptr, sizeof(ptr));
}

JNIEXPORT void JNICALL
Java_com_hipxel_flac_FlacDecoder_release(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacDecoder_delete(ptr);
}

JNIEXPORT jboolean JNICALL
Java_com_hipxel_flac_FlacDecoder_step(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return (jboolean) hipxel_FlacDecoder_step(ptr);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_read(JNIEnv *env, jobject thiz,
                                      jobject pointer, jbyteArray buffer, jlong length) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return (hipxel_FlacDecoder_readJni(ptr, env, buffer, length));
}

JNIEXPORT void JNICALL
Java_com_hipxel_flac_FlacDecoder_seekTo(JNIEnv *env, jobject thiz,
                                        jobject pointer, jlong position) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacDecoder_seekTo(ptr, position);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getSampleRate(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getSampleRate(ptr);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getChannelsCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getChannelsCount(ptr);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getBitsPerSample(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getBitsPerSample(ptr);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getTotalSamplesCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getTotalSamplesCount(ptr);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getPcmFramesPosition(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getPcmFramesPosition(ptr);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getBytesReadyCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getBytesReadyCount(ptr);
}
