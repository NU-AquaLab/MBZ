package com.nalindquist.snitch;

import android.content.Context;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.HorizontalScrollView;
import android.widget.ListView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;

public class PluginLayout {
    //private static final String APPLICATION_TEXT = "Application";
    private static final String LOCAL_HOST_TEXT = "Local Host";
    private static final String REMOTE_HOST_TEXT = "Remote Host";
    private static final String PROTOCOL_TEXT = "Protocol";
    private static final String[] HEADERS = {
            //APPLICATION_TEXT,
            LOCAL_HOST_TEXT,
            REMOTE_HOST_TEXT,
            PROTOCOL_TEXT
    };
    private static final String TAG = "snitch-ui";

    private Context m_context;
    private ArrayList<Flow> m_flows;
    private FlowAdapter m_adapter;

    public PluginLayout() {
        m_context = null;
        m_flows = new ArrayList<>();
        m_adapter = null;
    }

    public ViewGroup create(Context context, Object updater) {
        Log.d(TAG, "create");

        m_context = context;
        m_adapter = new FlowAdapter();
        m_adapter.setFlows(m_flows);

        ListView lv = new ListView(context);
        lv.setLayoutParams(new AbsListView.LayoutParams(
                AbsListView.LayoutParams.MATCH_PARENT,
                AbsListView.LayoutParams.MATCH_PARENT));
        lv.setAdapter(m_adapter);

        HorizontalScrollView hsv = new HorizontalScrollView(context);
        hsv.setLayoutParams(new AbsListView.LayoutParams(
                AbsListView.LayoutParams.WRAP_CONTENT,
                AbsListView.LayoutParams.WRAP_CONTENT));
        hsv.setFillViewport(true);
        hsv.addView(lv);

        return hsv;
    }

    public void update(byte[] buf) {
        Log.d(TAG, "update");

        Log.d(TAG, "received " + buf.length + " bytes");

        m_flows = new ArrayList<>();
        ByteBuffer bb = ByteBuffer.wrap(buf);
        bb.order(ByteOrder.LITTLE_ENDIAN);

        for (int i = 0; i < buf.length; i += 56) {
            int uid = bb.getInt(i);
            String local  = new String(Arrays.copyOfRange(buf, i+4, i+28));
            String remote = new String(Arrays.copyOfRange(buf, i+28, i+52));
            String proto  = new String(Arrays.copyOfRange(buf, i+52, i+56));

            String app = "";
            if (uid < 0) {
                app = "<unknown>";
            }

            Flow f = new Flow(app, local, remote, proto);
            m_flows.add(f);
        }

        Log.d(TAG, "added " + m_flows.size() + " flows");
    }

    public void refresh() {
        Log.d(TAG, "refresh");

        if (m_adapter != null) {
            m_adapter.setFlows(m_flows);
            m_adapter.notifyDataSetChanged();
        }
    }

    public void destroy() {
        Log.d(TAG, "destroy");

        m_context = null;
        m_adapter = null;
    }

    private class FlowAdapter extends BaseAdapter {
        private ArrayList<Flow> m_flows;

        FlowAdapter() {
            m_flows = new ArrayList<>();
        }

        @Override
        public int getCount() {
            return 1;
        }

        @Override
        public Object getItem(int i) {
            return m_flows;
        }

        @Override
        public long getItemId(int i) {
            return i;
        }

        @Override
        public View getView(int i, View view, ViewGroup parent) {
            TableLayout tl = new TableLayout(PluginLayout.this.m_context);

            tl.setLayoutParams(new AbsListView.LayoutParams(
                    AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.MATCH_PARENT));

            TableRow header = new TableRow(PluginLayout.this.m_context);

            for (String s: HEADERS) {
                TextView text = new TextView(PluginLayout.this.m_context);
                text.setText(s);
                header.addView(text);
            }

            tl.addView(header);

            for (Flow f: m_flows) {
                TableRow row = new TableRow(PluginLayout.this.m_context);

                //TextView app = new TextView(PluginLayout.this.m_context);
                //app.setText(f.app);

                TextView local = new TextView(PluginLayout.this.m_context);
                local.setText(f.local);

                TextView remote = new TextView(PluginLayout.this.m_context);
                remote.setText(f.remote);

                TextView proto = new TextView(PluginLayout.this.m_context);
                proto.setText(f.protocol);

                //row.addView(app);
                row.addView(local);
                row.addView(remote);
                row.addView(proto);

                tl.addView(row);
            }

            for (int j = 0; j < HEADERS.length; j++) {
                tl.setColumnShrinkable(j, true);
                tl.setColumnStretchable(j, true);
            }

            return tl;
        }

        void setFlows(ArrayList<Flow> flows) {
            m_flows = flows;
        }
    }

    private class Flow {
        String app;
        String local;
        String remote;
        String protocol;

        Flow(String app, String local, String remote, String protocol) {
            this.app = app;
            this.local = local;
            this.remote = remote;
            this.protocol = protocol;
        }
    }
}
