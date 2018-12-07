package com.aqualab.mbz;

import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;

class PluginConfig {
    private static final String TAG = "PluginConfig";

    private int m_pipeline;
    private int[] m_services;

    PluginConfig() {
        m_services = null;
    }

    boolean load(File f) {
        ArrayList<String> lines = new ArrayList<>();

        try {
            FileReader fr = new FileReader(f);
            BufferedReader br = new BufferedReader(fr);

            String line;
            while ((line = br.readLine()) != null) {
                lines.add(line);
            }

            br.close();
            fr.close();
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to read config file: " + e.getMessage());
            return false;
        }

        if (lines.size() < 1) {
            Log.e(TAG, "Invalid config file.");
            return false;
        }

        m_pipeline = Integer.parseInt(lines.get(0));

        m_services = new int[lines.size()-1];
        for (int i = 1; i < lines.size(); i++) {
            m_services[i-1] = Integer.parseInt(lines.get(i));
        }

        return true;
    }

    int getPipeline() {
        return m_pipeline;
    }

    int[] getServices() {
        return m_services;
    }
}
