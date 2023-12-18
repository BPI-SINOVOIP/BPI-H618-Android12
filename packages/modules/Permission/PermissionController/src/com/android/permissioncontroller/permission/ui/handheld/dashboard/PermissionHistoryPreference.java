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

package com.android.permissioncontroller.permission.ui.handheld.dashboard;

import static com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_DETAILS_INTERACTION;
import static com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_DETAILS_INTERACTION__ACTION__INFO_ICON_CLICKED;
import static com.android.permissioncontroller.PermissionControllerStatsLog.write;

import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;
import android.os.UserHandle;
import android.text.format.DateFormat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.permissioncontroller.R;
import com.android.permissioncontroller.permission.utils.KotlinUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * Preference for the permission history page
 */
public class PermissionHistoryPreference extends Preference {

    private static final String LOG_TAG = "PermissionHistoryPreference";

    private final Context mContext;
    private final UserHandle mUserHandle;
    private final String mPackageName;
    private final String mPermissionGroup;
    private final String mAccessTime;
    private final Drawable mAppIcon;
    private final String mTitle;
    private final float mDialogWidthScalar;
    private final float mDialogHeightScalar;
    private final List<Long> mAccessTimeList;
    private final ArrayList<String> mAttributionTags;
    private final boolean mIsLastUsage;
    private final Intent mIntent;

    private final long mSessionId;

    private Drawable mWidgetIcon;

    public PermissionHistoryPreference(@NonNull Context context,
            @NonNull UserHandle userHandle, @NonNull String pkgName,
            @NonNull Drawable appIcon,
            @NonNull String preferenceTitle,
            @NonNull String permissionGroup, @NonNull String accessTime,
            @Nullable CharSequence summaryText, @NonNull List<Long> accessTimeList,
            @NonNull ArrayList<String> attributionTags, boolean isLastUsage, long sessionId) {
        super(context);
        mContext = context;
        mUserHandle = userHandle;
        mPackageName = pkgName;
        mPermissionGroup = permissionGroup;
        mAccessTime = accessTime;
        mAppIcon = appIcon;
        mTitle = preferenceTitle;
        mWidgetIcon = null;
        mAccessTimeList = accessTimeList;
        mAttributionTags = attributionTags;
        mIsLastUsage = isLastUsage;
        TypedValue outValue = new TypedValue();
        mContext.getResources().getValue(R.dimen.permission_access_time_dialog_width_scalar,
                outValue, true);
        mDialogWidthScalar = outValue.getFloat();
        mContext.getResources().getValue(R.dimen.permission_access_time_dialog_height_scalar,
                outValue, true);
        mDialogHeightScalar = outValue.getFloat();
        mSessionId = sessionId;

        setTitle(mTitle);
        if (summaryText != null) {
            setSummary(summaryText);
        }

        mIntent = getViewPermissionUsageForPeriodIntent();
        if (mIntent != null) {
            mWidgetIcon = mContext.getDrawable(R.drawable.ic_info_outline);
            setWidgetLayoutResource(R.layout.image_view_with_divider);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        ViewGroup widgetFrame = (ViewGroup) holder.findViewById(android.R.id.widget_frame);
        LinearLayout widgetFrameParent = (LinearLayout) widgetFrame.getParent();

        View iconFrame = holder.findViewById(R.id.icon_frame);
        widgetFrameParent.removeView(iconFrame);

        ViewGroup widget = (ViewGroup) holder.findViewById(R.id.permission_history_layout);
        if (widget == null) {
            LayoutInflater inflater = mContext.getSystemService(LayoutInflater.class);
            widget = (ViewGroup) inflater.inflate(R.layout.permission_history_widget,
                    widgetFrameParent, false);

            widgetFrameParent.addView(widget, 0);
        }

        widgetFrameParent.setGravity(Gravity.TOP);

        TextView permissionHistoryTime = widget.findViewById(R.id.permission_history_time);
        permissionHistoryTime.setText(mAccessTime);

        ImageView permissionIcon = widget.findViewById(R.id.permission_history_icon);
        permissionIcon.setImageDrawable(mAppIcon);

        ImageView widgetView = widgetFrame.findViewById(R.id.icon);
        setInfoIcon(widgetView);

        View dashLine = widget.findViewById(R.id.permission_history_dash_line);
        dashLine.setVisibility(mIsLastUsage ? View.GONE : View.VISIBLE);

        setOnPreferenceClickListener((preference) -> {
            Intent intent = new Intent(Intent.ACTION_MANAGE_APP_PERMISSIONS);
            intent.putExtra(Intent.EXTRA_USER, mUserHandle);
            intent.putExtra(Intent.EXTRA_PACKAGE_NAME, mPackageName);

            mContext.startActivity(intent);
            return true;
        });
    }

    private void setHistoryIcon(ImageView widgetView) {
        widgetView.setImageDrawable(mWidgetIcon);
        widgetView.setOnClickListener(v -> {
            Dialog dialog = new Dialog(mContext);
            dialog.setContentView(R.layout.access_time_list_dialog);

            ImageView iconView = dialog.findViewById(R.id.icon);
            iconView.setImageDrawable(mAppIcon);

            TextView titleView = dialog.findViewById(R.id.title);
            titleView.setText(mTitle);

            TextView subtitleView = dialog.findViewById(R.id.subtitle);
            subtitleView.setText(mContext.getResources().getString(
                    R.string.permission_usage_access_dialog_subtitle,
                    KotlinUtils.INSTANCE.getPermGroupLabel(mContext, mPermissionGroup)));

            RecyclerView recyclerView = dialog.findViewById(R.id.access_time_list);
            recyclerView.setLayoutManager(new LinearLayoutManager(mContext));
            AccessTimeListAdapter adapter = new AccessTimeListAdapter(mAccessTimeList);
            recyclerView.setAdapter(adapter);

            if (mIntent != null) {
                TextView learnMoreView = dialog.findViewById(R.id.learn_more);
                learnMoreView.setVisibility(View.VISIBLE);
                learnMoreView.setOnClickListener(v1 -> {
                    try {
                        mContext.startActivity(mIntent);
                    } catch (ActivityNotFoundException e) {
                        Log.e(LOG_TAG, "No activity found for viewing permission usage.");
                    }
                });
            }

            dialog.show();

            // Resize the dialog.
            // Have to do this since the default dialog can't be set width and height.
            DisplayMetrics displayMetrics = new DisplayMetrics();
            mContext.getDisplay().getMetrics(displayMetrics);
            int displayWidth = displayMetrics.widthPixels;
            int displayHeight = displayMetrics.heightPixels;
            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
            layoutParams.copyFrom(dialog.getWindow().getAttributes());
            int dialogWindowWidth = (int) (displayWidth * mDialogWidthScalar);
            int dialogWindowHeight = (int) (displayHeight * mDialogHeightScalar);
            layoutParams.width = dialogWindowWidth;
            layoutParams.height = dialogWindowHeight;
            dialog.getWindow().setAttributes(layoutParams);
        });
    }

    private void setInfoIcon(ImageView widgetView) {
        if (mIntent != null) {
            widgetView.setImageDrawable(mWidgetIcon);
            widgetView.setOnClickListener(v -> {
                write(PERMISSION_DETAILS_INTERACTION,
                        mSessionId,
                        mPermissionGroup,
                        mPackageName,
                        PERMISSION_DETAILS_INTERACTION__ACTION__INFO_ICON_CLICKED);
                try {
                    mContext.startActivity(mIntent);
                } catch (ActivityNotFoundException e) {
                    Log.e(LOG_TAG, "No activity found for viewing permission usage.");
                }
            });
        }
    }

    /**
     * Get a {@link Intent#ACTION_VIEW_PERMISSION_USAGE_FOR_PERIOD} intent, or null if the intent
     * can't be handled.
     */
    private Intent getViewPermissionUsageForPeriodIntent() {
        Intent viewUsageIntent = new Intent();
        viewUsageIntent.setAction(Intent.ACTION_VIEW_PERMISSION_USAGE_FOR_PERIOD);
        viewUsageIntent.setPackage(mPackageName);
        viewUsageIntent.putExtra(Intent.EXTRA_PERMISSION_GROUP_NAME, mPermissionGroup);
        viewUsageIntent.putExtra(Intent.EXTRA_ATTRIBUTION_TAGS,
                mAttributionTags.toArray(new String[0]));
        viewUsageIntent.putExtra(Intent.EXTRA_START_TIME,
                mAccessTimeList.get(mAccessTimeList.size() - 1));
        viewUsageIntent.putExtra(Intent.EXTRA_END_TIME, mAccessTimeList.get(0));

        PackageManager packageManager = mContext.getPackageManager();
        ResolveInfo resolveInfo = packageManager.resolveActivity(viewUsageIntent,
                PackageManager.MATCH_INSTANT);
        if (resolveInfo != null && resolveInfo.activityInfo != null && Objects.equals(
                resolveInfo.activityInfo.permission,
                android.Manifest.permission.START_VIEW_PERMISSION_USAGE)) {
            return viewUsageIntent;
        }
        return null;
    }

    /**
     * Adapter for access time list dialog RecyclerView
     */
    public class AccessTimeListAdapter extends
            RecyclerView.Adapter<AccessTimeListAdapter.ViewHolder> {

        /**
         * ViewHolder for the AccessTimeListAdapter
         */
        public class ViewHolder extends RecyclerView.ViewHolder {
            public TextView accessTimeTextView;

            public ViewHolder(View itemView) {
                super(itemView);

                accessTimeTextView = itemView.findViewById(R.id.access_time);
            }
        }

        private final List<Long> mAccessTimeList;

        public AccessTimeListAdapter(List<Long> accessTimeList) {
            mAccessTimeList = accessTimeList;
        }

        @Override
        public AccessTimeListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            View itemView = inflater.inflate(R.layout.access_time_item, parent, false);

            return new ViewHolder(itemView);
        }

        @Override
        public void onBindViewHolder(AccessTimeListAdapter.ViewHolder holder, int position) {
            Long accessTime = mAccessTimeList.get(position);

            TextView textView = holder.accessTimeTextView;
            textView.setText(DateFormat.getTimeFormat(mContext).format(accessTime));
        }

        @Override
        public int getItemCount() {
            return mAccessTimeList.size();
        }
    }
}