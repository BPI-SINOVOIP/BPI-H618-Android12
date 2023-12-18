/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.car.ui.paintbooth.dialogs;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.AlertDialogBuilder;
import com.android.car.ui.FocusArea;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.paintbooth.R;
import com.android.car.ui.recyclerview.CarUiContentListItem;
import com.android.car.ui.recyclerview.CarUiListItemAdapter;
import com.android.car.ui.recyclerview.CarUiRadioButtonListItem;
import com.android.car.ui.recyclerview.CarUiRadioButtonListItemAdapter;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.toolbar.NavButtonMode;
import com.android.car.ui.toolbar.ToolbarController;

import java.util.ArrayList;
import java.util.List;

/**
 * Activity that shows different dialogs from the device default theme.
 */
public class DialogsActivity extends Activity implements InsetsChangedListener {

    private final List<Pair<Integer, View.OnClickListener>> mButtons = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.car_ui_recycler_view_activity);
        ToolbarController toolbar = CarUi.requireToolbar(this);
        toolbar.setTitle(getTitle());
        toolbar.setNavButtonMode(NavButtonMode.BACK);

        mButtons.add(Pair.create(R.string.dialog_show_dialog,
                v -> showDialog()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_icon,
                v -> showDialogWithIcon()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_edit,
                v -> showDialogWithTextBox()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_only_positive,
                v -> showDialogWithOnlyPositiveButton()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_no_button,
                v -> showDialogWithNoButtonProvided()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_checkbox,
                v -> showDialogWithCheckbox()));
        mButtons.add(Pair.create(R.string.dialog_show_dialog_no_title,
                v -> showDialogWithoutTitle()));
        mButtons.add(Pair.create(R.string.dialog_show_toast,
                v -> showToast()));
        mButtons.add(Pair.create(R.string.dialog_show_subtitle,
                v -> showDialogWithSubtitle()));
        mButtons.add(Pair.create(R.string.dialog_show_subtitle_and_icon,
                v -> showDialogWithSubtitleAndIcon()));
        mButtons.add(Pair.create(R.string.dialog_show_long_subtitle_and_icon,
                v -> showDialogWithLongSubtitleAndIcon()));
        mButtons.add(Pair.create(R.string.dialog_show_single_choice,
                v -> showDialogWithSingleChoiceItems()));
        mButtons.add(Pair.create(R.string.dialog_show_list_items_without_default_button,
                v -> showDialogWithListItemsWithoutDefaultButton()));
        mButtons.add(Pair.create(R.string.dialog_show_permission_dialog,
                v -> showPermissionDialog()));
        mButtons.add(Pair.create(R.string.dialog_show_multi_permission_dialog,
                v -> showMultiPermissionDialog()));
        mButtons.add(Pair.create(R.string.dialog_show_foreground_permission_dialog,
                v -> showForegroundPermissionDialog()));
        mButtons.add(Pair.create(R.string.dialog_show_background_permission_dialog,
                v -> showBackgroundPermissionDialog()));

        CarUiRecyclerView recyclerView = requireViewById(R.id.list);
        recyclerView.setAdapter(mAdapter);
    }

    private void showDialog() {
        new AlertDialogBuilder(this)
                .setTitle(getString(R.string.standard_alert_dialog))
                .setMessage(getString(R.string.my_message))
                .setNeutralButton(getString(R.string.neutral), (dialogInterface, which) -> {
                })
                .setPositiveButton(getString(R.string.ok), (dialogInterface, which) -> {
                })
                .setNegativeButton(getString(R.string.cancel), (dialogInterface, which) -> {
                })
                .show();
    }

    private void showDialogWithIcon() {
        new AlertDialogBuilder(this)
                .setTitle(getString(R.string.alert_dialog_with_icon))
                .setMessage(getString(R.string.my_message))
                .setIcon(R.drawable.ic_tracklist)
                .show();
    }

    private void showDialogWithNoButtonProvided() {
        new AlertDialogBuilder(this)
                .setTitle(getString(R.string.standard_alert_dialog))
                .show();
    }

    private void showDialogWithCheckbox() {
        new AlertDialogBuilder(this)
                .setTitle(R.string.custom_dialog_box)
                .setMultiChoiceItems(
                        new CharSequence[]{"I am a checkbox"},
                        new boolean[]{false},
                        (dialog, which, isChecked) -> {
                        })
                .setPositiveButton(getString(R.string.ok), (dialogInterface, which) -> {
                })
                .setNegativeButton(getString(R.string.cancel), (dialogInterface, which) -> {
                })
                .show();
    }

    private void showDialogWithTextBox() {
        new AlertDialogBuilder(this)
                .setTitle(R.string.standard_alert_dialog)
                .setEditBox(getString(R.string.edit_me_please), null, null)
                .setAutoDescUpdateForWidescreen(true)
                .setEditTextTitleAndDescForWideScreen("title", "desc from app")
                .setPositiveButton("OK", (dialogInterface, i) -> {
                })
                .show();
    }

    private void showDialogWithOnlyPositiveButton() {
        new AlertDialogBuilder(this)
                .setTitle(R.string.standard_alert_dialog)
                .setMessage(getString(R.string.my_message))
                .setPositiveButton(getString(R.string.ok), (dialogInterface, i) -> {
                })
                .show();
    }

    private void showDialogWithoutTitle() {
        new AlertDialogBuilder(this)
                .setMessage(R.string.no_title_message)
                .setPositiveButton(getString(R.string.ok), (dialogInterface, i) -> {
                })
                .setNegativeButton(getString(R.string.cancel), (dialogInterface, which) -> {
                })
                .show();
    }

    private void showToast() {
        Toast.makeText(this, "Toast message looks like this", Toast.LENGTH_LONG).show();
    }

    private void showDialogWithSubtitle() {
        new AlertDialogBuilder(this)
                .setTitle(R.string.my_title)
                .setSubtitle(R.string.my_subtitle)
                .setMessage(getString(R.string.my_message))
                .show();
    }

    private void showDialogWithSingleChoiceItems() {
        ArrayList<CarUiRadioButtonListItem> data = new ArrayList<>();

        CarUiRadioButtonListItem item = new CarUiRadioButtonListItem();
        item.setTitle(getString(R.string.first_item));
        data.add(item);

        item = new CarUiRadioButtonListItem();
        item.setTitle(getString(R.string.second_item));
        data.add(item);

        item = new CarUiRadioButtonListItem();
        item.setTitle(getString(R.string.third_item));
        data.add(item);

        new AlertDialogBuilder(this)
                .setTitle(R.string.select_one_option)
                .setSubtitle(R.string.select_one_option_at_a_time)
                .setSingleChoiceItems(new CarUiRadioButtonListItemAdapter(data), null)
                .show();
    }

    private void showDialogWithListItemsWithoutDefaultButton() {
        ArrayList<CarUiContentListItem> data = new ArrayList<>();
        AlertDialog[] dialog = new AlertDialog[1];

        CarUiContentListItem item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle("First item");
        item.setOnItemClickedListener(i -> dialog[0].dismiss());
        data.add(item);


        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle("Second item");
        item.setOnItemClickedListener(i -> dialog[0].dismiss());
        data.add(item);

        item = new CarUiContentListItem(CarUiContentListItem.Action.NONE);
        item.setTitle("Third item");
        item.setOnItemClickedListener(i -> dialog[0].dismiss());
        data.add(item);

        dialog[0] = new AlertDialogBuilder(this)
                .setTitle("Select one option.")
                .setSubtitle("Ony one option may be selected at a time")
                .setAdapter(new CarUiListItemAdapter(data))
                .setAllowDismissButton(false)
                .show();
    }

    private void showDialogWithSubtitleAndIcon() {
        new AlertDialogBuilder(this)
                .setTitle(R.string.my_title)
                .setSubtitle(R.string.my_subtitle)
                .setMessage(getString(R.string.my_message))
                .setIcon(R.drawable.ic_tracklist)
                .show();
    }

    private void showDialogWithLongSubtitleAndIcon() {
        new AlertDialogBuilder(this)
                .setTitle(getString(R.string.long_title))
                .setSubtitle(R.string.long_subtitle)
                .setMessage(getString(R.string.my_message))
                .setIcon(R.drawable.ic_tracklist)
                .show();
    }

    private void showPermissionDialog() {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Permission already granted. Remove CAMERA permission from "
                    + "Settings > All apps > PaintBooth", Toast.LENGTH_SHORT).show();
            return;
        }
        requestPermissions(new String[]{Manifest.permission.CAMERA}, 1);
    }

    private void showMultiPermissionDialog() {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(Manifest.permission.SEND_SMS)
                    == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(Manifest.permission.READ_CONTACTS)
                    == PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Permissions are already granted. Remove CAMERA, SEND_SMS or "
                    + "READ_CONTACTS permission from Settings > All apps > PaintBooth",
                    Toast.LENGTH_SHORT).show();
            return;
        }
        requestPermissions(new String[]{Manifest.permission.CAMERA,
                Manifest.permission.READ_CONTACTS, Manifest.permission.SEND_SMS}, 1);
    }

    private void showForegroundPermissionDialog() {
        requestPermissions(new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, 1);
    }

    private void showBackgroundPermissionDialog() {
        requestPermissions(new String[]{Manifest.permission.ACCESS_BACKGROUND_LOCATION}, 1);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        StringBuilder sb = new StringBuilder();
        sb.append("Permission ");
        for (int i = 0; i < permissions.length; i++) {
            sb.append(permissions[i]);
            sb.append("=");
            sb.append(grantResults[i] == PackageManager.PERMISSION_GRANTED ? "granted" : "denied");
            sb.append("\n");
        }
        Toast.makeText(this, sb.toString(), Toast.LENGTH_SHORT).show();
    }

    private static class ViewHolder extends RecyclerView.ViewHolder {

        private final Button mButton;

        ViewHolder(View itemView) {
            super(itemView);
            mButton = itemView.requireViewById(R.id.button);
        }

        public void bind(Integer title, View.OnClickListener listener) {
            mButton.setText(title);
            mButton.setOnClickListener(listener);
        }
    }

    private final RecyclerView.Adapter<ViewHolder> mAdapter =
            new RecyclerView.Adapter<ViewHolder>() {
                @Override
                public int getItemCount() {
                    return mButtons.size();
                }

                @Override
                public ViewHolder onCreateViewHolder(ViewGroup parent, int position) {
                    View item =
                            LayoutInflater.from(parent.getContext()).inflate(R.layout.list_item,
                                    parent, false);
                    return new ViewHolder(item);
                }

                @Override
                public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
                    Pair<Integer, View.OnClickListener> pair = mButtons.get(position);
                    holder.bind(pair.first, pair.second);
                }
            };

    @Override
    public void onCarUiInsetsChanged(@NonNull Insets insets) {
        FocusArea focusArea = requireViewById(R.id.focus_area);
        focusArea.setBoundsOffset(0, insets.getTop(), 0, insets.getBottom());
        focusArea.setHighlightPadding(0, insets.getTop(), 0, insets.getBottom());
        requireViewById(R.id.list)
                .setPadding(0, insets.getTop(), 0, insets.getBottom());
        requireViewById(android.R.id.content)
                .setPadding(insets.getLeft(), 0, insets.getRight(), 0);
    }
}
