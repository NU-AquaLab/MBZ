#include "PluginTable.h"
#include "common/utils.h"

#define TAG "PluginTable"

namespace routing {
  void PluginTable::addPlugin(int pid, Plugin *p) {
    m_pluginMap[pid] = p;
  }

  Plugin *PluginTable::getPlugin(int pid) {
    return m_pluginMap.find(pid)->second;
  }

  void PluginTable::removePlugin(int pid) {
    auto it = m_pluginMap.find(pid);

    if (it != m_pluginMap.end()) {
      Plugin *p = it->second;
      it->second = NULL;
      m_pluginMap.erase(it);

      delete p;
    }
  }

  bool PluginTable::hasPlugin(int pid) {
    return m_pluginMap.find(pid) != m_pluginMap.end();
  }

  bool PluginTable::pluginRunning(int pid) {
    auto it = m_pluginMap.find(pid);
    if (it == m_pluginMap.end()) {
      return false;
    }
    return it->second->isRunning();
  }

  void PluginTable::getPipelinePlugins(std::vector<Plugin *> *plugins) {
    for (auto it = m_pluginMap.begin(); it != m_pluginMap.end(); it++) {
      if (it->second->isPipeline() && it->second->isRunning()) {
        plugins->push_back(it->second);
      }
    }
  }

  void PluginTable::stopAll() {
    for (auto it = m_pluginMap.begin(); it != m_pluginMap.end(); it++) {
      if (it->second->isRunning()) {
        it->second->stop();
      }
    }
  }

  void PluginTable::joinAll() {
    for (auto it = m_pluginMap.begin(); it != m_pluginMap.end(); it++) {
      if (it->second->isRunning()) {
        int rc = pthread_join(*it->second->getThread(), NULL);
        if (rc < 0) {
          ERROR_PRINT(TAG, "Error joining plugin thread: %d", rc);
        }
        it->second->setRunState(false, NULL);
      }
    }
  }

  void PluginTable::removeAll() {
    for (auto it = m_pluginMap.begin(); it != m_pluginMap.end(); it++) {
      Plugin *p = it->second;
      it->second = NULL;
      p->deinit();
      delete p;
    }

    m_pluginMap.clear();
  }
}
