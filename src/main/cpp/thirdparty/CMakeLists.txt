#
# Copyright (C) 2020 Janusz Jankowski
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

set(FLAC_FILENAMES
	bitmath.c
	bitreader.c
	bitwriter.c
	cpu.c
	crc.c
	fixed.c
	fixed_intrin_sse2.c
	fixed_intrin_ssse3.c
	float.c
	format.c
	lpc.c
	lpc_intrin_sse.c
	lpc_intrin_sse2.c
	lpc_intrin_sse41.c
	lpc_intrin_avx2.c
	md5.c
	memory.c
	metadata_iterators.c
	metadata_object.c
	stream_decoder.c
	stream_encoder.c
	stream_encoder_intrin_sse2.c
	stream_encoder_intrin_ssse3.c
	stream_encoder_intrin_avx2.c
	stream_encoder_framing.c
	window.c
	)

foreach (filename ${FLAC_FILENAMES})
	set(FLAC_SOURCES ${FLAC_SOURCES} flac/src/libFLAC/${filename})
endforeach ()

add_library(FLAC STATIC ${FLAC_SOURCES})

set_property(TARGET FLAC PROPERTY C_STANDARD 99)
set_property(TARGET FLAC PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(FLAC PUBLIC
	flac/include
	flac/src/libFLAC/include
	)

target_compile_definitions(FLAC PRIVATE
	"PACKAGE_VERSION=\"1.3.2\""
	HAVE_STDINT_H=1
	HAVE_SYS_PARAM_H=1
	FLAC__INTEGER_ONLY_LIBRARY=1
	FLAC__NO_ASM=1
	FLAC__HAS_OGG=0
	_REENTRANT=1
	)

target_compile_options(FLAC PRIVATE
	-fvisibility=hidden
	)
