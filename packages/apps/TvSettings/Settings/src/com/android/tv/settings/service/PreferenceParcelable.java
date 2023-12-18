/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.tv.settings.service;

import android.content.Intent;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.ArrayMap;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class PreferenceParcelable implements Parcelable {
    public static final byte TYPE_RPEFERENCE = 0;
    public static final byte TYPE_PREFERENCE_CATEGORY = 1;
    public static final byte TYPE_PREFERENCE_ACCESS_POINT = 2;
    public static final byte TYPE_PREFERENCE_WIFI_COLLAPSE_CATEGORY = 3;

    private final String[] key;
    private String title;
    private String summary;
    private String contentDescription;
    private Bundle extras;
    private Intent intent;

    // 0 : preference, 1 : preferenceCategory, 2 : AccessPointPreference
    private byte type;

    // Provide extra information for particular type
    private Map<String, String> infoMap;

    // 0 : not updated, 1 : unchecked, 2 : checked
    private byte checked;

    // 0 : not updated, 1 : ininvisble, 2: visible
    private byte visible;
    private List<PreferenceParcelable> childPrefParcelables;

    public List<PreferenceParcelable> getChildPreferences() {
        return childPrefParcelables;
    }

    public void setChildPrefParcelables(
            List<PreferenceParcelable> childPrefParcelables) {
        this.childPrefParcelables = childPrefParcelables;
    }

    public List<PreferenceParcelable> getChildPrefParcelables() {
        if (childPrefParcelables == null) {
            childPrefParcelables = new ArrayList<>();
        }
        return childPrefParcelables;
    }

    public PreferenceParcelable(String key) {
        this.key = new String[]{key};
    }

    public PreferenceParcelable(String[] key) {
        this.key = key;
    }

    public PreferenceParcelable(String[] key, String title) {
        this.key = key;
        this.title = title;
    }

    public PreferenceParcelable(String[] key, String title, String summary) {
        this(key, title);
        this.summary = summary;
    }

    public String[] getKey() {
        return key;
    }

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getSummary() {
        return summary;
    }

    public void setSummary(String summary) {
        this.summary = summary;
    }


    public String getContentDescription() {
        return contentDescription;
    }

    public int getType() {
        return type;
    }

    public void setType(byte type) {
        this.type = type;
    }

    public Map<String, String> getInfoMap() {
        if (infoMap == null) {
            infoMap = new ArrayMap<>();
        }
        return infoMap;
    }

    public void setInfoMap(Map<String, String> info) {
        this.infoMap = info;
    }

    public void addInfo(String key, String value) {
        if (infoMap == null) {
            infoMap = new ArrayMap<>();
        }
        infoMap.put(key, value);
    }

    public String getInfo(String key) {
        if (infoMap == null || !infoMap.containsKey(key)) {
            return null;
        }
        return infoMap.get(key);
    }


    public void setContentDescription(String contentDescription) {
        this.contentDescription = contentDescription;
    }

    public byte getChecked() {
        return checked;
    }

    public void setChecked(byte checked) {
        this.checked = checked;
    }

    public void setChecked(boolean checked) {
        setChecked(ServiceUtil.getChecked(checked));
    }

    public void setVisible(boolean visible) {
        setVisible(ServiceUtil.getVisible(visible));
    }

    public byte getVisible() {
        return visible;
    }

    public void setVisible(byte visible) {
        this.visible = visible;
    }

    public Bundle getExtras() {
        return extras;
    }

    public void setExtras(Bundle extras) {
        this.extras = extras;
    }

    public Intent getIntent() {
        return intent;
    }

    public void setIntent(Intent intent) {
        this.intent = intent;
    }

    public void initChildPreferences() {
        childPrefParcelables = new ArrayList<>();
    }

    public void addChildPrefParcelable(PreferenceParcelable childPrefParcelable) {
        if (childPrefParcelables == null) {
            childPrefParcelables = new ArrayList<>();
        }
        childPrefParcelables.add(childPrefParcelable);
    }

    @Override
    public String toString() {
        return "PreferenceParcelable{" +
                "key='" + Arrays.toString(key) + '\'' +
                ", title='" + title + '\'' +
                ", summary='" + summary + '\'' +
                ", contentDescription='" + contentDescription + '\'' +
                ", type=" + type +
                ", extras=" + extras +
                ", intent=" + intent +
                ", infoMap=" + infoMap +
                ", checked=" + checked +
                ", visible=" + visible +
                ", childPrefParcelables=" + childPrefParcelables +
                '}';
    }

    public PreferenceParcelable immutableCopy() {
        PreferenceParcelable copy = new PreferenceParcelable(Arrays.copyOf(key, key.length));
        copy.setTitle(title);
        copy.setSummary(summary);
        copy.setType(type);
        copy.setChecked(checked);
        copy.setVisible(visible);
        copy.setContentDescription(contentDescription);
        if (extras != null) {
            copy.setExtras(new Bundle(extras));
        }
        if (intent != null) {
            copy.setIntent(new Intent(intent));
        }
        Map<String, String> infoMapCopy = new ArrayMap<>();
        if (infoMap != null) {
            infoMapCopy.putAll(infoMap);
        }
        copy.setInfoMap(infoMapCopy);
        if (childPrefParcelables != null) {
            copy.setChildPrefParcelables(childPrefParcelables.stream()
                    .map(PreferenceParcelable::immutableCopy).collect(Collectors.toList()));
        }
        return copy;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeStringArray(key);
        dest.writeString(title);
        dest.writeString(summary);
        dest.writeString(contentDescription);
        dest.writeByte(checked);
        dest.writeByte(visible);
        dest.writeByte(type);
        dest.writeBundle(extras);
        dest.writeParcelable(intent, flags);
        dest.writeMap(infoMap);
        dest.writeParcelableList(childPrefParcelables, flags);
    }

    public static final Creator<PreferenceParcelable> CREATOR =
            new Creator<PreferenceParcelable>() {
                @Override
                public PreferenceParcelable createFromParcel(Parcel source) {
                    PreferenceParcelable preferenceParcelable = new PreferenceParcelable(
                            source.createStringArray());
                    preferenceParcelable.setTitle(source.readString());
                    preferenceParcelable.setSummary(source.readString());
                    preferenceParcelable.setContentDescription(source.readString());
                    preferenceParcelable.setChecked(source.readByte());
                    preferenceParcelable.setVisible(source.readByte());
                    preferenceParcelable.setType(source.readByte());
                    preferenceParcelable.setExtras(source.readBundle());
                    preferenceParcelable.setIntent(
                            source.readParcelable(Intent.class.getClassLoader()));
                    source.readMap(preferenceParcelable.getInfoMap(), Map.class.getClassLoader());
                    source.readParcelableList(
                            preferenceParcelable.getChildPrefParcelables(),
                            PreferenceParcelable.class.getClassLoader());
                    return preferenceParcelable;
                }

                @Override
                public PreferenceParcelable[] newArray(int size) {
                    return new PreferenceParcelable[size];
                }
            };
}