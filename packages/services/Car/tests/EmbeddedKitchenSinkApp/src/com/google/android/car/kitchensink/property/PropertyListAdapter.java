/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.car.kitchensink.property;

import static java.lang.Integer.toHexString;

import android.car.VehiclePropertyIds;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.car.hardware.property.CarPropertyManager.CarPropertyEventCallback;
import android.content.Context;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.util.SparseLongArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import com.google.android.car.kitchensink.R;

import java.util.List;

class PropertyListAdapter extends BaseAdapter implements ListAdapter {
    private static final float DEFAULT_RATE = 1f;
    private static final String TAG = "PropertyListAdapter";
    private static final String OFF = "Off";
    private static final String ON = "On";
    private static final String MAX_SAMPLE_RATE = "Maximum sample rate";
    private static final String AVG_SAMPLE_RATE = "Average sample rate";
    private static final String[] DROP_MENU_FOR_CONTINUOUS =
            new String[]{OFF, MAX_SAMPLE_RATE, AVG_SAMPLE_RATE};
    private static final String[] DROP_MENU_FOR_ON_CHANGE = new String[]{OFF, ON};
    private final Context mContext;
    private final PropertyListEventListener mListener;
    private final CarPropertyManager mMgr;
    private final List<PropertyInfo> mPropInfo;
    private final TextView mTvEventLog;
    private String[] mItems;

    PropertyListAdapter(List<PropertyInfo> propInfo, CarPropertyManager mgr, TextView eventLog,
            ScrollView svEventLog, Context context) {
        mContext = context;
        mListener = new PropertyListEventListener(eventLog, svEventLog);
        mMgr = mgr;
        mPropInfo = propInfo;
        mTvEventLog = eventLog;
    }

    @Override
    public int getCount() {
        return mPropInfo.size();
    }

    @Override
    public Object getItem(int pos) {
        return mPropInfo.get(pos);
    }

    @Override
    public long getItemId(int pos) {
        return mPropInfo.get(pos).mPropId;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.property_list_item, null);
        }

        //Handle TextView and display string from your list
        TextView listItemText = (TextView) view.findViewById(R.id.tvPropertyName);
        listItemText.setText(mPropInfo.get(position).toString());

        Spinner dropdown = view.findViewById(R.id.tbRegisterPropertySpinner);

        CarPropertyConfig c = mPropInfo.get(position).mConfig;
        if (c.getChangeMode() == CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS) {
            mItems = DROP_MENU_FOR_CONTINUOUS;
        } else {
            mItems = DROP_MENU_FOR_ON_CHANGE;
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext,
                R.layout.custom_spinner_dropdown_item, mItems);

        adapter.setDropDownViewResource(R.layout.custom_spinner_dropdown_item);
        dropdown.setAdapter(adapter);
        dropdown.setSelection(0);
        dropdown.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int pos, long id) {
                String item = (String) adapterView.getItemAtPosition(pos);
                CarPropertyConfig c = mPropInfo.get(position).mConfig;
                int propId = c.getPropertyId();
                try {
                    if (OFF.equals(item)) {
                        mMgr.unregisterCallback(mListener, propId);
                    } else if (MAX_SAMPLE_RATE.equals(item)) {
                        mListener.addPropertySelectedSampleRate(propId, c.getMaxSampleRate());
                        mListener.updatePropertyStartTime(propId);
                        mListener.resetEventCountForProperty(propId);
                        mMgr.registerCallback(mListener, propId, c.getMaxSampleRate());
                    } else if (AVG_SAMPLE_RATE.equals(item)) {
                        mListener.addPropertySelectedSampleRate(propId,
                                (c.getMaxSampleRate() + c.getMinSampleRate()) / 2);
                        mListener.updatePropertyStartTime(propId);
                        mListener.resetEventCountForProperty(propId);
                        mMgr.registerCallback(mListener, propId,
                                (c.getMaxSampleRate() + c.getMinSampleRate()) / 2);
                    } else if (ON.equals(item)) {
                        mListener.addPropertySelectedSampleRate(propId,
                                DEFAULT_RATE);
                        mListener.updatePropertyStartTime(propId);
                        mListener.resetEventCountForProperty(propId);
                        mMgr.registerCallback(mListener, propId, DEFAULT_RATE);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Unhandled exception: ", e);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
                // do nothing.
            }
        });
        return view;
    }


    private static class PropertyListEventListener implements CarPropertyEventCallback {
        private final ScrollView mScrollView;
        private final TextView mTvLogEvent;
        private final SparseArray<Float> mPropSampleRate = new SparseArray<>();
        private final SparseLongArray mStartTime = new SparseLongArray();
        private final SparseIntArray mNumEvents = new SparseIntArray();

        PropertyListEventListener(TextView logEvent, ScrollView scrollView) {
            mScrollView = scrollView;
            mTvLogEvent = logEvent;
        }

        void addPropertySelectedSampleRate(Integer propId, Float sampleRate) {
            mPropSampleRate.put(propId, sampleRate);
        }

        void updatePropertyStartTime(Integer propId) {
            mStartTime.put(propId, System.currentTimeMillis());
        }

        void resetEventCountForProperty(Integer propId) {
            mNumEvents.put(propId, 0);
        }

        @Override
        public void onChangeEvent(CarPropertyValue value) {
            int propId = value.getPropertyId();

            mNumEvents.put(propId, mNumEvents.get(propId) + 1);
            mTvLogEvent.append(String.format("Event %1$s: time=%2$s propId=0x%3$s areaId=0x%3$s "
                            + "name=%4$s status=%5$s value=%6$s", mNumEvents.get(propId),
                    value.getTimestamp(), toHexString(propId), VehiclePropertyIds.toString(propId),
                    value.getStatus(), value.getValue()));
            if (mPropSampleRate.contains(propId)) {
                mTvLogEvent.append(
                        String.format(" selected sample rate=%1$s actual sample rate=%2$s\n",
                                mPropSampleRate.get(propId),
                                mNumEvents.get(propId) / (System.currentTimeMillis()
                                        - mStartTime.get(propId))));
            } else {
                mTvLogEvent.append("\n");
            }
            scrollToBottom();
        }

        @Override
        public void onErrorEvent(int propId, int areaId) {
            mTvLogEvent.append("Received error event propId=0x" + toHexString(propId)
                    + ", areaId=0x" + toHexString(areaId));
            scrollToBottom();
        }

        private void scrollToBottom() {
            mScrollView.post(new Runnable() {
                public void run() {
                    mScrollView.fullScroll(View.FOCUS_DOWN);
                    //mScrollView.smoothScrollTo(0, mTextStatus.getBottom());
                }
            });
        }

    }
}
