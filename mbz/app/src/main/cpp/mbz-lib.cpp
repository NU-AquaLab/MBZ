#include <jni.h>

#include "routing/Router.h"

static routing::Router r;

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_initNativeRouting(
  JNIEnv *env, jobject obj) {
  r.init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_runNativeRouting(
  JNIEnv *env, jobject obj, jint tunfd) {
  r.run(env, obj, tunfd);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_stopNativeRouting(
  JNIEnv *env, jobject obj) {
  r.stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_startPluginNative(
  JNIEnv *env, jobject obj, jint pid,
  jint pipeline, jintArray services, jstring libpath) {
  jint *s = env->GetIntArrayElements(services, NULL);
  jsize n = env->GetArrayLength(services);
  const char *l = env->GetStringUTFChars(libpath, NULL);

  r.startPlugin(pid, pipeline, s, (int) n, l);

  env->ReleaseIntArrayElements(services, s, 0);
  env->ReleaseStringUTFChars(libpath, l);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_stopPluginNative(
  JNIEnv *env, jobject obj, jint pid) {
  r.stopPlugin(pid);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_aqualab_mbz_RouterController_updatePluginNative(
  JNIEnv *env, jobject obj, jint pid, jbyteArray data) {
  jbyte *buf = env->GetByteArrayElements(data, NULL);
  jsize n = env->GetArrayLength(data);

  r.updatePlugin(pid, (char *) buf, n);

  env->ReleaseByteArrayElements(data, buf, 0);
}

