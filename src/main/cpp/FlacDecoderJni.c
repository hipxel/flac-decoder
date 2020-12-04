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

typedef struct {
	size_t bufferLength;
	jbyte *buffer;
	hipxel_FlacDecoder *decoder;
} hipxel_FlacDecoderJni;

static hipxel_FlacDecoderJni *hipxel_FlacDecoderJni_new(hipxel_DataReader dataReader) {
	hipxel_FlacDecoderJni *fdj = malloc(sizeof(hipxel_FlacDecoderJni));
	fdj->bufferLength = 0;
	fdj->buffer = NULL;
	fdj->decoder = hipxel_FlacDecoder_new(dataReader);
	return fdj;
}

static void hipxel_FlacDecoderJni_delete(hipxel_FlacDecoderJni *fdj) {
	hipxel_FlacDecoder_delete(fdj->decoder);
	free(fdj->buffer);
	free(fdj);
}

static jbyte *hipxel_FlacDecoderJni_prepareTemporaryBuffer(
		hipxel_FlacDecoderJni *fdj, size_t length) {
	if (length <= fdj->bufferLength)
		return fdj->buffer;

	jbyte *p = realloc(fdj->buffer, length * sizeof(jbyte));
	if (NULL == p)
		return NULL;

	fdj->buffer = p;
	fdj->bufferLength = length;
	return fdj->buffer;
}

JNIEXPORT jobject JNICALL
Java_com_hipxel_flac_FlacDecoder_create(JNIEnv *env, jobject thiz, jobject dataReader) {
	hipxel_DataReader jdr = hipxel_JavaDataReader_create(env, dataReader);
	hipxel_FlacDecoderJni *ptr = hipxel_FlacDecoderJni_new(jdr);

	if (!ptr->decoder->initialized) {
		hipxel_FlacDecoderJni_delete(ptr);
		return NULL;
	}

	return (*env)->NewDirectByteBuffer(env, ptr, sizeof(ptr));
}

JNIEXPORT void JNICALL
Java_com_hipxel_flac_FlacDecoder_release(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacDecoderJni_delete(ptr);
}

JNIEXPORT jboolean JNICALL
Java_com_hipxel_flac_FlacDecoder_step(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return (jboolean) hipxel_FlacDecoder_step(ptr->decoder);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_read(JNIEnv *env, jobject thiz,
                                      jobject pointer, jbyteArray buffer, jlong length) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);

	jbyte *tmpBuffer = hipxel_FlacDecoderJni_prepareTemporaryBuffer(ptr, (size_t) length);
	if (NULL == tmpBuffer)
		return -1;

	jlong ret = (jlong) (hipxel_FlacDecoder_read(ptr->decoder, tmpBuffer, length));

	if (ret <= 0)
		return ret;

	(*env)->SetByteArrayRegion(env, buffer, 0, (jsize) (ret), tmpBuffer);

	return ret;
}

JNIEXPORT void JNICALL
Java_com_hipxel_flac_FlacDecoder_seekTo(JNIEnv *env, jobject thiz,
                                        jobject pointer, jlong position) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacDecoder_seekTo(ptr->decoder, position);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getSampleRate(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getSampleRate(ptr->decoder);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getChannelsCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getChannelsCount(ptr->decoder);
}

JNIEXPORT jint JNICALL
Java_com_hipxel_flac_FlacDecoder_getBitsPerSample(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getBitsPerSample(ptr->decoder);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getTotalSamplesCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getTotalSamplesCount(ptr->decoder);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getPcmFramesPosition(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getPcmFramesPosition(ptr->decoder);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacDecoder_getBytesReadyCount(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacDecoderJni *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return hipxel_FlacDecoder_getBytesReadyCount(ptr->decoder);
}
