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
package com.android.car.ui.toolbar;

import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.View;

import androidx.annotation.XmlRes;

import com.android.car.ui.R;
import com.android.car.ui.utils.CarUiUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * A class for reading {@link MenuItem MenuItems} from xml files.
 *
 * Apps don't need to use this, as there also exists a {@link ToolbarController#setMenuItems(int)}
 * function that will include the same functionality.
 */
public class MenuItemXmlParserUtil {

    private MenuItemXmlParserUtil() {}

    /**
     * Reads a list of {@link MenuItem MenuItems} from the provided xml file.
     *
     * Apps don't need to use this, as there also exists a
     * {@link ToolbarController#setMenuItems(int)} function that will include the same
     * functionality.
     */
    public static List<MenuItem> readMenuItemList(Context c, @XmlRes int resId) {
        if (resId == 0) {
            return new ArrayList<>();
        }

        try (XmlResourceParser parser = c.getResources().getXml(resId)) {
            AttributeSet attrs = Xml.asAttributeSet(parser);
            List<MenuItem> menuItems = new ArrayList<>();

            parser.next();
            parser.next();
            parser.require(XmlPullParser.START_TAG, null, "MenuItems");
            while (parser.next() != XmlPullParser.END_TAG) {
                menuItems.add(readMenuItem(c, parser, attrs));
            }

            return menuItems;
        } catch (XmlPullParserException | IOException e) {
            throw new RuntimeException("Unable to parse Menu Items", e);
        }
    }

    private static MenuItem readMenuItem(Context c, XmlResourceParser parser, AttributeSet attrs)
            throws XmlPullParserException, IOException {

        parser.require(XmlPullParser.START_TAG, null, "MenuItem");

        TypedArray a = c.obtainStyledAttributes(attrs, R.styleable.CarUiToolbarMenuItem);
        try {
            int id = a.getResourceId(R.styleable.CarUiToolbarMenuItem_id, View.NO_ID);
            String title = a.getString(R.styleable.CarUiToolbarMenuItem_title);
            Drawable icon = a.getDrawable(R.styleable.CarUiToolbarMenuItem_icon);
            boolean isSearch = a.getBoolean(R.styleable.CarUiToolbarMenuItem_search, false);
            boolean isSettings = a.getBoolean(R.styleable.CarUiToolbarMenuItem_settings, false);
            boolean tinted = a.getBoolean(R.styleable.CarUiToolbarMenuItem_tinted, true);
            boolean visible = a.getBoolean(R.styleable.CarUiToolbarMenuItem_visible, true);
            boolean showIconAndTitle = a.getBoolean(
                    R.styleable.CarUiToolbarMenuItem_showIconAndTitle, false);
            boolean checkable = a.getBoolean(R.styleable.CarUiToolbarMenuItem_checkable, false);
            boolean checked = a.getBoolean(R.styleable.CarUiToolbarMenuItem_checked, false);
            boolean checkedExists = a.hasValue(R.styleable.CarUiToolbarMenuItem_checked);
            boolean activatable = a.getBoolean(R.styleable.CarUiToolbarMenuItem_activatable, false);
            boolean activated = a.getBoolean(R.styleable.CarUiToolbarMenuItem_activated, false);
            boolean activatedExists = a.hasValue(R.styleable.CarUiToolbarMenuItem_activated);
            int displayBehaviorInt = a.getInt(R.styleable.CarUiToolbarMenuItem_displayBehavior, 0);
            int uxRestrictions = a.getInt(R.styleable.CarUiToolbarMenuItem_uxRestrictions, 0);
            String onClickMethod = a.getString(R.styleable.CarUiToolbarMenuItem_onClick);
            MenuItem.OnClickListener onClickListener = null;

            if (onClickMethod != null) {
                Activity activity = CarUiUtils.getActivity(c);
                if (activity == null) {
                    throw new RuntimeException("Couldn't find an activity for the MenuItem");
                }

                try {
                    Method m = activity.getClass().getMethod(onClickMethod, MenuItem.class);
                    onClickListener = i -> {
                        try {
                            m.invoke(activity, i);
                        } catch (InvocationTargetException | IllegalAccessException e) {
                            throw new RuntimeException("Couldn't call the MenuItem's listener", e);
                        }
                    };
                } catch (NoSuchMethodException e) {
                    throw new RuntimeException("OnClick method "
                            + onClickMethod + "(MenuItem) not found in your activity", e);
                }
            }

            MenuItem.DisplayBehavior displayBehavior = displayBehaviorInt == 0
                    ? MenuItem.DisplayBehavior.ALWAYS
                    : MenuItem.DisplayBehavior.NEVER;

            parser.next();
            parser.require(XmlPullParser.END_TAG, null, "MenuItem");

            MenuItem.Builder builder = MenuItem.builder(c)
                    .setId(id)
                    .setTitle(title)
                    .setIcon(icon)
                    .setOnClickListener(onClickListener)
                    .setUxRestrictions(uxRestrictions)
                    .setTinted(tinted)
                    .setVisible(visible)
                    .setShowIconAndTitle(showIconAndTitle)
                    .setDisplayBehavior(displayBehavior);

            if (isSearch) {
                builder.setToSearch();
            }

            if (isSettings) {
                builder.setToSettings();
            }

            if (checkable || checkedExists) {
                builder.setChecked(checked);
            }

            if (activatable || activatedExists) {
                builder.setActivated(activated);
            }

            return builder.build();
        } finally {
            a.recycle();
        }
    }
}
