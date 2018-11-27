package com.aqualab.mbz;

import android.content.Context;
import android.util.Log;
import android.view.ViewGroup;

import java.io.File;
import java.lang.reflect.Method;

import dalvik.system.DexClassLoader;

class Plugin {
    private static final String TAG = "Plugin";
    private static final String CONFIG_FILE = "config";
    private static final String LAYOUT_NAME = "PluginLayout";

    private int m_pid;
    private String m_name;
    private PluginConfig m_config;
    private String m_libpath;
    private boolean m_running;
    private Object m_instance;
    private Method m_create;
    private Method m_update;
    private Method m_refresh;
    private Method m_destroy;

    Plugin(int id, String name) {
        m_pid = id;
        m_name = name;
        m_config = null;
        m_libpath = "";
        m_running = false;
        m_instance = null;
        m_create = null;
        m_update = null;
        m_destroy = null;
    }

    boolean load(File dir, Context context) {
        File config = new File(dir, CONFIG_FILE);
        if (!config.exists()) {
            Log.e(TAG, "Plugin '" + m_name + "' does not have config file.");
            return false;
        }

        m_config = new PluginConfig();
        if (!m_config.load(config)) {
            Log.e(TAG, "Unable to read config file for plugin '" + m_name + "'.");
            return false;
        }

        File jar = new File(dir, m_name + ".jar");
        if (!jar.exists()) {
            Log.e(TAG, "Plugin '" + m_name + "' does not have jar file.");
            return false;
        }

        File lib = new File(dir, "lib" + m_name + ".so");
        if (!lib.exists()) {
            Log.e(TAG, "Plugin '" + m_name + "' does not have so file.");
            return false;
        }
        m_libpath = lib.getAbsolutePath();

        DexClassLoader loader = new DexClassLoader(
                jar.getAbsolutePath(),
                context.getCacheDir().getAbsolutePath(),
                null,
                context.getClassLoader());
        String className = "com.nalindquist." + m_name + "." + LAYOUT_NAME;

        Class<?> layoutClass;
        try {
            layoutClass = loader.loadClass(className);
        }
        catch (Exception e) {
            Log.e(TAG, "Error loading plugin layout: " + e.getMessage());
            return false;
        }

        try {
            m_instance = layoutClass.newInstance();
        } catch (Exception e) {
            Log.e(TAG, "Error creating layout instance: " + e.getMessage());
            return false;
        }

        try {
            m_create = layoutClass.getMethod("create", Context.class, Object.class);
            m_update = layoutClass.getMethod("update", byte[].class);
            m_refresh = layoutClass.getMethod("refresh");
            m_destroy = layoutClass.getMethod("destroy");
        }
        catch (Exception e) {
            Log.e(TAG, "Error getting methods: " + e.getMessage());
            return false;
        }

        return true;
    }

    int getId() {
        return m_pid;
    }

    String getName() {
        return m_name;
    }

    int getPipeline() {
        return m_config.getPipeline();
    }

    int[] getServices() {
        return m_config.getServices();
    }

    String getLibPath() {
        return m_libpath;
    }

    boolean isRunning() {
        return m_running;
    }

    void setRunning(boolean val) {
        m_running = val;
    }

    ViewGroup create(Context context, Object updater) {
        try {
            return (ViewGroup) m_create.invoke(m_instance, context, updater);
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to invoke create(): " + e.getCause().getMessage());
            return null;
        }
    }

    void update(byte[] buf) {
        try {
            m_update.invoke(m_instance, buf);
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to invoke update(): " + e.getCause().getMessage());
        }
    }

    void refresh() {
        try {
            m_refresh.invoke(m_instance);
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to invoke refresh(): " + e.getCause().getMessage());
        }
    }

    void destroy() {
        try {
            m_destroy.invoke(m_instance);
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to invoke destroy(): " + e.getCause().getMessage());
        }
    }
}
