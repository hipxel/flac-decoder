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

#ifndef HIPXEL_FLACDECODER
#define HIPXEL_FLACDECODER

#include "DataReader.h"

#include <stdbool.h>

struct hipxel_GrowingBuffer;

typedef struct hipxel_FlacDecoder {
	hipxel_DataReader reader;
	struct hipxel_GrowingBuffer *growingBuffer;
	void *internalDecoder;

	int64_t sourceLength;
	int64_t currentOffset;

	int64_t requestedSamplePosition;
	int64_t bytesWrittenSinceRequest;

	bool calledWrite;
	bool endOfFile;
	bool finished;
	bool initialized;

	bool gotStreamInfo;

	struct {
		uint64_t totalSamplesCount;
		uint32_t sampleRate;
		uint32_t channelsCount;
		uint32_t bitsPerSample;
	} info;
} hipxel_FlacDecoder;

hipxel_FlacDecoder *hipxel_FlacDecoder_new(hipxel_DataReader reader);

void hipxel_FlacDecoder_delete(hipxel_FlacDecoder *fd);

bool hipxel_FlacDecoder_step(hipxel_FlacDecoder *fd);

int64_t hipxel_FlacDecoder_read(hipxel_FlacDecoder *fd, void *buffer, int64_t length);

void hipxel_FlacDecoder_seekTo(hipxel_FlacDecoder *fd, int64_t position);

int64_t hipxel_FlacDecoder_getPcmFramesPosition(hipxel_FlacDecoder *fd);

int64_t hipxel_FlacDecoder_getBytesReadyCount(hipxel_FlacDecoder *fd);

inline static uint32_t hipxel_FlacDecoder_getSampleRate(hipxel_FlacDecoder *fd) {
	return fd->info.sampleRate;
}

inline static uint32_t hipxel_FlacDecoder_getChannelsCount(hipxel_FlacDecoder *fd) {
	return fd->info.channelsCount;
}

inline static uint32_t hipxel_FlacDecoder_getBitsPerSample(hipxel_FlacDecoder *fd) {
	return fd->info.bitsPerSample;
}

inline static uint64_t hipxel_FlacDecoder_getTotalSamplesCount(hipxel_FlacDecoder *fd) {
	return fd->info.totalSamplesCount;
}

#endif // HIPXEL_FLACDECODER
