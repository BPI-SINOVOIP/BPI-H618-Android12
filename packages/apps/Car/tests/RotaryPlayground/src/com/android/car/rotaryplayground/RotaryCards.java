/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.rotaryplayground;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.android.car.ui.FocusArea;

/** Fragment to demo a layout with cards that are FocusArea containers. */
public class RotaryCards extends Fragment {

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.rotary_cards, container, false);

        // This button will be removed and immediately added back after click. So the focus
        // highlight will jump to another view after rotary click.
        FocusArea focusArea5 = view.findViewById(R.id.focus_area5);
        Button button5a = view.findViewById(R.id.button_5a);
        button5a.setOnClickListener(v -> {
            int index = focusArea5.indexOfChild(button5a);
            focusArea5.removeView(button5a);
            focusArea5.addView(button5a, index);
        });

        // The background of this button will be changed after click. The focus highlight will stay
        // on this Button after click.
        Button button5b = view.findViewById(R.id.button_5b);
        boolean[] stopped = {false};
        button5b.setOnClickListener(v -> {
            stopped[0] = !stopped[0];
            Drawable drawable = view.getContext().getDrawable(
                    stopped[0] ? R.drawable.ic_stop : R.drawable.ic_play_arrow);
            button5b.setBackground(drawable);
        });

        // This button will be removed and immediately added back after click. It's not focusable
        // but its container is focusable. The focus highlight will stay on the container after
        // click.
        ViewGroup button5cContainer = view.findViewById(R.id.button_5c_container);
        Button button5c = view.findViewById(R.id.button_5c);
        button5c.setFocusable(false);
        button5cContainer.setOnClickListener(v -> {
            button5cContainer.removeView(button5c);
            button5cContainer.addView(button5c, 0);
        });

        // This button will be removed then added back and request focus explicitly after click.
        // So the focus highlight will jump to another view then jump back after rotary click.
        Button button5d = view.findViewById(R.id.button_5d);
        button5d.setOnClickListener(v -> {
            boolean needRestoreFocus = button5d.isFocused();
            int index = focusArea5.indexOfChild(button5d);
            focusArea5.removeView(button5d);
            focusArea5.addView(button5d, index);
            if (needRestoreFocus) {
                button5d.requestFocus();
            }
        });

        // This button will be disabled after click. So the focus highlight will jump to another
        // view after rotary click.
        Button button5e = view.findViewById(R.id.button_5e);
        button5e.setOnClickListener(v -> button5e.setEnabled(!button5e.isEnabled()));

        // This button will appear disabled but is not disabled after click. So the focus
        // highlight will stay after rotary click.
        CustomButton button5f = view.findViewById(R.id.button_5f);
        button5f.setOnClickListener(v -> button5f.setRotaryEnabled(!button5f.isRotaryEnabled()));

        return view;
    }
}
