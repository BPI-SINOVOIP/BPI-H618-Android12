/*
 * Copyright 2020 HIMSA II K/S - www.himsa.com.
 * Represented by EHIMA - www.ehima.com
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

package com.android.bluetooth.le_audio;

import android.bluetooth.BluetoothDevice;

/**
 * Stack event sent via a callback from JNI to Java, or generated
 * internally by the LeAudio State Machine.
 */
public class LeAudioStackEvent {
    // Event types for STACK_EVENT message (coming from native in bt_le_audio.h)
    private static final int EVENT_TYPE_NONE = 0;
    public static final int EVENT_TYPE_CONNECTION_STATE_CHANGED = 1;
    public static final int EVENT_TYPE_GROUP_STATUS_CHANGED = 2;
    public static final int EVENT_TYPE_AUDIO_CONF_CHANGED = 4;
    public static final int EVENT_TYPE_SET_MEMBER_AVAILABLE = 5;
    // -------- DO NOT PUT ANY NEW UNICAST EVENTS BELOW THIS LINE-------------
    public static final int EVENT_TYPE_UNICAST_MAX = 7;

    // Do not modify without updating the HAL bt_le_audio.h files.
    // Match up with GroupStatus enum of bt_le_audio.h
    static final int CONNECTION_STATE_DISCONNECTED = 0;
    static final int CONNECTION_STATE_CONNECTING = 1;
    static final int CONNECTION_STATE_CONNECTED = 2;
    static final int CONNECTION_STATE_DISCONNECTING = 3;

    static final int GROUP_STATUS_IDLE = 0;
    static final int GROUP_STATUS_STREAMING = 1;
    static final int GROUP_STATUS_SUSPENDED = 2;
    static final int GROUP_STATUS_RECONFIGURED = 3;
    static final int GROUP_STATUS_DESTROYED = 4;

    public int type = EVENT_TYPE_NONE;
    public BluetoothDevice device;
    public int valueInt1 = 0;
    public int valueInt2 = 0;
    public int valueInt3 = 0;
    public int valueInt4 = 0;

    LeAudioStackEvent(int type) {
        this.type = type;
    }

    @Override
    public String toString() {
        // event dump
        StringBuilder result = new StringBuilder();
        result.append("LeAudioStackEvent {type:" + eventTypeToString(type));
        result.append(", device:" + device);
        result.append(", value1:" + eventTypeValue1ToString(type, valueInt1));
        result.append(", value2:" + eventTypeValue2ToString(type, valueInt2));
        result.append(", value3:" + eventTypeValue3ToString(type, valueInt3));
        result.append(", value4:" + eventTypeValue4ToString(type, valueInt4));
        result.append("}");
        return result.toString();
    }

    private static String eventTypeToString(int type) {
        switch (type) {
            case EVENT_TYPE_NONE:
                return "EVENT_TYPE_NONE";
            case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                return "EVENT_TYPE_CONNECTION_STATE_CHANGED";
            case EVENT_TYPE_GROUP_STATUS_CHANGED:
                return "EVENT_TYPE_GROUP_STATUS_CHANGED";
            case EVENT_TYPE_AUDIO_CONF_CHANGED:
                return "EVENT_TYPE_AUDIO_CONF_CHANGED";
            case EVENT_TYPE_SET_MEMBER_AVAILABLE:
                return "EVENT_TYPE_SET_MEMBER_AVAILABLE";
            default:
                return "EVENT_TYPE_UNKNOWN:" + type;
        }
    }

    private static String eventTypeValue1ToString(int type, int value) {
        switch (type) {
            case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                switch (value) {
                    case CONNECTION_STATE_DISCONNECTED:
                        return  "CONNECTION_STATE_DISCONNECTED";
                    case CONNECTION_STATE_CONNECTING:
                        return  "CONNECTION_STATE_CONNECTING";
                    case CONNECTION_STATE_CONNECTED:
                        return  "CONNECTION_STATE_CONNECTED";
                    case CONNECTION_STATE_DISCONNECTING:
                        return  "CONNECTION_STATE_DISCONNECTING";
                    default:
                        return "UNKNOWN";
                }
            case EVENT_TYPE_GROUP_STATUS_CHANGED:
                // same as EVENT_TYPE_GROUP_STATUS_CHANGED
            case EVENT_TYPE_SET_MEMBER_AVAILABLE:
                // same as EVENT_TYPE_GROUP_STATUS_CHANGED
            case EVENT_TYPE_AUDIO_CONF_CHANGED:
                // FIXME: It should have proper direction names here
                return "{direction:" + value + "}";
            default:
                break;
        }
        return Integer.toString(value);
    }

    private static String eventTypeValue2ToString(int type, int value) {
        switch (type) {
            case EVENT_TYPE_GROUP_STATUS_CHANGED:
                switch (value) {
                    case GROUP_STATUS_IDLE:
                        return "GROUP_STATUS_IDLE";
                    case GROUP_STATUS_STREAMING:
                        return "GROUP_STATUS_STREAMING";
                    case GROUP_STATUS_SUSPENDED:
                        return "GROUP_STATUS_SUSPENDED";
                    case GROUP_STATUS_RECONFIGURED:
                        return "GROUP_STATUS_RECONFIGURED";
                    case GROUP_STATUS_DESTROYED:
                        return "GROUP_STATUS_DESTROYED";
                    default:
                        break;
                }
                break;
            case EVENT_TYPE_AUDIO_CONF_CHANGED:
                return "{group_id:" + Integer.toString(value) + "}";
            default:
                break;
        }
        return Integer.toString(value);
    }

    private static String eventTypeValue3ToString(int type, int value) {
        switch (type) {
            case EVENT_TYPE_GROUP_STATUS_CHANGED:
                return "{group_flags:" + Integer.toString(value) + "}";
            case EVENT_TYPE_AUDIO_CONF_CHANGED:
                // FIXME: It should have proper location names here
                return "{snk_audio_loc:" + value + "}";
            default:
                break;
        }
        return Integer.toString(value);
    }

    private static String eventTypeValue4ToString(int type, int value) {
        switch (type) {
            case EVENT_TYPE_AUDIO_CONF_CHANGED:
                // FIXME: It should have proper location names here
                return "{src_audio_loc:" + value + "}";
            default:
                break;
        }
        return Integer.toString(value);
    }
}
