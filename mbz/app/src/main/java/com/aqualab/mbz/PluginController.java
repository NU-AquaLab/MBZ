package com.aqualab.mbz;

import java.util.HashMap;

class PluginController {
    private HashMap<Integer, Plugin> m_plugins;

    PluginController() {
        m_plugins = new HashMap<>();
    }

    void addPlugin(Plugin p) {
        m_plugins.put(p.getId(), p);
    }

    Plugin getPlugin(int pid) {
        return m_plugins.get(pid);
    }

    boolean pluginRunning(int pid) {
        return m_plugins.get(pid).isRunning();
    }

    void setPluginStarted(int pid) {
        m_plugins.get(pid).setRunning(true);
    }

    void setPluginStopped(int pid) {
        m_plugins.get(pid).setRunning(false);
    }

    void setAllStopped() {
        for (Plugin p: m_plugins.values()) {
            p.setRunning(false);
        }
    }
}
