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

package com.android.car.dialer.ui.activecall;

import android.content.Context;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import com.android.car.dialer.R;
import com.android.car.dialer.ui.view.ContactAvatarOutputlineProvider;
import com.android.car.telephony.common.CallDetail;
import com.android.car.telephony.common.TelecomUtils;

/**
 * View holder for a user profile of a conference
 */
public class ConferenceProfileViewHolder extends RecyclerView.ViewHolder {

    private ImageView mAvatar;
    private TextView mTitle;
    private TextView mNumber;
    private Context mContext;

    ConferenceProfileViewHolder(View v) {
        super(v);

        mAvatar = v.findViewById(R.id.user_profile_avatar);
        mAvatar.setOutlineProvider(ContactAvatarOutputlineProvider.get());
        mTitle = v.findViewById(R.id.user_profile_title);
        mNumber = v.findViewById(R.id.user_profile_phone_number);
        mContext = v.getContext();
    }

    /**
     * Binds call details to the profile views
     */
    public void bind(CallDetail callDetail) {
        String number = callDetail.getNumber();
        TelecomUtils.getPhoneNumberInfo(mContext, number)
                .thenAcceptAsync((info) -> {
                    if (mContext == null) {
                        return;
                    }

                    mAvatar.setImageDrawable(TelecomUtils.createLetterTile(mContext, null, null));
                    mTitle.setText(info.getDisplayName());

                    String phoneNumberLabel = info.getTypeLabel();
                    if (!phoneNumberLabel.isEmpty()) {
                        phoneNumberLabel += " ";
                    }
                    phoneNumberLabel += TelecomUtils.getFormattedNumber(mContext, number);
                    if (!TextUtils.isEmpty(phoneNumberLabel)
                            && !phoneNumberLabel.equals(info.getDisplayName())) {
                        mNumber.setText(phoneNumberLabel);
                    } else {
                        mNumber.setText(mContext.getString(R.string.unknown));
                    }

                    TelecomUtils.setContactBitmapAsync(mContext, mAvatar,
                            info.getAvatarUri(), info.getInitials(), info.getDisplayName());

                }, mContext.getMainExecutor());
    }
}
