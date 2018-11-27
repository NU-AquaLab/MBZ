package com.nalindquist.snitch;

import android.app.Activity;

public class PluginActivity extends Activity {
    private String name;

    public PluginActivity() {
        name = "foo";
    }

    public String getName() {
        return name;
    }
}
