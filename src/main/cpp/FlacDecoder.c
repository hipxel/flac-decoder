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

#include "GrowingBuffer.h"

#include <FLAC/stream_decoder.h>

#include <android/log.h>

#include <stdlib.h>
#include <string.h>

#define HIPXEL_LOG_ERROR(...) \
    ((void)__android_log_print(ANDROID_LOG_ERROR, "FlacDecoder", __VA_ARGS__))

static inline void leftShiftCopy(int16_t *dst, const FLAC__int32 *const buffer[],
                                 unsigned int framesCount, unsigned int channelsCount,
                                 unsigned int bitShift) {
	for (unsigned int i = 0; i < framesCount; ++i) {
		for (unsigned int c = 0; c < channelsCount; ++c) {
			*dst++ = (int16_t) (buffer[c][i] << bitShift);
		}
	}
}

static inline void rightShiftCopy(int16_t *dst, const FLAC__int32 *const buffer[],
                                  unsigned int framesCount, unsigned int channelsCount,
                                  unsigned int bitShift) {
	for (unsigned int i = 0; i < framesCount; ++i) {
		for (unsigned int c = 0; c < channelsCount; ++c) {
			*dst++ = (int16_t) (buffer[c][i] >> bitShift);
		}
	}
}

static FLAC__StreamDecoderReadStatus readCallback(
		const FLAC__StreamDecoder *decoder,
		FLAC__byte buffer[], size_t *bytes,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	hipxel_DataReader *reader = &(fd->reader);
	int64_t got = reader->read(reader->p, fd->currentOffset, *bytes, buffer);

	if (got < 0) {
		*bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	if (0 == got) {
		*bytes = 0;
		fd->endOfFile = true;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}

	*bytes = (size_t) got;
	fd->currentOffset += got;
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderSeekStatus seekCallback(
		const FLAC__StreamDecoder *decoder,
		FLAC__uint64 absolute_byte_offset,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	fd->currentOffset = absolute_byte_offset;
	fd->endOfFile = false;
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus tellCallback(
		const FLAC__StreamDecoder *decoder,
		FLAC__uint64 *absolute_byte_offset,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	int64_t p = fd->currentOffset;
	*absolute_byte_offset = (uint64_t) (0 > p ? 0 : p);
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

static FLAC__StreamDecoderLengthStatus lengthCallback(
		const FLAC__StreamDecoder *decoder,
		FLAC__uint64 *stream_length,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	int64_t size = fd->sourceLength;
	if (size >= 0) {
		*stream_length = (uint64_t) size;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	} else {
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	}
}

static FLAC__bool eofCallback(
		const FLAC__StreamDecoder *decoder,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	return fd->endOfFile;
}

static FLAC__StreamDecoderWriteStatus writeCallback(
		const FLAC__StreamDecoder *decoder,
		const FLAC__Frame *frame, const FLAC__int32 *const buffer[],
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	fd->calledWrite = true;

	uint32_t channelsCount = fd->info.channelsCount;
	unsigned framesCount = frame->header.blocksize;
	uint64_t bytesCount = framesCount * channelsCount * sizeof(int16_t);

	int16_t *p = (int16_t *) hipxel_GrowingBuffer_claimForWrite(fd->growingBuffer, bytesCount);
	if (bytesCount > 0 && (NULL == p))
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	// output 16 bits as most of android audio stack works with it
	int needLeftShift = 16 - fd->info.bitsPerSample;
	if (needLeftShift >= 0) {
		leftShiftCopy(p, buffer, framesCount, channelsCount, (unsigned) needLeftShift);
	} else {
		rightShiftCopy(p, buffer, framesCount, channelsCount, (unsigned) (-needLeftShift));
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadataCallback(
		const FLAC__StreamDecoder *decoder,
		const FLAC__StreamMetadata *metadata,
		void *client_data) {
	hipxel_FlacDecoder *fd = (hipxel_FlacDecoder *) client_data;

	if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO) {
		HIPXEL_LOG_ERROR("excessive STREAMINFO, type: %d", (int) metadata->type);
		return;
	}

	if (fd->gotStreamInfo)
		return;

	FLAC__StreamMetadata_StreamInfo i = metadata->data.stream_info;

	fd->info.totalSamplesCount = i.total_samples;
	fd->info.sampleRate = i.sample_rate;
	fd->info.channelsCount = i.channels;
	fd->info.bitsPerSample = i.bits_per_sample;

	fd->gotStreamInfo = true;
}

static void errorCallback(
		const FLAC__StreamDecoder *decoder,
		FLAC__StreamDecoderErrorStatus status,
		void *client_data) {
	HIPXEL_LOG_ERROR("got error from FLAC decoder: %d", (int) status);
	// process_single will return false anyway if error was critical
}

static void reset(hipxel_FlacDecoder *fd, bool init) {
	fd->finished = true;

	FLAC__StreamDecoder *decoder = (FLAC__StreamDecoder *) fd->internalDecoder;
	if (NULL == decoder)
		return;

	fd->currentOffset = 0;
	fd->endOfFile = false;

	hipxel_GrowingBuffer_clear(fd->growingBuffer);
	fd->requestedSamplePosition = 0;
	fd->bytesWrittenSinceRequest = 0;

	if (!init) {
		if (!FLAC__stream_decoder_reset(decoder)) {
			HIPXEL_LOG_ERROR("reset failed");
			return;
		}
	}

	if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder)) {
		HIPXEL_LOG_ERROR("metadata processing failed");
		return;
	}

	if (!fd->gotStreamInfo) {
		HIPXEL_LOG_ERROR("no FLAC's STREAMINFO");
		return;
	}

	if (fd->info.channelsCount <= 0) {
		HIPXEL_LOG_ERROR("invalid channels count: %d", fd->info.channelsCount);
		return;
	}

	if (fd->info.bitsPerSample > 32 || fd->info.bitsPerSample < 8) {
		HIPXEL_LOG_ERROR("invalid bits per sample: %d", fd->info.bitsPerSample);
		return;
	}

	fd->finished = false;
}

static void init(hipxel_FlacDecoder *fd) {
	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
	if (NULL == decoder) {
		fd->finished = true;
		HIPXEL_LOG_ERROR("couldn't create decoder");
		return;
	}
	fd->internalDecoder = decoder;

	FLAC__stream_decoder_set_md5_checking(decoder, false);
	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(
			decoder, FLAC__METADATA_TYPE_STREAMINFO);

	FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(
			decoder,
			readCallback, seekCallback, tellCallback,
			lengthCallback, eofCallback, writeCallback,
			metadataCallback, errorCallback,
			(void *) fd);

	if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fd->finished = true;
		HIPXEL_LOG_ERROR("FLAC decoder init failed %d", (int) initStatus);
		return;
	}

	reset(fd, true);
	if (!fd->finished)
		fd->initialized = true;
}

bool hipxel_FlacDecoder_step(hipxel_FlacDecoder *fd) {
	FLAC__StreamDecoder *decoder = (FLAC__StreamDecoder *) fd->internalDecoder;
	if (NULL == decoder)
		return false;

	if (fd->endOfFile)
		return false;

	if (fd->finished)
		return false;

	fd->calledWrite = false;
	fd->finished = !(FLAC__stream_decoder_process_single(decoder));

	if (!fd->calledWrite)
		fd->finished = true;

	return !fd->finished;
}

int64_t hipxel_FlacDecoder_read(hipxel_FlacDecoder *fd, void *buffer, int64_t length) {
	int64_t red = hipxel_GrowingBuffer_consume(fd->growingBuffer, buffer, length);

	if (red < 0)
		return red;

	fd->bytesWrittenSinceRequest += red;
	return red;
}

static void slowSeekTo(hipxel_FlacDecoder *fd, int64_t position) {
	int64_t reqByte = position * fd->info.channelsCount * sizeof(int16_t);

	if (fd->bytesWrittenSinceRequest > reqByte) {
		reset(fd, false);
		if (fd->finished)
			return;
	}

	while (fd->bytesWrittenSinceRequest < reqByte) {
		int64_t diff = reqByte - fd->bytesWrittenSinceRequest;

		int64_t rc = hipxel_GrowingBuffer_getLength(fd->growingBuffer);
		int64_t toTake = rc < diff ? rc : diff;

		hipxel_GrowingBuffer_discard(fd->growingBuffer, toTake);
		fd->bytesWrittenSinceRequest += toTake;

		if (hipxel_GrowingBuffer_getLength(fd->growingBuffer) <= 0) {
			if (!hipxel_FlacDecoder_step(fd)) {
				return;
			}
		}
	}
}

void hipxel_FlacDecoder_seekTo(hipxel_FlacDecoder *fd, int64_t position) {
	FLAC__StreamDecoder *decoder = (FLAC__StreamDecoder *) fd->internalDecoder;
	if (NULL == decoder)
		return;

	fd->finished = false;

	if (position > fd->info.totalSamplesCount)
		position = fd->info.totalSamplesCount;

	if (position < 0)
		position = 0;

	if (fd->sourceLength < 0) {
		// libFLAC doesn't support seeking on files with unknown length, so seek manually
		slowSeekTo(fd, position);
		return;
	}

	if (!FLAC__stream_decoder_seek_absolute(decoder, (uint64_t) position)) {
		HIPXEL_LOG_ERROR("failed seek");
	} else {
		hipxel_GrowingBuffer_clear(fd->growingBuffer);
		fd->requestedSamplePosition = position;
		fd->bytesWrittenSinceRequest = 0;
	}
}

int64_t hipxel_FlacDecoder_getPcmFramesPosition(hipxel_FlacDecoder *fd) {
	if (fd->info.channelsCount == 0)
		return 0;

	int64_t offsetInPcmFrames =
			fd->bytesWrittenSinceRequest / (fd->info.channelsCount * sizeof(int16_t));
	return fd->requestedSamplePosition + offsetInPcmFrames;
}

int64_t hipxel_FlacDecoder_getBytesReadyCount(hipxel_FlacDecoder *fd) {
	return hipxel_GrowingBuffer_getLength(fd->growingBuffer);
}

hipxel_FlacDecoder *hipxel_FlacDecoder_new(hipxel_DataReader reader) {
	hipxel_FlacDecoder *fd = malloc(sizeof(hipxel_FlacDecoder));

	fd->reader = reader;
	fd->growingBuffer = hipxel_GrowingBuffer_new();
	fd->internalDecoder = NULL;

	fd->sourceLength = -1;
	fd->currentOffset = 0;

	fd->requestedSamplePosition = 0;
	fd->bytesWrittenSinceRequest = 0;

	fd->calledWrite = false;
	fd->endOfFile = false;
	fd->finished = false;

	fd->gotStreamInfo = false;

	fd->info.totalSamplesCount = 0;
	fd->info.sampleRate = 0;
	fd->info.channelsCount = 0;
	fd->info.bitsPerSample = 0;

	fd->sourceLength = reader.getSize(reader.p);

	fd->initialized = false;
	init(fd);

	return fd;
}

void hipxel_FlacDecoder_delete(hipxel_FlacDecoder *fd) {
	if (NULL != fd->internalDecoder)
		FLAC__stream_decoder_delete((FLAC__StreamDecoder *) fd->internalDecoder);

	hipxel_GrowingBuffer_delete(fd->growingBuffer);

	fd->reader.release(fd->reader.p);

	free(fd);
}
