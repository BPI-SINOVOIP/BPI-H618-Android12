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
package com.android.car.messenger.core.ui.conversationlist;

import android.app.Activity;
import androidx.lifecycle.ViewModelProvider;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;
import com.android.car.messenger.common.Conversation;
import com.android.car.messenger.core.models.ConnectionStatus;
import com.android.car.messenger.core.models.UserAccount;
import com.android.car.messenger.core.shared.MessageConstants;
import com.android.car.messenger.core.ui.base.MessageListBaseFragment;
import com.android.car.messenger.core.util.L;
import com.android.car.messenger.core.util.VoiceUtil;
import com.android.car.ui.toolbar.MenuItem;

import java.util.ArrayList;

/** Fragment for Message History/Conversation Metadata List */
public class ConversationListFragment extends MessageListBaseFragment
        implements ConversationItemAdapter.OnConversationItemClickListener {
    @NonNull
    private static final String BLUETOOTH_SETTING_ACTION = "android.settings.BLUETOOTH_SETTINGS";

    @NonNull
    private static final String BLUETOOTH_SETTING_CATEGORY = "android.intent.category.DEFAULT";

    @NonNull private static final String KEY_USER_ACCOUNT = "KEY_USER_ACCOUNT";
    @Nullable private ConversationItemAdapter mConversationItemAdapter;
    @Nullable private UserAccount mUserAccount;

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (getArguments() != null) {
            mUserAccount = getArguments().getParcelable(KEY_USER_ACCOUNT);
        }

        // Don't recreate the adapter if we already have one, so that the list items
        // will display immediately upon the view being recreated.
        L.d("In View Created, about to load message data");
        if (mConversationItemAdapter == null) {
            mConversationItemAdapter =
                    new ConversationItemAdapter(/* onConversationItemClickListener= */ this);
        }
        getRecyclerView().setAdapter(mConversationItemAdapter);
        ConversationListViewModel viewModel =
                new ViewModelProvider(this).get(ConversationListViewModel.class);

        viewModel
                .getConversations(mUserAccount)
                .observe(
                        this,
                        conversationLog -> {
                            if (conversationLog.getConnectionStatus()
                                    != ConnectionStatus.CONNECTED) {
                                ConnectionStatus connectionStatus =
                                        conversationLog.getConnectionStatus();
                                if (connectionStatus == ConnectionStatus.DISCONNECTED) {
                                    // we currently only support bluetooth disconnect.
                                    // Future work can add other types of disconnect such as login
                                    handleBluetoothDisconnected();
                                }
                            } else if (conversationLog.isLoading()) {
                                mLoadingFrameLayout.showLoading();
                            } else if (conversationLog.getData() == null
                                    || conversationLog.getData().isEmpty()) {
                                mLoadingFrameLayout.showEmpty(
                                        MessageConstants.INVALID_RES_ID,
                                        R.string.no_new_messages,
                                        MessageConstants.INVALID_RES_ID);
                                setMenuItems();
                            } else {
                                mConversationItemAdapter.setConversationLogItems(
                                        conversationLog.getData());
                                mLoadingFrameLayout.showContent();
                                setMenuItems();
                            }
                        });
    }

    private void handleBluetoothDisconnected() {
        Intent launchIntent = new Intent();
        launchIntent.setAction(BLUETOOTH_SETTING_ACTION);
        launchIntent.addCategory(BLUETOOTH_SETTING_CATEGORY);
        mLoadingFrameLayout.showError(
                MessageConstants.INVALID_RES_ID,
                R.string.bluetooth_disconnected,
                MessageConstants.INVALID_RES_ID,
                R.string.connect_bluetooth_button_text,
                v -> startActivity(launchIntent),
                true);
    }

    private void setMenuItems() {
        Activity activity = getActivity();
        if (activity == null || mUserAccount == null || mToolbar == null) {
            return;
        }
        if (!getResources().getBoolean(R.bool.direct_send_supported)) {
            return;
        }
        MenuItem newMessageButton =
                new MenuItem.Builder(activity)
                        .setIcon(R.drawable.car_ui_icon_edit)
                        .setTinted(true)
                        .setShowIconAndTitle(true)
                        .setTitle(R.string.new_message)
                        .setOnClickListener(
                                item ->
                                        VoiceUtil.voiceRequestGenericCompose(
                                                activity, mUserAccount))
                        .build();
        ArrayList<MenuItem> menuItems = new ArrayList<>();
        menuItems.add(newMessageButton);
        mToolbar.setMenuItems(menuItems);
    }

    @Override
    public void onConversationItemClicked(@NonNull Conversation conversation) {
        if (mUserAccount == null) {
            return;
        }
        VoiceUtil.voiceRequestReadConversation(requireActivity(), mUserAccount, conversation);
    }

    @Override
    public void onReplyIconClicked(@NonNull Conversation conversation) {
        if (mUserAccount == null) {
            return;
        }
        VoiceUtil.voiceRequestReplyConversation(requireActivity(), mUserAccount, conversation);
    }

    /**
     * Get instance of Conversation Log fragment
     *
     * @param userAccount the user device info data will be retrieved for. If null, this fragment
     *     shows a disconnect page
     * @return ConversationLogFragment instance
     */
    public static ConversationListFragment newInstance(@Nullable UserAccount userAccount) {
        Bundle args = new Bundle();
        args.putParcelable(KEY_USER_ACCOUNT, userAccount);
        ConversationListFragment fragment = new ConversationListFragment();
        fragment.setArguments(args);
        return fragment;
    }

    /**
     * Get unique fragment tag for fragment loading data for user device
     *
     * @param userAccount the user device info data will be retrieved for.
     * @return unique fragment tag
     */
    public static String getFragmentTag(@Nullable UserAccount userAccount) {
        int id = userAccount == null ? -1 : userAccount.getId();
        return ConversationListFragment.class.getName() + id;
    }
}
