#include <services/UiService.h>
#include "Jni.h"
#include "common/utils.h"

#define TAG "Jni"

namespace routing {
  bool Jni::cacheRefs(JNIEnv *jenv, jobject jrouter) {
    // get class handles
    jclass routerClass = jenv->FindClass("com/aqualab/mbz/RouterController");
    if (routerClass == NULL) {
      ERROR_PRINT(TAG, "Unable to find router control class.");
      return false;
    }

    // get method handles
    m_protect = jenv->GetMethodID(routerClass, "protect", "(I)Z");
    if (m_protect == NULL) {
      ERROR_PRINT(TAG, "Unable to find protect() method.");
      return false;
    }

    m_postUiData = jenv->GetMethodID(routerClass, "postUiData", "(I[B)Z");
    if (m_postUiData == NULL) {
      ERROR_PRINT(TAG, "Unable to find postUiData() method.");
      return false;
    }

    m_getWifiSignalLevel = jenv->GetMethodID(routerClass, "getWifiSignalLevel", "()D");
    if (m_getWifiSignalLevel == NULL) {
      ERROR_PRINT(TAG, "Unable to find getWifiSignalLevel() method.");
      return false;
    }

    m_disconnectWifi = jenv->GetMethodID(routerClass, "disconnectWifi", "()Z");
    if (m_disconnectWifi == NULL) {
      ERROR_PRINT(TAG, "Unable to find disconnectWifi() method.");
      return false;
    }

    // cache environment (only valid for duration of Router::run())
    m_instance = jrouter;
    m_jenv = jenv;

    return true;
  }

  bool Jni::protectSocket(int sock) {
    bool ok = m_jenv->CallBooleanMethod(m_instance, m_protect, sock);

    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when calling protect().");
      return false;
    }

    return ok;
  }

  bool Jni::postUiData(services::ui_data *uidata) {
    jbyteArray buf = m_jenv->NewByteArray(uidata->len);
    if (buf == NULL) {
      ERROR_PRINT(TAG, "Unable to create new object array.");
      return false;
    }

    m_jenv->SetByteArrayRegion(buf, 0, uidata->len, (const jbyte *) uidata->buf);
    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when setting buffer region.");
      return false;
    }

    bool ok = m_jenv->CallBooleanMethod(m_instance, m_postUiData, uidata->pid, buf);
    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when calling postUiData().");
      return false;
    }

    m_jenv->DeleteLocalRef(buf);
    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when deleting array.");
      return false;
    }

    return ok;
  }

  bool Jni::getWifiSignalLevel(double *level) {
    double l = m_jenv->CallDoubleMethod(m_instance, m_getWifiSignalLevel);

    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when calling getWifiSignalLevel().");
      *level = -1.0;
      return false;
    }

    *level = l;
    return true;
  }

  bool Jni::disconnectWifi() {
    bool ok = m_jenv->CallBooleanMethod(m_instance, m_disconnectWifi);

    if (m_jenv->ExceptionCheck() == JNI_TRUE) {
      m_jenv->ExceptionClear();
      ERROR_PRINT(TAG, "Exception raised by JNI when calling disconnectWifi().");
      return false;
    }

    return ok;
  }
}
