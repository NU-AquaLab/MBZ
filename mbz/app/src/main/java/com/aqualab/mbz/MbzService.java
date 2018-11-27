package com.aqualab.mbz;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.net.VpnService;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Toast;

public class MbzService extends VpnService {
    private static final String TAG = "MbzService";
    private static final String CHANNEL_ID = "MbzChannel";
    private static final int FOREGROUND_ID = 1;

    private RouterController m_rctrl;
    private PluginController m_pctrl;

    @Override
    public void onCreate() {
        m_rctrl = new RouterController(this);
        m_pctrl = new PluginController();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            createNotificationChannel();
        }

        Log.d(TAG, "Service created.");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return new MbzBinder();
    }

    @Override
    public void onDestroy() {
        if (!m_rctrl.isStopped()) {
            stopRouting();
        }
        Log.d(TAG, "Service destroyed.");
    }

    @TargetApi(27)
    private void createNotificationChannel() {
        CharSequence name = getString(R.string.channel_name);
        String description = getString(R.string.channel_description);
        int importance = NotificationManager.IMPORTANCE_LOW;
        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
        channel.setDescription(description);

        NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (nm != null) {
            nm.createNotificationChannel(channel);
        }
        else {
            Log.e(TAG, "No notification manager.");
            Toast.makeText(this, "No notification manager.", Toast.LENGTH_SHORT).show();
            stopSelf();
        }
    }

    private Notification createNotification() {
        Notification.Builder b;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            b = new Notification.Builder(this, CHANNEL_ID);
        }
        else {
            b = new Notification.Builder(this);
        }

        return b.setSmallIcon(R.drawable.ic_launcher_background)
                .setContentTitle("MBZ Router")
                .setContentText("MBZ routing is enabled.")
                .setPriority(Notification.PRIORITY_LOW)
                .build();
    }

    // interface to activities

    class MbzBinder extends Binder {
        void startRouting() {
            MbzService.this.startRouting();
        }

        void stopRouting() {
            MbzService.this.stopRouting();
        }

        boolean routerRunning() {
            return MbzService.this.routerRunning();
        }

        void registerPlugin(Plugin p) {
            MbzService.this.registerPlugin(p);
        }

        void startPlugin(Plugin p) {
            MbzService.this.startPlugin(p);
        }

        void stopPlugin(Plugin p) {
            MbzService.this.stopPlugin(p);
        }

        boolean pluginRunning(Plugin p) {
            return MbzService.this.pluginRunning(p);
        }

        ViewGroup pluginCreate(int pid, Context context, Object updater) {
            return MbzService.this.pluginCreate(pid, context, updater);
        }

        void pluginRefresh(int pid) {
            MbzService.this.pluginRefresh(pid);
        }

        void pluginDestroy(int pid) {
            MbzService.this.pluginDestroy(pid);
        }

        NativeUpdater getNativeUpdater(int pid) {
            return new NativeUpdater(pid);
        }
    }

    public class NativeUpdater {
        private int m_pid;

        NativeUpdater(int pid) {
            m_pid = pid;
        }

        // for plugin use
        public void updateNative(byte[] buf) {
            MbzService.this.pluginUpdateNative(m_pid, buf);
        }
    }

    // interface to MbzActivity

    private void startRouting() {
        Log.d(TAG, "Starting router...");

        if (!m_rctrl.isStopped()) {
            Toast.makeText(this, "Router already running.", Toast.LENGTH_SHORT).show();
            return;
        }

        boolean ok = m_rctrl.requestStart(new Builder());
        if (!ok) {
            Log.e(TAG, "Router failed to start.");
            return;
        }

        startForeground(FOREGROUND_ID, createNotification());
    }

    private void stopRouting() {
        Log.d(TAG, "Stopping router...");

        if (m_rctrl.isStopped()) {
            Toast.makeText(this, "Router not running.", Toast.LENGTH_SHORT).show();
            return;
        }

        m_rctrl.requestStop();
        m_pctrl.setAllStopped();
    }

    private boolean routerRunning() {
        return !m_rctrl.isStopped();
    }

    private void registerPlugin(Plugin p) {
        m_pctrl.addPlugin(p);
    }

    private void startPlugin(Plugin p) {
        if (m_rctrl.isStopped()) {
            Log.e(TAG, "Unable to start plugin: router not running.");
            return;
        }

        if (m_pctrl.pluginRunning(p.getId())) {
            Log.d(TAG, "Plugin '" + p.getName()  + "' already started.");
            return;
        }

        m_pctrl.setPluginStarted(p.getId());
        m_rctrl.requestPluginStart(p);

        Log.d(TAG, "Plugin '" + p.getName() + "' started.");
    }

    private void stopPlugin(Plugin p) {
        if (m_rctrl.isStopped()) {
            Log.e(TAG, "Unable to stop plugin: router not running.");
            return;
        }

        if (!m_pctrl.pluginRunning(p.getId())) {
            Log.d(TAG, "Plugin '" + p.getName()  + "' not running.");
            return;
        }

        m_rctrl.requestPluginStop(p.getId());
        m_pctrl.setPluginStopped(p.getId());

        Log.d(TAG, "Plugin '" + p.getName() + "' stopped.");
    }

    private boolean pluginRunning(Plugin p) {
        return m_pctrl.pluginRunning(p.getId());
    }

    // interface to PluginActivity

    private ViewGroup pluginCreate(int pid, Context context, Object updater) {
        return m_pctrl.getPlugin(pid).create(context, updater);
    }

    private void pluginRefresh(int pid) {
        m_pctrl.getPlugin(pid).refresh();
    }

    private void pluginDestroy(int pid) {
        m_pctrl.getPlugin(pid).destroy();
    }

    private void pluginUpdateNative(int pid, byte[] buf) {
        m_rctrl.requestPluginUpdate(pid, buf);
    }

    // interface to RouterController

    void routerStopped() {
        stopForeground(true);
    }

    void updatePlugin(int pid, byte[] buf) {
        m_pctrl.getPlugin(pid).update(buf);
    }

    void requestPluginRefresh(int pid) {
        try {
            if (PluginActivity.currpid == pid) {
                Intent intent = new Intent(this, PluginActivity.class);
                intent.putExtra(MbzActivity.EXTRA_PID, pid);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
            }
        }
        catch (Exception e) {
            Log.e(TAG, "error: " + e.getMessage());
        }
    }
}
