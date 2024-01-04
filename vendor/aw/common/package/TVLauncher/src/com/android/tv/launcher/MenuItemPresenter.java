/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.launcher;

import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Paint.FontMetricsInt;
import android.graphics.Typeface;
import android.net.Uri;
import androidx.leanback.widget.Presenter;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.tv.launcher.widget.BitmapDownloader;
import com.android.tv.launcher.widget.BitmapDownloader.BitmapCallback;
import com.android.tv.launcher.widget.BitmapWorkerOptions;

public class MenuItemPresenter extends Presenter {

    private static final String TAG = "MenuItemPresenter";

    private static class MenuItemViewHolder extends ViewHolder {
        public final ImageView mIconView;
        public final TextView mTitleView;
        //remove the description view
       // public final TextView mDescriptionView;
        public BitmapCallback mBitmapCallBack;

        MenuItemViewHolder(View v) {
            super(v);

            mIconView = (ImageView) v.findViewById(R.id.icon);
            mTitleView = (TextView) v.findViewById(R.id.title);
           // mDescriptionView = (TextView) v.findViewById(R.id.description);
        }
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {

        LayoutInflater inflater = (LayoutInflater) parent.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View v = inflater.inflate(R.layout.browse_item, parent, false);//每一个图标项的内部layout
        return new MenuItemViewHolder(v);
    }

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, Object item) {
        if (item instanceof MenuItem && viewHolder instanceof MenuItemViewHolder) {
            final MenuItem menuItem = (MenuItem) item;
            MenuItemViewHolder menuItemViewHolder = (MenuItemViewHolder) viewHolder;
            Resources res = menuItemViewHolder.mTitleView.getContext().getResources();
            ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams)
                menuItemViewHolder.mTitleView.getLayoutParams();
            lp.bottomMargin = (int) res.getDimension(R.dimen.browse_item_description_marginBottom);
            menuItemViewHolder.mTitleView.setSingleLine(false);
            menuItemViewHolder.mTitleView.setLines(2);
            menuItemViewHolder.mTitleView.setLayoutParams(lp);
            viewHolder.view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (v != null && menuItem.getIntent() != null) {
                        ((Activity) v.getContext()).startActivity(menuItem.getIntent());
                    }
                }
            });
            prepareImageView(menuItemViewHolder, menuItem.getImageUriGetter().getUri(), menuItem);
        }

    }

    @Override
    public void onUnbindViewHolder(ViewHolder viewHolder) {
        if (viewHolder instanceof MenuItemViewHolder) {
            MenuItemViewHolder menuItemViewHolder = (MenuItemViewHolder) viewHolder;
            menuItemViewHolder.mIconView.setImageBitmap(null);
            BitmapDownloader.getInstance(viewHolder.view.getContext()).cancelDownload(
                    menuItemViewHolder.mBitmapCallBack);
        }
    }

    //将imageUri显示到ViewHolder中去
    private void prepareImageView(final MenuItemViewHolder menuItemViewHolder, String imageUri, MenuItem menuItem) {

        menuItemViewHolder.mIconView.setVisibility(View.INVISIBLE);
        LayoutParams lp = menuItemViewHolder.mIconView.getLayoutParams();
        Context context = menuItemViewHolder.view.getContext();
        int defaultMenuItemWidth = context.getResources().getInteger(R.integer.default_menuitem_width);
        int defaultMenuItemHeight = context.getResources().getInteger(R.integer.default_menuitem_height);
        if (imageUri != null) {
            menuItemViewHolder.mBitmapCallBack = new BitmapCallback() {
                @Override
                public void onBitmapRetrieved(Bitmap bitmap) {
                    if (bitmap != null) {
                        if(bitmap.getWidth() != defaultMenuItemWidth &&
                                bitmap.getHeight() != defaultMenuItemHeight){
                            menuItemViewHolder.mIconView.setBackground(
                                    context.getResources().getDrawable(R.drawable.aw_icon_background));
                        }
                        menuItemViewHolder.mIconView.setImageBitmap(bitmap);
                        menuItemViewHolder.mIconView.setVisibility(View.VISIBLE);
                        menuItemViewHolder.mIconView.setAlpha(0f);
                        fadeIn(menuItemViewHolder.mIconView);
                        prepareTextView(menuItemViewHolder.mTitleView, menuItem.getTitle());
                    }
                }
            };

            //异步加载图片
            BitmapWorkerOptions bitmapWorkerOptions = new BitmapWorkerOptions.Builder(context)
                    .resource(Uri.parse(imageUri))
                    .height(lp.height)
                    .width(lp.width)
                    .build();
            BitmapDownloader.getInstance(context).getBitmap(bitmapWorkerOptions,
                    menuItemViewHolder.mBitmapCallBack);
        }
    }

    private void fadeIn(View v) {
    	
        ObjectAnimator alphaAnimator = ObjectAnimator.ofFloat(v, "alpha", 1f);
        alphaAnimator.setDuration(v.getContext().getResources().getInteger(
                android.R.integer.config_shortAnimTime));
        alphaAnimator.start();
    }

    //设置title内容到textView中去
    private boolean prepareTextView(TextView textView, String title) {
    	
        boolean hasTextView = !TextUtils.isEmpty(title);
        if (hasTextView) {
            textView.setText(title);
            textView.setVisibility(View.VISIBLE);
        } else {
            textView.setVisibility(View.GONE);
        }
        return hasTextView;
    }
}
