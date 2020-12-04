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

#include "GrowingBuffer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

hipxel_GrowingBuffer *hipxel_GrowingBuffer_new() {
	hipxel_GrowingBuffer *gb = malloc(sizeof(hipxel_GrowingBuffer));
	gb->data = NULL;
	gb->dataCapacity = 0;
	gb->dataLength = 0;

	return gb;
}

void hipxel_GrowingBuffer_delete(hipxel_GrowingBuffer *gb) {
	free(gb->data);
	free(gb);
}

static bool ensureCanFit(hipxel_GrowingBuffer *gb, int64_t length) {
	if (length <= gb->dataCapacity)
		return true;

	void *newData = realloc(gb->data, length);
	if (NULL == newData)
		return false;

	gb->data = (uint8_t *) newData;
	gb->dataCapacity = length;

	return true;
}

void *hipxel_GrowingBuffer_claimForWrite(hipxel_GrowingBuffer *gb, int64_t length) {
	if (!ensureCanFit(gb, gb->dataLength + length))
		return NULL;

	if (gb->dataCapacity <= 0)
		return NULL;

	void *p = gb->data + gb->dataLength;
	gb->dataLength += length;
	return p;
}

int64_t hipxel_GrowingBuffer_consume(hipxel_GrowingBuffer *gb, void *buffer, int64_t length) {
	if (gb->dataCapacity <= 0 || gb->dataLength <= 0)
		return 0;

	int64_t tlen = length < gb->dataLength ? length : gb->dataLength;
	memcpy(buffer, gb->data, tlen);

	hipxel_GrowingBuffer_discard(gb, tlen);

	return tlen;
}

int64_t hipxel_GrowingBuffer_discard(hipxel_GrowingBuffer *gb, int64_t length) {
	int64_t tlen = length < gb->dataLength ? length : gb->dataLength;
	int64_t rem = gb->dataLength - tlen;
	if (rem > 0) {
		memmove(gb->data, gb->data + tlen, rem);
		gb->dataLength = rem;
	} else {
		gb->dataLength = 0;
	}
	return tlen < 0 ? 0 : tlen;
}

void hipxel_GrowingBuffer_clear(hipxel_GrowingBuffer *gb) {
	gb->dataLength = 0;
}

int64_t hipxel_GrowingBuffer_getLength(hipxel_GrowingBuffer *gb) {
	return gb->dataLength;
}
