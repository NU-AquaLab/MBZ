#ifndef MBZ_UISERVICE_H
#define MBZ_UISERVICE_H

#include <stdint.h>

#include "Service.h"

namespace services {
  class UiService : public Service {
  public:
    UiService(int sid, int rreqfd, int wreqfd,
              pthread_t *t,
              bool (*pushUiData)(ui_data *));

    virtual void init();
    virtual void deinit();

  protected:
    virtual bool execReq(int fid, void *buf);
    bool execReqPushUiData(void *buf);

  private:
    bool (*m_pushuidata)(ui_data *);
  };
}


#endif //MBZ_UISERVICE_H
