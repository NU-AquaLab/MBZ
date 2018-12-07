package com.nalindquist.firewall;

import android.content.Context;
import android.text.InputType;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Space;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import java.lang.reflect.Method;
import java.util.ArrayList;

public class PluginLayout {
    private static final String HEADER_DEST   = "Destination";
    private static final String HEADER_PREFIX = "Prefix";
    private static final String HEADER_REMOVE = " ";
    private static final String[] HEADERS = {
            HEADER_DEST,
            HEADER_PREFIX,
            HEADER_REMOVE,
    };
    private static final String PROMPT_DEST = "Address: ";
    private static final String PROMPT_PREFIX = "Prefix: ";
    private static final String TEXT_BLOCK = "Block";
    private static final String TEXT_REMOVE = "Remove";
    private static final String TAG = "firewall-ui";

    private Context m_context;
    private Object m_updater;
    private Method m_updateNative;
    private BlacklistAdapter m_adapter;
    private ArrayList<Route> m_routes;

    public PluginLayout() {
        m_context = null;
        m_updater = null;
        m_updateNative = null;
        m_adapter = null;
        m_routes = new ArrayList<>();
    }

    public ViewGroup create(Context context, Object updater) {
        Log.d(TAG, "create");

        m_context = context;
        m_updater = updater;
        try {
            m_updateNative = updater.getClass().getMethod(
                    "updateNative", byte[].class);
        }
        catch (Exception e) {
            Log.e(TAG, "Unable to get updateNative() method: " + e.toString() + ": " + e.getMessage());
        }
        m_adapter = new BlacklistAdapter();

        doNativeUpdate();

        LinearLayout ll = new LinearLayout(context);
        ll.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT));
        ll.setOrientation(LinearLayout.VERTICAL);
        ll.setGravity(Gravity.CENTER);

        ListView blacklist = new ListView(context);
        blacklist.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        blacklist.setAdapter(m_adapter);

        Space space1 = new Space(context);
        space1.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,0, 1.0f));

        LinearLayout input1 = new LinearLayout(context);
        input1.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input1.setOrientation(LinearLayout.HORIZONTAL);
        input1.setGravity(Gravity.CENTER);

        TextView destPrompt = new TextView(context);
        destPrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        destPrompt.setText(PROMPT_DEST);

        final EditText destInput = new EditText(context);
        destInput.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        destInput.setInputType(InputType.TYPE_CLASS_TEXT);

        input1.addView(destPrompt);
        input1.addView(destInput);

        LinearLayout input2 = new LinearLayout(context);
        input2.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input2.setOrientation(LinearLayout.HORIZONTAL);
        input2.setGravity(Gravity.CENTER);

        TextView prefixPrompt = new TextView(context);
        prefixPrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        prefixPrompt.setText(PROMPT_PREFIX);

        final EditText prefixInput = new EditText(context);
        prefixInput.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        prefixInput.setInputType(InputType.TYPE_CLASS_TEXT);

        input2.addView(prefixPrompt);
        input2.addView(prefixInput);

        LinearLayout input3 = new LinearLayout(context);
        input3.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input3.setOrientation(LinearLayout.HORIZONTAL);
        input3.setGravity(Gravity.CENTER);

        Button blockButton = new Button(context);
        blockButton.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        blockButton.setText(TEXT_BLOCK);
        blockButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                m_routes.add(new Route(
                        destInput.getText().toString(),
                        Integer.parseInt(prefixInput.getText().toString())));
                doNativeUpdate();
                m_adapter.notifyDataSetChanged();
            }
        });

        input3.addView(blockButton);

        Space space2 = new Space(context);
        space2.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,0, 1.0f));

        ll.addView(blacklist);
        ll.addView(space1);
        ll.addView(input1);
        ll.addView(input2);
        ll.addView(input3);
        ll.addView(space2);

        return ll;
    }

    public void update(byte[] buf) {
        Log.d(TAG, "update");
    }

    public void refresh() {
        Log.d(TAG, "refresh");
    }

    public void destroy() {
        Log.d(TAG, "destroy");

        m_context = null;
        m_updater = null;
        m_updateNative = null;
        m_adapter = null;
    }

    private void doNativeUpdate() {
        if (m_updateNative != null) {
            byte[] buf = new byte[m_routes.size()*20];

            for (int i = 0; i < m_routes.size(); i++) {
                int j = i*20;
                Route r = m_routes.get(i);

                byte[] dest = r.ip.getBytes();
                for (int k = 0; k < dest.length; k++) {
                    buf[j+k] = dest[k];
                }
                for (int k = dest.length; k < 16; k++) {
                    buf[j+k] = 0;
                }

                buf[j+16] = (byte) r.prefix;

                for (int k = 17; k < 20; k++) {
                    buf[j+k] = 0;
                }
            }

            try {
                m_updateNative.invoke(m_updater, buf);
            }
            catch (Exception e) {
                Log.e(TAG, "Unable to invoke updateNative(): " + e.getMessage());
            }
        }
    }

    private class BlacklistAdapter extends BaseAdapter {
        BlacklistAdapter() {}

        @Override
        public int getCount() {
            return 1;
        }

        @Override
        public Object getItem(int i) {
            return m_routes;
        }

        @Override
        public long getItemId(int i) {
            return i;
        }

        @Override
        public View getView(int i, View view, ViewGroup parent) {
            TableLayout tl = new TableLayout(m_context);

            tl.setLayoutParams(new AbsListView.LayoutParams(
                    AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.WRAP_CONTENT));

            TableRow header = new TableRow(m_context);
            header.setLayoutParams(new TableLayout.LayoutParams(
                    TableLayout.LayoutParams.MATCH_PARENT,
                    TableLayout.LayoutParams.WRAP_CONTENT));
                ran.push(el.id);

            for (String s: HEADERS) {
                TextView text = new TextView(m_context);
                text.setText(s);
                header.addView(text);
            }

            tl.addView(header);

            int j = 0;
            for (Route r: m_routes) {
                final int index = j;

                TableRow row = new TableRow(m_context);
                row.setLayoutParams(new TableLayout.LayoutParams(
                        TableLayout.LayoutParams.MATCH_PARENT,
                        TableLayout.LayoutParams.WRAP_CONTENT));

                TextView dest = new TextView(m_context);
                dest.setText(r.ip);

                TextView prefix = new TextView(m_context);
                prefix.setText(String.format("%d", r.prefix));

                Button remove = new Button(PluginLayout.this.m_context);
                remove.setText(TEXT_REMOVE);
                remove.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        m_routes.remove(index);
                        doNativeUpdate();
                        notifyDataSetChanged();
                    }
                });

                row.addView(dest);
                row.addView(prefix);
                row.addView(remove);

                tl.addView(row);

                j++;
            }

            for (j = 0; j < HEADERS.length; j++) {
                tl.setColumnShrinkable(j, true);
                tl.setColumnStretchable(j, true);
            }

            return tl;
        }
    }

    private class Route {
        String ip;
        int prefix;

        Route(String ip, int prefix) {
            this.ip = ip;
            this.prefix = prefix;
        }
    }
}
