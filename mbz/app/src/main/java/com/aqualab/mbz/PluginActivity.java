package com.aqualab.mbz;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.ViewGroup;

public class PluginActivity extends Activity {
    static int currpid = -1;

    private ServiceConnection m_conn;
    private MbzService.MbzBinder m_binder;
    private int m_pid;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_plugin);

        m_conn = new MbzServiceConnection();
        m_binder = null;
        m_pid = getIntent().getIntExtra(MbzActivity.EXTRA_PID, -2);

        Intent intent = new Intent(this, MbzService.class);
        boolean ok = bindService(intent, m_conn, Context.BIND_AUTO_CREATE);
        if (!ok) {
            unbindService(m_conn);
        }
    }

    @Override
    public void onStart() {
        super.onStart();

        currpid = m_pid;
    }

    @Override
    public void onStop() {
        super.onStop();

        currpid = -1;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (m_binder != null) {
            m_binder.pluginDestroy(m_pid);
            unbindService(m_conn);
        }
        m_binder = null;
    }

    @Override
    public void onNewIntent(Intent intent) {
        int pid = intent.getIntExtra(MbzActivity.EXTRA_PID, -2);

        if (pid != currpid) {
            return;
        }

        if (m_binder == null) {
            return;
        }

        m_binder.pluginRefresh(pid);
    }

    private class MbzServiceConnection implements ServiceConnection {
        @Override
        public void onServiceConnected(ComponentName className, IBinder binder) {
            m_binder = (MbzService.MbzBinder) binder;
            MbzService.NativeUpdater u = m_binder.getNativeUpdater(m_pid);
            ViewGroup v = m_binder.pluginCreate(m_pid, PluginActivity.this, u);
            setContentView(v);
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            m_binder = null;
        }
    }
}
