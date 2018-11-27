package com.nalindquist.ifmanager;

import android.content.Context;
import android.text.InputType;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;

public class PluginLayout {
    private static final String HEADER_TRIGGER = "Trigger";
    private static final String HEADER_ACTION  = "Action";
    private static final String HEADER_REMOVE  = " ";
    private static final String[] HEADERS = {
            HEADER_TRIGGER,
            HEADER_ACTION,
            HEADER_REMOVE,
    };

    private static final String PROMPT_DATA     = "Data:";
    private static final String PROMPT_FUNCTION = "Function:";
    private static final String PROMPT_VALUE    = "Value:";
    private static final String PROMPT_ACTION   = "Action:";
    private static final String TEXT_ADD    = "Add";
    private static final String TEXT_REMOVE = "Remove";

    private static final int DATA_TYPE_WIFI_SIGNAL_LEVEL = 0;
    private static final int[] DATA_TYPES = {
            DATA_TYPE_WIFI_SIGNAL_LEVEL,
    };
    private static final String[] DATA_TYPES_TEXT = {
            "Wifi Signal Level",
    };

    private static final int FUNCTION_LT = 0;
    private static final int FUNCTION_EQ = 1;
    private static final int FUNCTION_GT = 2;
    private static final int[] FUNCTIONS = {
            FUNCTION_LT,
            FUNCTION_EQ,
            FUNCTION_GT,
    };
    private static final String[] FUNCTIONS_TEXT = {
            "<",
            "=",
            ">",
    };

    private static final int ACTION_DISCONNECT_WIFI = 0;
    private static final int[] ACTIONS = {
            ACTION_DISCONNECT_WIFI,
    };
    private static final String[] ACTIONS_TEXT = {
            "Disconnect Wifi",
    };

    private static final String TAG = "ifmanager-ui";

    private Context m_context;
    private Object m_updater;
    private Method m_updateNative;
    private RuleAdapter m_adapter;
    private ArrayList<Rule> m_rules;
    private int m_dataSelected;
    private int m_functionSelected;
    private int m_actionSelected;

    public PluginLayout() {
        m_context = null;
        m_updater = null;
        m_updateNative = null;
        m_adapter = null;
        m_rules = new ArrayList<>();
        m_dataSelected = 0;
        m_functionSelected = 0;
        m_actionSelected = 0;
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
        m_adapter = new RuleAdapter();

        doNativeUpdate();

        LinearLayout ll = new LinearLayout(context);
        ll.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT));
        ll.setOrientation(LinearLayout.VERTICAL);
        ll.setGravity(Gravity.CENTER);

        ListView rules = new ListView(context);
        rules.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        rules.setAdapter(m_adapter);

        Space space1 = new Space(context);
        space1.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,0, 1.0f));

        LinearLayout input1 = new LinearLayout(context);
        input1.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input1.setOrientation(LinearLayout.HORIZONTAL);
        input1.setGravity(Gravity.CENTER);

        TextView dataPrompt = new TextView(context);
        dataPrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        dataPrompt.setText(PROMPT_DATA);

        Spinner dataSpinner = new Spinner(context);
        dataSpinner.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        dataSpinner.setAdapter(new TextAdapter(DATA_TYPES, DATA_TYPES_TEXT));
        dataSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                m_dataSelected = position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                m_dataSelected = -1;
            }
        });

        input1.addView(dataPrompt);
        input1.addView(dataSpinner);

        LinearLayout input2 = new LinearLayout(context);
        input2.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input2.setOrientation(LinearLayout.HORIZONTAL);
        input2.setGravity(Gravity.CENTER);

        TextView functionPrompt = new TextView(context);
        functionPrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        functionPrompt.setText(PROMPT_FUNCTION);

        Spinner functionSpinner = new Spinner(context);
        functionSpinner.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        functionSpinner.setAdapter(new TextAdapter(FUNCTIONS, FUNCTIONS_TEXT));
        functionSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                m_functionSelected = position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                m_functionSelected = -1;
            }
        });

        input2.addView(functionPrompt);
        input2.addView(functionSpinner);

        LinearLayout input3 = new LinearLayout(context);
        input3.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input3.setOrientation(LinearLayout.HORIZONTAL);
        input3.setGravity(Gravity.CENTER);

        TextView valuePrompt = new TextView(context);
        valuePrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        valuePrompt.setText(PROMPT_VALUE);

        final EditText valueInput = new EditText(context);
        valueInput.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        valueInput.setInputType(InputType.TYPE_CLASS_TEXT);

        input3.addView(valuePrompt);
        input3.addView(valueInput);

        LinearLayout input4 = new LinearLayout(context);
        input4.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input4.setOrientation(LinearLayout.HORIZONTAL);
        input4.setGravity(Gravity.CENTER);

        TextView actionPrompt = new TextView(context);
        actionPrompt.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        actionPrompt.setText(PROMPT_ACTION);

        Spinner actionSpinner = new Spinner(context);
        actionSpinner.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f));
        actionSpinner.setAdapter(new TextAdapter(ACTIONS, ACTIONS_TEXT));
        actionSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                m_actionSelected = position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                m_actionSelected = -1;
            }
        });

        input4.addView(actionPrompt);
        input4.addView(actionSpinner);

        LinearLayout input5 = new LinearLayout(context);
        input5.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        input5.setOrientation(LinearLayout.HORIZONTAL);
        input5.setGravity(Gravity.CENTER);

        Button addButton = new Button(context);
        addButton.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        addButton.setText(TEXT_ADD);
        addButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                m_rules.add(new Rule(
                        m_dataSelected,
                        m_functionSelected,
                        Double.parseDouble(valueInput.getText().toString()),
                        m_actionSelected));
                doNativeUpdate();
                m_adapter.notifyDataSetChanged();
            }
        });

        input5.addView(addButton);

        Space space2 = new Space(context);
        space2.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,0, 1.0f));

        ll.addView(rules);
        ll.addView(space1);
        ll.addView(input1);
        ll.addView(input2);
        ll.addView(input3);
        ll.addView(input4);
        ll.addView(input5);
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
            byte[] buf = new byte[m_rules.size()*20];
            ByteBuffer bb = ByteBuffer.wrap(buf);
            bb.order(ByteOrder.LITTLE_ENDIAN);

            for (int i = 0; i < m_rules.size(); i++) {
                int j = i*20;
                Rule r = m_rules.get(i);

                Log.d(TAG, "data = " + r.data);
                Log.d(TAG, "function = " + r.function);
                Log.d(TAG, "value = " + r.value);
                Log.d(TAG, "action = " + r.action);

                bb.putInt(j, r.data);
                bb.putInt(j+4, r.function);
                bb.putDouble(j+8, r.value);
                bb.putInt(j+16, r.action);
            }

            try {
                m_updateNative.invoke(m_updater, buf);
            }
            catch (Exception e) {
                Log.e(TAG, "Unable to invoke updateNative(): " + e.getMessage());
            }
        }
    }

    private class RuleAdapter extends BaseAdapter {
        RuleAdapter() {}

        @Override
        public int getCount() {
            return 1;
        }

        @Override
        public Object getItem(int i) {
            return m_rules;
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

            for (String s: HEADERS) {
                TextView text = new TextView(m_context);
                text.setText(s);
                header.addView(text);
            }

            tl.addView(header);

            int j = 0;
            for (Rule r: m_rules) {
                final int index = j;

                TableRow row = new TableRow(m_context);
                row.setLayoutParams(new TableLayout.LayoutParams(
                        TableLayout.LayoutParams.MATCH_PARENT,
                        TableLayout.LayoutParams.WRAP_CONTENT));

                TextView trigger = new TextView(m_context);
                StringBuilder triggerText = new StringBuilder(64);
                triggerText.append(DATA_TYPES_TEXT[r.data]).append(" ");
                triggerText.append(FUNCTIONS_TEXT[r.function]).append(" ");
                triggerText.append(r.value);
                trigger.setText(triggerText.toString());

                TextView action = new TextView(m_context);
                action.setText(ACTIONS_TEXT[r.action]);

                Button remove = new Button(PluginLayout.this.m_context);
                remove.setText(TEXT_REMOVE);
                remove.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        m_rules.remove(index);
                        doNativeUpdate();
                        notifyDataSetChanged();
                    }
                });

                row.addView(trigger);
                row.addView(action);
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

    private class TextAdapter extends BaseAdapter {
        private int[] m_values;
        private String[] m_text;

        TextAdapter(int[] values, String[] text) {
            m_values = values;
            m_text = text;
        }

        @Override
        public int getCount() {
            return m_values.length;
        }

        @Override
        public Object getItem(int i) {
            return m_values[i];
        }

        @Override
        public long getItemId(int i) {
            return i;
        }

        @Override
        public View getView(int i, View view, ViewGroup parent) {
            TextView v = new TextView(m_context);
            v.setText(m_text[i]);

            return v;
        }
    }

    private class Rule {
        int data;
        int function;
        double value;
        int action;

        Rule(int data, int function, double value, int action) {
            this.data = data;
            this.function = function;
            this.value = value;
            this.action = action;
        }
    }
}
