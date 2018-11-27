#ifndef MBZ_JNI_H
#define MBZ_JNI_H

#include <jni.h>

#include "services/UiService.h"

namespace routing {
  class Jni {
  public:
    bool cacheRefs(JNIEnv *jenv, jobject jrouter);

    bool protectSocket(int sock);
    bool postUiData(services::ui_data *uidata);
    bool getWifiSignalLevel(double *level);
    bool disconnectWifi();

  private:
    JNIEnv *m_jenv;
    jobject m_instance;
    jmethodID m_protect;
    jmethodID m_postUiData;
    jmethodID m_getWifiSignalLevel;
    jmethodID m_disconnectWifi;
  };
}


#endif //MBZ_JNI_H
