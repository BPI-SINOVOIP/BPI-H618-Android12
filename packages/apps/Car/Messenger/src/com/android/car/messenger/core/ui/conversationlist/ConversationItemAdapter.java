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

import android.content.Context;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;

import com.android.car.messenger.R;
import com.android.car.messenger.common.Conversation;

import java.util.ArrayList;
import java.util.List;

/** Adapter for conversation log list. */
public class ConversationItemAdapter extends RecyclerView.Adapter<ConversationItemViewHolder> {
    /** Item Click listener for when an item on the UI is tapped */
    public interface OnConversationItemClickListener {
        /** Callback to start tap to read voice interaction for conversation item */
        void onConversationItemClicked(@NonNull Conversation conversation);
        /** Callback to start tap to reply voice interaction for conversation item */
        void onReplyIconClicked(@NonNull Conversation conversation);
    }

    @NonNull private final List<UIConversationItem> mUIConversationItems = new ArrayList<>();
    @NonNull private final OnConversationItemClickListener mOnConversationItemClickListener;

    public ConversationItemAdapter(
            @NonNull OnConversationItemClickListener onConversationItemClickListener) {
        mOnConversationItemClickListener = onConversationItemClickListener;
    }

    /** Sets conversation logs. */
    public void setConversationLogItems(@NonNull List<UIConversationItem> uIConversationItems) {
        mUIConversationItems.clear();
        mUIConversationItems.addAll(uIConversationItems);
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public ConversationItemViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        Context context = parent.getContext();
        View rootView =
                LayoutInflater.from(context)
                        .inflate(R.layout.conversation_list_item, parent, false);
        return new ConversationItemViewHolder(rootView, mOnConversationItemClickListener);
    }

    @Override
    public void onBindViewHolder(@NonNull ConversationItemViewHolder holder, int position) {
        holder.bind(mUIConversationItems.get(position));
    }

    @Override
    public void onViewRecycled(@NonNull ConversationItemViewHolder holder) {
        holder.recycle();
    }

    @Override
    public int getItemCount() {
        return mUIConversationItems.size();
    }
}
