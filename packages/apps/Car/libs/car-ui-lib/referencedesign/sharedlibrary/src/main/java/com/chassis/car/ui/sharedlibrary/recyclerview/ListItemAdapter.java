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
package com.chassis.car.ui.sharedlibrary.recyclerview;

import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterDataObserverOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ContentListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.HeaderListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ListItemOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ViewHolderOEMV1;

import com.chassis.car.ui.sharedlibrary.R;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Adapter for {@link RecyclerViewOEMV1} to display {@link ContentListItemOEMV1} and {@link
 * HeaderListItemOEMV1}.
 */
public class ListItemAdapter extends RecyclerView.Adapter<ListItemAdapter.BaseViewHolder> implements
        AdapterOEMV1<ListItemAdapter.BaseViewHolder> {
    static final int VIEW_TYPE_LIST_ITEM = 1;
    static final int VIEW_TYPE_LIST_HEADER = 2;

    private final List<? extends ListItemOEMV1> mItems;
    @NonNull
    private final List<AdapterDataObserverOEMV1> mAdapterDataObservers = new ArrayList<>();
    private final RecyclerView.AdapterDataObserver mAdapterDataObserver =
            new RecyclerView.AdapterDataObserver() {
                @Override
                public void onChanged() {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onChanged();
                    }
                }

                @Override
                public void onItemRangeChanged(int positionStart, int itemCount) {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onItemRangeChanged(positionStart, itemCount);
                    }
                }

                @Override
                public void onItemRangeChanged(int positionStart, int itemCount,
                        @Nullable Object payload) {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onItemRangeChanged(positionStart, itemCount, payload);
                    }
                }

                @Override
                public void onItemRangeInserted(int positionStart, int itemCount) {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onItemRangeInserted(positionStart, itemCount);
                    }
                }

                @Override
                public void onItemRangeRemoved(int positionStart, int itemCount) {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onItemRangeRemoved(positionStart, itemCount);
                    }
                }

                @Override
                public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        for (int i = 0; i < itemCount; i++) {
                            observer.onItemMoved(fromPosition + i, toPosition + i);
                        }
                    }
                }

                @Override
                public void onStateRestorationPolicyChanged() {
                    for (AdapterDataObserverOEMV1 observer : mAdapterDataObservers) {
                        observer.onStateRestorationPolicyChanged();
                    }
                }
            };

    public ListItemAdapter(List<? extends ListItemOEMV1> items) {
        mItems = items;
    }

    @NonNull
    @Override
    public ListItemAdapter.BaseViewHolder onCreateViewHolder(
            @NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());

        switch (viewType) {
            case VIEW_TYPE_LIST_ITEM:
                return new ListItemViewHolder(
                        inflater.inflate(R.layout.list_item, parent, false));
            case VIEW_TYPE_LIST_HEADER:
                return new HeaderViewHolder(
                        inflater.inflate(R.layout.header_list_item, parent, false));
            default:
                throw new IllegalStateException("Unknown item type.");
        }
    }

    @Override
    public int getItemViewType(int position) {
        if (mItems.get(position) instanceof ContentListItemOEMV1) {
            return VIEW_TYPE_LIST_ITEM;
        } else if (mItems.get(position) instanceof HeaderListItemOEMV1) {
            return VIEW_TYPE_LIST_HEADER;
        }

        throw new IllegalStateException("Unknown view type.");
    }

    @Override
    public int getStateRestorationPolicyInt() {
        switch (getStateRestorationPolicy()) {
            case PREVENT:
                return AdapterOEMV1.PREVENT;
            case PREVENT_WHEN_EMPTY:
                return AdapterOEMV1.PREVENT_WHEN_EMPTY;
            case ALLOW:
            default:
                return AdapterOEMV1.ALLOW;
        }
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerViewOEMV1 recyclerView) {
        //  Do nothing. This method will never be invoked.
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerViewOEMV1 recyclerView) {
        //  Do nothing. This method will never be invoked.
    }

    @Override
    public void onAttachedToRecyclerView(@NonNull RecyclerView recyclerView) {
        registerAdapterDataObserver(mAdapterDataObserver);
    }

    @Override
    public void onDetachedFromRecyclerView(@NonNull RecyclerView recyclerView) {
        unregisterAdapterDataObserver(mAdapterDataObserver);
    }

    @Override
    public void registerAdapterDataObserver(AdapterDataObserverOEMV1 observer) {
        if (observer == null) {
            return;
        }
        mAdapterDataObservers.add(observer);
    }

    @Override
    public void unregisterAdapterDataObserver(AdapterDataObserverOEMV1 observer) {
        if (observer == null) {
            return;
        }
        mAdapterDataObservers.remove(observer);
    }

    @Override
    public void setRecyclerView(View recyclerview) {
        //  Do nothing. This method will never be invoked.
    }

    @Override
    public void onBindViewHolder(@NonNull ListItemAdapter.BaseViewHolder holder, int position) {
        switch (holder.getItemViewType()) {
            case VIEW_TYPE_LIST_ITEM:
                if (!(holder instanceof ListItemViewHolder)) {
                    throw new IllegalStateException("Incorrect view holder type for list item.");
                }

                ListItemOEMV1 item = mItems.get(position);
                if (!(item instanceof ContentListItemOEMV1)) {
                    throw new IllegalStateException(
                            "Expected item to be bound to viewHolder to be instance of "
                                    + "CarUiContentListItem.");
                }

                ((ListItemViewHolder) holder).bind((ContentListItemOEMV1) item);
                break;
            case VIEW_TYPE_LIST_HEADER:
                if (!(holder instanceof HeaderViewHolder)) {
                    throw new IllegalStateException("Incorrect view holder type for list item.");
                }

                ListItemOEMV1 header = mItems.get(position);
                if (!(header instanceof HeaderListItemOEMV1)) {
                    throw new IllegalStateException(
                            "Expected item to be bound to viewHolder to be instance of "
                                    + "CarUiHeaderListItem.");
                }

                ((HeaderViewHolder) holder).bind((HeaderListItemOEMV1) header);
                break;
            default:
                throw new IllegalStateException("Unknown item view type.");
        }
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    abstract static class BaseViewHolder extends RecyclerView.ViewHolder implements
            ViewHolderOEMV1 {
        BaseViewHolder(@NonNull View itemView) {
            super(itemView);
        }
    }

    /**
     * Holds views of {@link ContentListItemOEMV1}.
     */
    static class ListItemViewHolder extends ListItemAdapter.BaseViewHolder {
        final TextView mTitle;
        final TextView mBody;
        final ImageView mIcon;
        final ImageView mContentIcon;
        final ImageView mAvatarIcon;
        final ViewGroup mIconContainer;
        final ViewGroup mActionContainer;
        final View mActionDivider;
        final Switch mSwitch;
        final CheckBox mCheckBox;
        final RadioButton mRadioButton;
        final ImageView mSupplementalIcon;
        final View mTouchInterceptor;
        final View mReducedTouchInterceptor;
        final View mActionContainerTouchInterceptor;

        ListItemViewHolder(@NonNull View itemView) {
            super(itemView);
            mTitle = itemView.requireViewById(R.id.list_item_title);
            mBody = itemView.requireViewById(R.id.list_item_body);
            mIcon = itemView.requireViewById(R.id.list_item_icon);
            mContentIcon = itemView.requireViewById(R.id.list_item_content_icon);
            mAvatarIcon = itemView.requireViewById(R.id.list_item_avatar_icon);
            mIconContainer = itemView.requireViewById(R.id.list_item_icon_container);
            mActionContainer = itemView.requireViewById(R.id.list_item_action_container);
            mActionDivider = itemView.requireViewById(R.id.list_item_action_divider);
            mSwitch = itemView.requireViewById(R.id.list_item_switch_widget);
            mCheckBox = itemView.requireViewById(R.id.list_item_checkbox_widget);
            mRadioButton = itemView.requireViewById(R.id.list_item_radio_button_widget);
            mSupplementalIcon = itemView.requireViewById(R.id.list_item_supplemental_icon);
            mReducedTouchInterceptor = itemView.requireViewById(
                    R.id.list_item_reduced_touch_interceptor);
            mTouchInterceptor = itemView.requireViewById(
                    R.id.list_item_touch_interceptor);
            mActionContainerTouchInterceptor = itemView.requireViewById(
                    R.id.list_item_action_container_touch_interceptor);
        }

        void bind(@NonNull ContentListItemOEMV1 item) {
            if (item.getTitle() != null) {
                mTitle.setText(item.getTitle());
                mTitle.setVisibility(View.VISIBLE);
            } else {
                mTitle.setVisibility(View.GONE);
            }

            if (item.getBody() != null) {
                mBody.setText(TextUtils.join("\n", item.getBody()));
                mBody.setVisibility(View.VISIBLE);
            } else {
                mBody.setVisibility(View.GONE);
            }

            mIcon.setVisibility(View.GONE);
            mContentIcon.setVisibility(View.GONE);
            mAvatarIcon.setVisibility(View.GONE);

            Drawable icon = item.getIcon();
            if (icon != null) {
                mIconContainer.setVisibility(View.VISIBLE);

                switch (item.getPrimaryIconType()) {
                    case CONTENT:
                        mContentIcon.setVisibility(View.VISIBLE);
                        mContentIcon.setImageDrawable(icon);
                        break;
                    case STANDARD:
                        mIcon.setVisibility(View.VISIBLE);
                        mIcon.setImageDrawable(icon);
                        break;
                    case AVATAR:
                        mAvatarIcon.setVisibility(View.VISIBLE);
                        mAvatarIcon.setImageDrawable(icon);
                        mAvatarIcon.setClipToOutline(true);
                        break;
                }
            } else {
                mIconContainer.setVisibility(View.GONE);
            }

            mActionDivider.setVisibility(
                    item.isActionDividerVisible() ? View.VISIBLE : View.GONE);
            mSwitch.setVisibility(View.GONE);
            mCheckBox.setVisibility(View.GONE);
            mRadioButton.setVisibility(View.GONE);
            mSupplementalIcon.setVisibility(View.GONE);

            Consumer<ContentListItemOEMV1> itemOnClickListener = item.getOnClickListener();

            switch (item.getAction()) {
                case NONE:
                    mActionContainer.setVisibility(View.GONE);

                    // Display ripple effects across entire item when clicked by using full-sized
                    // touch interceptor.
                    mTouchInterceptor.setVisibility(View.VISIBLE);
                    mTouchInterceptor.setOnClickListener(v -> {
                        if (itemOnClickListener != null) {
                            itemOnClickListener.accept(item);
                        }
                    });
                    mTouchInterceptor.setClickable(itemOnClickListener != null);
                    mReducedTouchInterceptor.setVisibility(View.GONE);
                    mActionContainerTouchInterceptor.setVisibility(View.GONE);
                    break;
                case SWITCH:
                    bindCompoundButton(item, mSwitch, itemOnClickListener);
                    break;
                case CHECK_BOX:
                    bindCompoundButton(item, mCheckBox, itemOnClickListener);
                    break;
                case RADIO_BUTTON:
                    bindCompoundButton(item, mRadioButton, itemOnClickListener);
                    break;
                case CHEVRON:
                    mSupplementalIcon.setVisibility(View.VISIBLE);
                    mSupplementalIcon.setImageDrawable(itemView.getContext().getDrawable(
                            R.drawable.icon_chevron));
                    mActionContainer.setVisibility(View.VISIBLE);
                    mTouchInterceptor.setVisibility(View.VISIBLE);
                    mTouchInterceptor.setOnClickListener(v -> {
                        if (itemOnClickListener != null) {
                            itemOnClickListener.accept(item);
                        }
                    });
                    mTouchInterceptor.setClickable(itemOnClickListener != null);
                    mReducedTouchInterceptor.setVisibility(View.GONE);
                    mActionContainerTouchInterceptor.setVisibility(View.GONE);
                    break;
                case ICON:
                    mSupplementalIcon.setVisibility(View.VISIBLE);
                    mSupplementalIcon.setImageDrawable(item.getSupplementalIcon());

                    mActionContainer.setVisibility(View.VISIBLE);

                    // If the icon has a click listener, use a reduced touch interceptor to create
                    // two distinct touch area; the action container and the remainder of the list
                    // item. Each touch area will have its own ripple effect. If the icon has no
                    // click listener, it shouldn't be clickable.
                    if (item.getSupplementalIconOnClickListener() == null) {
                        mTouchInterceptor.setVisibility(View.VISIBLE);
                        mTouchInterceptor.setOnClickListener(v -> {
                            if (itemOnClickListener != null) {
                                itemOnClickListener.accept(item);
                            }
                        });
                        mTouchInterceptor.setClickable(itemOnClickListener != null);
                        mReducedTouchInterceptor.setVisibility(View.GONE);
                        mActionContainerTouchInterceptor.setVisibility(View.GONE);
                    } else {
                        mReducedTouchInterceptor.setVisibility(View.VISIBLE);
                        mReducedTouchInterceptor.setOnClickListener(v -> {
                            if (itemOnClickListener != null) {
                                itemOnClickListener.accept(item);
                            }
                        });
                        mReducedTouchInterceptor.setClickable(itemOnClickListener != null);
                        mActionContainerTouchInterceptor.setVisibility(View.VISIBLE);
                        mActionContainerTouchInterceptor.setOnClickListener(
                                (container) -> {
                                    if (item.getSupplementalIconOnClickListener() != null) {
                                        item.getSupplementalIconOnClickListener().accept(item);
                                    }
                                });
                        mActionContainerTouchInterceptor.setClickable(
                                item.getSupplementalIconOnClickListener() != null);
                        mTouchInterceptor.setVisibility(View.GONE);
                    }
                    break;
                default:
                    throw new IllegalStateException("Unknown secondary action type.");
            }

            itemView.setActivated(item.isActivated());
            setEnabled(itemView, item.isEnabled());
        }

        void setEnabled(View view, boolean enabled) {
            view.setEnabled(enabled);
            if (view instanceof ViewGroup) {
                ViewGroup group = (ViewGroup) view;

                for (int i = 0; i < group.getChildCount(); i++) {
                    setEnabled(group.getChildAt(i), enabled);
                }
            }
        }

        void bindCompoundButton(@NonNull ContentListItemOEMV1 item,
                @NonNull CompoundButton compoundButton,
                @Nullable Consumer<ContentListItemOEMV1> itemOnClickListener) {
            compoundButton.setVisibility(View.VISIBLE);
            compoundButton.setOnCheckedChangeListener(null);
            compoundButton.setChecked(item.isChecked());
            compoundButton.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> item.setChecked(isChecked));

            // Clicks anywhere on the item should toggle the checkbox state. Use full touch
            // interceptor.
            mTouchInterceptor.setVisibility(View.VISIBLE);
            mTouchInterceptor.setOnClickListener(v -> {
                compoundButton.toggle();
                if (itemOnClickListener != null) {
                    itemOnClickListener.accept(item);
                }
            });
            // Compound button list items should always be clickable
            mTouchInterceptor.setClickable(true);
            mReducedTouchInterceptor.setVisibility(View.GONE);
            mActionContainerTouchInterceptor.setVisibility(View.GONE);

            mActionContainer.setVisibility(View.VISIBLE);
            mActionContainer.setClickable(false);
        }

        @Override
        public View getItemView() {
            return super.itemView;
        }
    }

    /**
     * Holds views of {@link HeaderListItemOEMV1}.
     */
    static class HeaderViewHolder extends ListItemAdapter.BaseViewHolder {
        private final TextView mTitle;
        private final TextView mBody;

        HeaderViewHolder(@NonNull View itemView) {
            super(itemView);
            mTitle = itemView.requireViewById(R.id.list_item_title);
            mBody = itemView.requireViewById(R.id.list_item_body);
        }

        private void bind(@NonNull HeaderListItemOEMV1 item) {
            mTitle.setText(item.getTitle());

            CharSequence body = item.getBody();
            if (!TextUtils.isEmpty(body)) {
                mBody.setText(body);
            } else {
                mBody.setVisibility(View.GONE);
            }
        }

        @Override
        public View getItemView() {
            return super.itemView;
        }
    }
}
