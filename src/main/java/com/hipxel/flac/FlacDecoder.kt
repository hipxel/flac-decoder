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

package com.hipxel.flac

import java.nio.ByteBuffer

class FlacDecoder(dataReader: DataReader) {
	private var pointer: ByteBuffer? = null

	init {
		if (!Loader.loadNative())
			throw IllegalStateException("native library is not loaded")

		pointer = create(dataReader)
		if (pointer == null)
			throw IllegalStateException("native create failed")
	}

	fun release() {
		pointer?.let {
			pointer = null
			release(it)
		}
	}

	fun step(): Boolean {
		return pointer?.let { step(it) } ?: false
	}

	fun read(buffer: ByteArray, length: Long): Long {
		return pointer?.let { read(it, buffer, length) } ?: -1L
	}

	fun seekTo(position: Long) {
		pointer?.let { seekTo(it, position) }
	}

	val sampleRate: Int
		get() = pointer?.let { getSampleRate(it) } ?: 0

	val channelsCount: Int
		get() = pointer?.let { getChannelsCount(it) } ?: 0

	val bitsPerSample: Int
		get() = pointer?.let { getBitsPerSample(it) } ?: 0

	val totalSamplesCount: Long
		get() = pointer?.let { getTotalSamplesCount(it) } ?: 0

	val pcmFramesPosition: Long
		get() = pointer?.let { getPcmFramesPosition(it) } ?: 0

	val bytesReadyCount: Long
		get() = pointer?.let { getBytesReadyCount(it) } ?: 0

	private external fun create(dataReader: DataReader): ByteBuffer?

	private external fun release(pointer: ByteBuffer)

	private external fun step(pointer: ByteBuffer): Boolean

	private external fun read(pointer: ByteBuffer, buffer: ByteArray, length: Long): Long

	private external fun seekTo(pointer: ByteBuffer, position: Long)

	private external fun getSampleRate(pointer: ByteBuffer): Int

	private external fun getChannelsCount(pointer: ByteBuffer): Int

	private external fun getBitsPerSample(pointer: ByteBuffer): Int

	private external fun getTotalSamplesCount(pointer: ByteBuffer): Long

	private external fun getPcmFramesPosition(pointer: ByteBuffer): Long

	private external fun getBytesReadyCount(pointer: ByteBuffer): Long

	private object Loader {
		private val loaded by lazy {
			try {
				System.loadLibrary("flacdecoder")
				true
			} catch (t: Throwable) {
				t.printStackTrace()
				false
			}
		}

		fun loadNative(): Boolean {
			return loaded
		}
	}
}