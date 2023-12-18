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
package com.chassis.car.ui.sharedlibrary.toolbar;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.Switch;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.MenuItemOEMV1;

import com.chassis.car.ui.sharedlibrary.R;
import com.chassis.car.ui.sharedlibrary.uxr.DrawableStateView;

class MenuItemView extends FrameLayout {
    private static final int[] RESTRICTED_STATE = new int[] {R.attr.state_ux_restricted};
    private static final int[] ENABLED_STATE = new int[] {android.R.attr.state_enabled};

    private final MenuItemOEMV1 mMenuItem;

    private final View mIconContainer;
    private final ImageView mIconView;
    private final Switch mSwitch;
    private final TextView mTextView;
    private final TextView mTextWithIconView;

    private final int mMenuItemIconSize;

    MenuItemView(
            @NonNull Context context,
            MenuItemOEMV1 menuItem) {
        super(context);
        mMenuItemIconSize = getResources().getDimensionPixelSize(R.dimen.primary_icon_size);

        setVisibility(View.GONE);

        LayoutInflater.from(getContext()).inflate(menuItem.isPrimary()
                ? R.layout.toolbar_menu_item_primary
                : R.layout.toolbar_menu_item, this, true);

        mIconContainer = requireViewById(R.id.car_ui_toolbar_menu_item_icon_container);
        mIconView = requireViewById(R.id.car_ui_toolbar_menu_item_icon);
        mSwitch = requireViewById(R.id.car_ui_toolbar_menu_item_switch);
        mTextView = requireViewById(R.id.car_ui_toolbar_menu_item_text);
        mTextWithIconView = requireViewById(R.id.car_ui_toolbar_menu_item_text_with_icon);

        mMenuItem = menuItem;
        mMenuItem.setUpdateListener(this::onMenuItemUpdated);
        onMenuItemUpdated(mMenuItem);
    }

    public void onMenuItemUpdated(MenuItemOEMV1 menuItem) {
        setId(mMenuItem.getId());

        boolean hasIcon = mMenuItem.getIcon() != null;
        boolean hasText = !TextUtils.isEmpty(mMenuItem.getTitle());
        boolean textAndIcon = mMenuItem.isShowingIconAndTitle();
        boolean checkable = mMenuItem.isCheckable();

        if (!mMenuItem.isVisible() || (!checkable && !hasIcon && !hasText)) {
            setVisibility(View.GONE);
            return;
        }
        setVisibility(View.VISIBLE);
        setContentDescription(mMenuItem.getTitle());

        View clickTarget;
        if (checkable) {
            mSwitch.setChecked(mMenuItem.isChecked());
            clickTarget = mSwitch;
        } else if (hasText && hasIcon && textAndIcon) {
            mMenuItem.getIcon().setBounds(0, 0, mMenuItemIconSize, mMenuItemIconSize);
            mTextWithIconView.setCompoundDrawables(mMenuItem.getIcon(), null, null, null);
            mTextWithIconView.setText(mMenuItem.getTitle());
            clickTarget = mTextWithIconView;
        } else if (hasIcon) {
            mIconView.setImageDrawable(mMenuItem.getIcon());
            clickTarget = mIconContainer;
        } else { // hasText will be true
            mTextView.setText(mMenuItem.getTitle());
            clickTarget = mTextView;
        }

        mIconContainer.setVisibility(clickTarget == mIconContainer ? View.VISIBLE : View.GONE);
        mTextView.setVisibility(clickTarget == mTextView ? View.VISIBLE : View.GONE);
        mTextWithIconView.setVisibility(clickTarget == mTextWithIconView
                ? View.VISIBLE : View.GONE);
        mSwitch.setVisibility(clickTarget == mSwitch ? View.VISIBLE : View.GONE);

        if (!mMenuItem.isTinted() && hasIcon) {
            mMenuItem.getIcon().setTintList(null);
        }

        recursiveSetEnabledAndDrawableState(this);
        setActivated(mMenuItem.isActivated());

        clickTarget.setOnClickListener(v -> {
            if (mMenuItem.isEnabled()) {
                mMenuItem.performClick();
            }
        });
    }

    private void recursiveSetEnabledAndDrawableState(View view) {
        int[] drawableState = mMenuItem.isRestricted() ? RESTRICTED_STATE : null;
        int[] drawableStateToRemove = mMenuItem.isEnabled() ? null : ENABLED_STATE;

        if (view instanceof DrawableStateView) {
            ((DrawableStateView) view).setExtraDrawableState(drawableState, drawableStateToRemove);
        }

        if (view instanceof ViewGroup) {
            ViewGroup viewGroup = ((ViewGroup) view);
            for (int i = 0; i < viewGroup.getChildCount(); i++) {
                recursiveSetEnabledAndDrawableState(viewGroup.getChildAt(i));
            }
        }
    }
}
