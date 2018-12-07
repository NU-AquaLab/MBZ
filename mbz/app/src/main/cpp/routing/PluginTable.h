#ifndef MBZ_PLUGINTABLE_H
#define MBZ_PLUGINTABLE_H

#include <map>
#include <vector>

#include "Plugin.h"

namespace routing {
  class PluginTable {
  public:
    void addPlugin(int pid, Plugin *p);
    Plugin *getPlugin(int pid);
    void removePlugin(int pid);
    bool hasPlugin(int pid);
    bool pluginRunning(int pid);
    void getPipelinePlugins(std::vector<Plugin*> *plugins);

    void stopAll();
    void joinAll();
    void removeAll();

  private:
    std::map<int, Plugin*> m_pluginMap;
  };
}


#endif //MBZ_PLUGINTABLE_H
