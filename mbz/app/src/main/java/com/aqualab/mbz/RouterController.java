package com.aqualab.mbz;

import android.content.Context;
import android.net.VpnService;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.Toast;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.UnknownHostException;
import java.util.Enumeration;

public class RouterController implements Runnable {
    private static final String TAG = "RouterController";
    private static final String TUN_ADDRESS_V4 = "192.168.0.0";
    private static final int TUN_PREFIX_V4 = 32;

    private Thread m_thread;
    private MbzService m_service;
    private VpnService.Builder m_builder;
    private WifiManager m_wifi;
    private boolean m_stopped;

    RouterController(MbzService service) {
        m_thread = null;
        m_service = service;
        m_wifi = (WifiManager) service.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        m_builder = null;
        m_stopped = true;

        initNativeRouting();
    }

    // interface to MbzService

    boolean isStopped() {
        return m_stopped;
    }

    boolean requestStart(VpnService.Builder bldr) {
        if (!m_stopped) {
            return false;
        }

        m_builder = bldr;
        m_stopped = false;
        m_thread = new Thread(this);
        m_thread.start();

        return true;
    }

    void requestStop() {
        if (m_stopped) {
            return;
        }
        stopNativeRouting();
    }

    void requestPluginStart(Plugin p) {
        startPluginNative(p.getId(), p.getPipeline(), p.getServices(), p.getLibPath());
    }

    void requestPluginStop(int pid) {
        stopPluginNative(pid);
    }

    void requestPluginUpdate(int pid, byte[] buf) {
        updatePluginNative(pid, buf);
    }

    // interface to native code

    boolean protect(int sock) {
        return m_service.protect(sock);
    }

    boolean postUiData(int pid, byte[] buf) {
        m_service.updatePlugin(pid, buf);
        m_service.requestPluginRefresh(pid);
        return true;
    }

    double getWifiSignalLevel() {
        WifiInfo info = m_wifi.getConnectionInfo();
        if (info == null) {
            return 0.0;
        }

        return WifiManager.calculateSignalLevel(info.getRssi(), 100);
    }

    boolean disconnectWifi() {
        return m_wifi.disconnect();
    }

    // routing loop

    @Override
    public void run() {
        ParcelFileDescriptor fd = configure();

        if (fd != null) {
            runNativeRouting(fd.detachFd());
        }
        else {
            Log.e(TAG, "Unable to init tun interface.");
        }

        m_service.routerStopped();
        m_stopped = true;
        Log.d(TAG, "Routing loop stopped.");
    }

    private ParcelFileDescriptor configure() {
        // set address of tun interface
        m_builder.addAddress(TUN_ADDRESS_V4, TUN_PREFIX_V4);
        Log.d(TAG, "Assigned tun0 addr: " + TUN_ADDRESS_V4 + "/" + TUN_PREFIX_V4);

        // add catch-all route
        m_builder.addRoute("0.0.0.0", 0);

        //added by jtn
        //Add the dns resolver to the path
//        try {
//            m_builder.addDnsServer("8.8.8.8");
//        }catch(IllegalArgumentException iae){
//            Log.d("JTN_DNS", "Error. Invalid DNS resolver: 8.8.8.8");
//        }
        // add routes for currently active network interfaces
        try {
            Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements()) {
                NetworkInterface netif = e.nextElement();
                if (netif.isUp() && !netif.isLoopback()) {
                    for (InterfaceAddress ia : netif.getInterfaceAddresses()) {
                        configureRoute(ia);
                    }
                }
            }
        }
        catch (Exception e) {
            Log.e(TAG, "Error adding routes for network interfaces: " + e.getMessage());
            return null;
        }

        m_builder.setSession("MBZ");
        return m_builder.establish();
    }

    private void configureRoute(InterfaceAddress ia) throws UnknownHostException {
        // get addr
        InetAddress in = ia.getAddress();
        short prefix = ia.getNetworkPrefixLength();

        if (in instanceof Inet4Address) {
            // zero all but prefix
            byte[] raw = in.getAddress();
            for (byte i = 31; i >= prefix; i--) {
                raw[i/8] &= ~(0x01 << ((31-i) % 8));
            }
            InetAddress zeroed = InetAddress.getByAddress(raw);
            String addr = zeroed.getHostAddress();

            // add route
            m_builder.addRoute(addr, prefix);
            Log.d(TAG, "Added tun0 route: " +
                    addr + "/" + prefix);
        }
    }

    private native void initNativeRouting();
    private native void runNativeRouting(int fd);
    private native void stopNativeRouting();
    private native void startPluginNative(int pid, int pipeline, int[] services, String libpath);
    private native void stopPluginNative(int pid);
    private native void updatePluginNative(int pid, byte[] buf);

    static {
        System.loadLibrary("mbz-lib");
    }
}
