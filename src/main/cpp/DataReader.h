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

#ifndef HIPXEL_DATAREADER
#define HIPXEL_DATAREADER

#include <stdint.h>

typedef struct hipxel_DataReader {
	void *p;

	int64_t (*read)(void *p, int64_t position, int64_t length, void *buffer);

	int64_t (*getSize)(void *p);

	void (*release)(void *p);
} hipxel_DataReader;

#endif // HIPXEL_DATAREADER
