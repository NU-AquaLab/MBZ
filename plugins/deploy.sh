adb push $1/app/build/outputs/apk/debug/app-debug.apk /data/data/com.nalindquist.mbz/files/plugins/$1/$1.jar
adb push $1/app/build/intermediates/cmake/debug/obj/armeabi-v7a/libplugin.so /data/data/com.nalindquist.mbz/files/plugins/$1/lib$1.so
adb push $1/config /data/data/com.nalindquist.mbz/files/plugins/$1