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

#ifndef HIPXEL_GROWINGBUFFER
#define HIPXEL_GROWINGBUFFER

#include <stdint.h>

typedef struct hipxel_GrowingBuffer {
	uint8_t *data;
	int64_t dataCapacity;
	int64_t dataLength;
} hipxel_GrowingBuffer;

hipxel_GrowingBuffer *hipxel_GrowingBuffer_new();

void hipxel_GrowingBuffer_delete(hipxel_GrowingBuffer *gb);

void *hipxel_GrowingBuffer_claimForWrite(hipxel_GrowingBuffer *gb, int64_t length);

int64_t hipxel_GrowingBuffer_consume(hipxel_GrowingBuffer *gb, void *buffer, int64_t length);

int64_t hipxel_GrowingBuffer_discard(hipxel_GrowingBuffer *gb, int64_t length);

void hipxel_GrowingBuffer_clear(hipxel_GrowingBuffer *gb);

int64_t hipxel_GrowingBuffer_getLength(hipxel_GrowingBuffer *gb);

#endif // HIPXEL_GROWINGBUFFER
