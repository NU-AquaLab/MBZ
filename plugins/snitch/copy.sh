adb push app/build/outputs/apk/debug/app-debug.apk /sdcard/
adb push app/build/intermediates/cmake/debug/obj/armeabi-v7a/libplugin.so /sdcard/
adb push config /sdcard/
adb shell cp /sdcard/app-debug.apk /data/data/com.nalindquist.mbz/files/plugins/snitch/snitch.jar
adb shell cp /sdcard/libplugin.so /data/data/com.nalindquist.mbz/files/plugins/snitch/libsnitch.so
adb shell cp /sdcard/config /data/data/com.nalindquist.mbz/files/plugins/snitch/config
