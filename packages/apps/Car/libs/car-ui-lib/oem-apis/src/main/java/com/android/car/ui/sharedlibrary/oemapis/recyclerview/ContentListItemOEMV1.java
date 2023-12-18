/*
 * Copyright 2021 The Android Open Source Project
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

package com.android.car.ui.sharedlibrary.oemapis.recyclerview;

import android.graphics.drawable.Drawable;
import android.text.SpannableString;

import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;

/**
 * The OEM interface for content list item.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class ContentListItemOEMV1 implements ListItemOEMV1 {
    public enum IconType {
        /**
         * For an icon type of CONTENT, the primary icon is a larger than {@code STANDARD}.
         */
        CONTENT,
        /**
         * For an icon type of STANDARD, the primary icon is the standard size.
         */
        STANDARD,
        /**
         * For an icon type of AVATAR, the primary icon is masked to provide an icon with a modified
         * shape.
         */
        AVATAR
    }

    /**
     * Enum of secondary action types of a list item.
     */
    public enum Action {
        /**
         * For an action value of NONE, no action element is shown for a list item.
         */
        NONE,
        /**
         * For an action value of SWITCH, a switch is shown for the action element of the list
         * item.
         */
        SWITCH,
        /**
         * For an action value of CHECK_BOX, a checkbox is shown for the action element of the list
         * item.
         */
        CHECK_BOX,
        /**
         * For an action value of RADIO_BUTTON, a radio button is shown for the action element of
         * the list item.
         */
        RADIO_BUTTON,
        /**
         * For an action value of ICON, an icon is shown for the action element of the list item.
         */
        ICON,
        /**
         * For an action value CHEVRON, a chevron is shown for the action element of the list item.
         */
        CHEVRON
    }

    private final Action mAction;
    private final SpannableString mTitle;
    private final List<SpannableString> mBody;
    private final Drawable mIcon;
    private final IconType mPrimaryIconType;
    private final Drawable mSupplementalIcon;
    private boolean mIsChecked;
    private final boolean mIsEnabled;
    private final boolean mIsActivated;
    private final boolean mIsActionDividerVisible;
    private final Consumer<ContentListItemOEMV1> mOnClickListener;
    private final Consumer<ContentListItemOEMV1> mOnCheckedChangeListener;
    private final Consumer<ContentListItemOEMV1> mSupplementalIconOnClickListener;

    ContentListItemOEMV1(Builder builder) {
        mAction = builder.mAction;
        mTitle = builder.mTitle;
        mBody = builder.mBody;
        mIcon = builder.mIcon;
        mPrimaryIconType = builder.mPrimaryIconType;
        mSupplementalIcon = builder.mSupplementalIcon;
        mIsChecked = builder.mIsChecked;
        mIsEnabled = builder.mIsEnabled;
        mIsActivated = builder.mIsActivated;
        mIsActionDividerVisible = builder.mIsActionDividerVisible;
        mOnClickListener = builder.mOnClickListener;
        mOnCheckedChangeListener = builder.mOnCheckedChangeListener;
        mSupplementalIconOnClickListener = builder.mSupplementalIconOnClickListener;
    }

    /**
     * Returns the title of the item.
     */
    public SpannableString getTitle() {
        return mTitle;
    }

    /**
     * Returns the body of the item.
     */
    public List<SpannableString> getBody() {
        return mBody;
    }

    /**
     * Returns the icon of the item.
     */
    public Drawable getIcon() {
        return mIcon;
    }

    /**
     * Returns the primary icon type for the item.
     */
    public IconType getPrimaryIconType() {
        return mPrimaryIconType;
    }

    /**
     * Returns {@code true} if the item is activated.
     */
    public boolean isActivated() {
        return mIsActivated;
    }

    /**
     * Returns {@code true} if the item is enabled.
     */
    public boolean isEnabled() {
        return mIsEnabled;
    }

    /**
     * Returns {@code true} if the item is checked. Will always return {@code false} when the action
     * type for the item is {@code Action.NONE}.
     */
    public boolean isChecked() {
        return mIsChecked;
    }

    /**
     * Returns {@code true} if the action divider is visible.
     */
    public boolean isActionDividerVisible() {
        return mIsActionDividerVisible;
    }

    /**
     * Returns the action type for the item.
     */
    public Action getAction() {
        return mAction;
    }

    /**
     * Returns the supplemental icon for the item.
     */
    public Drawable getSupplementalIcon() {
        return mSupplementalIcon;
    }

    /**
     * Returns the click listener registered for the supplemental icon on this item.
     */
    public Consumer<ContentListItemOEMV1> getSupplementalIconOnClickListener() {
        return mSupplementalIconOnClickListener;
    }

    /**
     * Returns the click listener registered for this item.
     */
    public Consumer<ContentListItemOEMV1> getOnClickListener() {
        return mOnClickListener;
    }

    /**
     * Returns the checked change listener registered for this item.
     */
    public Consumer<ContentListItemOEMV1> getOnCheckedChangeListener() {
        return mOnCheckedChangeListener;
    }

    /**
     * Sets the checked state of the item.
     *
     * @param checked the checked state for the item.
     */
    public void setChecked(boolean checked) {
        if (checked == mIsChecked) {
            return;
        }

        // Checked state can only be set when action type is checkbox, radio button or switch.
        if (mAction == Action.CHECK_BOX || mAction == Action.SWITCH
                || mAction == Action.RADIO_BUTTON) {
            mIsChecked = checked;

            if (mOnCheckedChangeListener != null) {
                mOnCheckedChangeListener.accept(this);
            }
        }
    }

    /**
     * A builder of {@link ContentListItemOEMV1}.
     */
    public static final class Builder {
        private final Action mAction;
        private SpannableString mTitle;
        private List<SpannableString> mBody;
        private Drawable mIcon;
        private IconType mPrimaryIconType = IconType.STANDARD;
        private Drawable mSupplementalIcon;
        private boolean mIsChecked = false;
        private boolean mIsEnabled = true;
        private boolean mIsActivated = false;
        private boolean mIsActionDividerVisible;
        private Consumer<ContentListItemOEMV1> mOnClickListener;
        private Consumer<ContentListItemOEMV1> mOnCheckedChangeListener;
        private Consumer<ContentListItemOEMV1> mSupplementalIconOnClickListener;

        /**
         * Returns a new instance of a {@link Builder}.
         */
        public Builder(Action action) {
            mAction = action;
        }

        /**
         * Sets the title of the item.
         *
         * @param text text to display as title
         */
        public Builder setTitle(SpannableString text) {
            mTitle = text;
            return this;
        }

        /**
         * Sets the body of the item.
         *
         * @param text text to display as body text.
         */
        public Builder setBody(SpannableString text) {
            mBody = Collections.singletonList(text);
            return this;
        }

        /**
         * Sets the body of the item.
         *
         * @param textList list of text to display as body text. Each {@link SpannableString} in the
         *                list will be rendered on a new line, separated by a line break.
         */
        public Builder setBody(List<SpannableString> textList) {
            mBody = textList;
            return this;
        }

        /**
         * Sets the icon of the item.
         *
         * @param icon the icon to display.
         * @param type the icon type for the item.
         */
        public Builder setIcon(Drawable icon, IconType type) {
            mIcon = icon;
            mPrimaryIconType = type;
            return this;
        }

        /**
         * Sets the activated state of the item.
         *
         * @param activated the activated state for the item.
         */
        public Builder setActivated(boolean activated) {
            mIsActivated = activated;
            return this;
        }

        /**
         * Sets the enabled state of the item.
         *
         * @param enabled the enabled state for the item.
         */
        public Builder setEnabled(boolean enabled) {
            mIsEnabled = enabled;
            return this;
        }

        /**
         * Sets the checked state of the item.
         *
         * @param checked the checked state for the item.
         */
        public Builder setChecked(boolean checked) {
            mIsChecked = checked;
            return this;
        }

        /**
         * Sets the visibility of the action divider.
         *
         * @param visible visibility of the action divider.
         */
        public Builder setActionDividerVisible(boolean visible) {
            mIsActionDividerVisible = visible;
            return this;
        }

        /**
         * Sets supplemental icon to be displayed in a list item.
         *
         * @param icon the Drawable to set as the icon, or null to clear the content.
         */
        public Builder setSupplementalIcon(Drawable icon) {
            return setSupplementalIcon(icon, null);
        }

        /**
         * Sets supplemental icon to be displayed in a list item.
         *
         * @param icon     the Drawable to set as the icon, or null to clear the content.
         * @param listener the callback that is invoked when the icon is clicked.
         */
        public Builder setSupplementalIcon(Drawable icon, Consumer<ContentListItemOEMV1> listener) {
            if (mAction != Action.ICON) {
                throw new IllegalStateException(
                        "Cannot set supplemental icon on list item that does not have an action of "
                                + "type ICON");
            }

            mSupplementalIcon = icon;
            mSupplementalIconOnClickListener = listener;
            return this;
        }

        /**
         * Registers a callback to be invoked when the item is clicked.
         *
         * @param listener callback to be invoked when item is clicked.
         */
        public void setOnItemClickedListener(Consumer<ContentListItemOEMV1> listener) {
            mOnClickListener = listener;
        }

        /**
         * Registers a callback to be invoked when the checked state of list item changes.
         *
         * <p>Checked state changes can take place when the action type is {@code Action.SWITCH} or
         * {@code Action.CHECK_BOX}.
         *
         * @param listener callback to be invoked when the checked state shown in the UI changes.
         */
        public Builder setOnCheckedChangeListener(Consumer<ContentListItemOEMV1> listener) {
            mOnCheckedChangeListener = listener;
            return this;
        }

        /**
         * Returns a {@link ContentListItemOEMV1} for this {@link Builder}.
         */
        public ContentListItemOEMV1 build() {
            return new ContentListItemOEMV1(this);
        }
    }
}
