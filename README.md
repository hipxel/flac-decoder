# FLAC decoder for android
Android's [MediaExtractor](https://developer.android.com/reference/android/media/MediaExtractor) has issues with seeking FLAC files on most of pre-Android 10 devices.

This library adds minimalistic FLAC decoder written in C with Kotlin bindings (just takes ~30kB per arch).
In order to use it, add it f.e. from Android Studio as Gradle module.

## Try it!
You can check (hear) code in action, it is used to decode FLAC files in this app: [Music Speed Changer](https://play.google.com/store/apps/details?id=com.hipxel.audio.music.speed.changer)
