package com.aqualab.mbz;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.net.VpnService;
import android.os.Bundle;
import android.os.IBinder;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.JarURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.JarOutputStream;

public class MbzActivity extends Activity {
    public static final String EXTRA_PID = "com.aqualab.mbz.PID";

    private static final String TAG = "MbzActivity";
    private static final String PLUGIN_DIRNAME = "plugins";

    private static final boolean LOAD_PLUGINS = true;

    private ServiceConnection m_conn;
    private MbzService.MbzBinder m_binder;
    private PluginAdapter m_adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mbz);
        // init state
        m_conn = new MbzServiceConnection();
        m_binder = null;
        if(LOAD_PLUGINS) {
            m_adapter = new PluginAdapter();
        }

        //set dns server to a valid address
        android.provider.Settings.System.putString(getContentResolver(), Settings.System.WIFI_STATIC_DNS1, "8.8.8.8");

        // init UI elements
        final Button startButton = findViewById(R.id.router_start);
        Button stopButton = findViewById(R.id.router_stop);

        startButton.setText(R.string.router_start_text);
        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = VpnService.prepare(MbzActivity.this);
                if (intent != null) {
                    startActivityForResult(intent, 0);
                }
                else {
                    startRouting();
                }
                startButton.setClickable(false);
                startButton.setText("Running");
            }
        });

        stopButton.setText(R.string.router_stop_text);
        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopRouting();
                startButton.setClickable(true);
                startButton.setText("Start");
            }
        });

        if(LOAD_PLUGINS) {
            ListView pluginList = findViewById(R.id.plugin_list);
            pluginList.setAdapter(m_adapter);
        }
        Log.d(TAG, "Activity created.");
    }

    @Override
    protected void onStart() {
        super.onStart();

        Intent intent = new Intent(this, MbzService.class);
        boolean ok = bindService(intent, m_conn, Context.BIND_AUTO_CREATE);
        if (ok) {
            startService(intent);
        }
        else {
            unbindService(m_conn);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (m_binder != null) {
            boolean running = m_binder.routerRunning();
            unbindService(m_conn);
            if (!running) {
                Intent intent = new Intent(this, MbzService.class);
                stopService(intent);
            }
        }
        m_binder = null;

        Log.d(TAG, "Activity destroyed.");
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (result == RESULT_OK) {
            startRouting();
        }
    }

    private class MbzServiceConnection implements ServiceConnection {
        @Override
        public void onServiceConnected(ComponentName className, IBinder binder) {
            m_binder = (MbzService.MbzBinder) binder;
            if(LOAD_PLUGINS) {
                m_adapter.registerAll();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            m_binder = null;
        }
    }

    private void startRouting() {
        if (m_binder != null) {
            m_binder.startRouting();
            if(LOAD_PLUGINS) {
                m_adapter.notifyDataSetChanged();
            }
        }
        else {
            Toast.makeText(this, "Unable to bind service.", Toast.LENGTH_SHORT).show();
        }
    }

    private void stopRouting() {
        if (m_binder != null) {
            m_binder.stopRouting();
            if(LOAD_PLUGINS) {
                m_adapter.notifyDataSetChanged();
            }
        }
        else {
            Toast.makeText(this, "Unable to bind service.", Toast.LENGTH_SHORT).show();
        }
    }

    private void registerPlugin(Plugin p) {
        if (m_binder != null) {
            m_binder.registerPlugin(p);
        }
        else {
            Toast.makeText(this, "Unable to bind service.", Toast.LENGTH_SHORT).show();
        }
    }

    private void startPlugin(Plugin p) {
        if (m_binder != null) {
            m_binder.startPlugin(p);
        }
        else {
            Toast.makeText(this, "Unable to bind service.", Toast.LENGTH_SHORT).show();
        }
    }

    private void stopPlugin(Plugin p) {
        if (m_binder != null) {
            m_binder.stopPlugin(p);
        }
        else {
            Toast.makeText(this, "Unable to bind service.", Toast.LENGTH_SHORT).show();
        }
    }

//    private boolean downloadJar(File f) {
//        final File dir = new File(f.getAbsolutePath(), "example.jar");
//        new Thread(new Runnable(){
//
//            @Override
//            public void run() {
//                try {
//                    URL url = new URL("jar:http://hinckley.cs.northwestern.edu/~jtn609/mbz/plugins/example/example.jar!/");
//                    JarURLConnection jarc = (JarURLConnection)url.openConnection();
//                    JarFile jf = jarc.getJarFile();
//
//                    JarOutputStream jos = new JarOutputStream(new FileOutputStream(dir), jf.getManifest());
//                    BufferedOutputStream bos = new BufferedOutputStream(jos);
//                    Enumeration<JarEntry> jarenum = jf.entries();
//                    JarEntry je;
//                    while(jarenum.hasMoreElements()){
//                        je = jarenum.nextElement();
//                        jos.putNextEntry(je);
//
//                    }
//                    jos.flush();
//                    jos.close();
//
//                }catch(Exception e){
//                    e.printStackTrace();
//                }
//
//            }
//        }).start();
//        return true;
//    }
//
//    private boolean downloadConfig(File f) {
//        //JTN - Start of messing stuff up
//        Log.d(TAG, "Detour to test making new plugin");
//        final File dir = new File(f.getAbsolutePath(), "example");
//        boolean success = dir.mkdir();
//        Log.d(TAG, "Result: " + success);
////        Log.d(TAG, "dir: " + dir.getAbsolutePath());
//        new Thread(new Runnable() {
//
//            public void run() {
//                //Make new plugin folder
//
//                ArrayList<String> urls = new ArrayList<String>();
//                try {
//                    // Create a URL for the desired page
//                    URL url = new URL("http://hinckley.cs.northwestern.edu/~jtn609/mbz/plugins/example/config"); //My text file location
//                    //First open the connection
//                    HttpURLConnection conn = (HttpURLConnection) url.openConnection();
//                    conn.setConnectTimeout(60000); // timing out in a minute
//                    BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
//                    String str;
//                    while ((str = in.readLine()) != null) {
//                        urls.add(str);
//                    }
//                    in.close();
//                } catch (Exception e) {
//                    Log.d("MyTag", e.toString());
//                }
//                String contents = "";
//                Log.d(TAG, "Is Pipelined?: " + urls.get(0));
//                contents = urls.get(0) + "\n";
//                String[] id_vals = new String[]{"Flow-Stats", "UI", "WiFi"};
//                for (int i = 1; i < urls.size(); i++) {
//                    int id = Integer.parseInt(urls.get(i));
//                    Log.d(TAG, id_vals[id]);
//                    contents += urls.get(i) + "\n";
//                }
//                    try {
////                        fos = openFileOutput(fos, Context.MODE_PRIVATE);
//                        File plugins = new File(getFilesDir(), "plugins");
//                        Log.d(TAG, "Here?: " + plugins.exists());
//                        File[] fs = plugins.listFiles();
//                        for (int i = 0; i < fs.length; i++){
//                            File f = fs[i];
//                            Log.d(TAG, f.getName());
//                        }
////                        Log.d(TAG, "Made File?: " + b);
//                        File ex = new File(plugins, "example");
//                        File con = new File(ex, "config");
//                        FileOutputStream fos = new FileOutputStream(con);
//                        fos.write(contents.getBytes());
//                        fos.close();
//                    }catch(Exception e){
//                        e.printStackTrace();
//                    }
//                    downloadJar(dir);
//            }
//        }).start();
//        return true;
//    }

    private class PluginAdapter extends BaseAdapter {
        private ArrayList<Plugin> m_plugins;

        PluginAdapter() {
            m_plugins = new ArrayList<>();
            Log.d(TAG, "MBZ PLUGIN_DIRNAME: " + PLUGIN_DIRNAME);
            File dir = new File(getFilesDir(), PLUGIN_DIRNAME);
//            downloadConfig(dir);
            int i = 0;
            for (File f: dir.listFiles()) {
                if (f.isDirectory()) {
                    Plugin p = new Plugin(i, f.getName());
                    if (p.load(f, MbzActivity.this)) {
                        m_plugins.add(p);
                        Log.d(TAG, "Found plugin: " + p.getName());
                    }
                    else {
                        Log.e(TAG, "Bad plugin: " + f.getName());
                    }
                }
                else {
                    Log.e(TAG, "Junk in plugin directory: " + f.getAbsolutePath());
                }

                i++;
            }
        }

        @Override
        public int getCount() {
            return m_plugins.size();
        }

        @Override
        public Object getItem(int i) {
            return m_plugins.get(i);
        }

        @Override
        public long getItemId(int i) {
            return i;
        }

        @Override
        public View getView(int i, View view, ViewGroup parent) {
            if (view == null) {
                view = getLayoutInflater().inflate(R.layout.plugin_entry, parent, false);
            }

            final Plugin p = m_plugins.get(i);

            TextView text = view.findViewById(R.id.plugin_name);
            text.setText(p.getName());

            Button openButton = view.findViewById(R.id.plugin_open);
            openButton.setText(R.string.plugin_open_text);
            openButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent i = new Intent(MbzActivity.this, PluginActivity.class);
                    i.putExtra(EXTRA_PID, p.getId());
                    startActivity(i);
                }
            });

            Button startButton = view.findViewById(R.id.plugin_start);
            startButton.setText(R.string.plugin_start_text);
            startButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    startPlugin(p);
                    notifyDataSetChanged();
                }
            });

            Button stopButton = view.findViewById(R.id.plugin_stop);
            stopButton.setText(R.string.plugin_stop_text);
            stopButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    stopPlugin(p);
                    notifyDataSetChanged();
                }
            });

            if (m_binder != null && m_binder.pluginRunning(p)) {
                view.setBackgroundColor(Color.GREEN);
            }
            else {
                view.setBackgroundColor(Color.YELLOW);
            }

            return view;
        }

        void registerAll() {
            for (Plugin p: m_plugins) {
                registerPlugin(p);
            }
        }
    }
}
