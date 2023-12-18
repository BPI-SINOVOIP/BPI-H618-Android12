package com.softwinner.TvdVideo;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class IconifiedTextView extends LinearLayout {

    private TextView mText = null;
    private ImageView mIcon = null;

    public IconifiedTextView(Context context, int id, String text) {
        super(context);

        View view;
        LayoutInflater inflate = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        view = inflate.inflate(R.layout.list_item, null);
        mIcon = (ImageView) view.findViewById(R.id.list_icon);
        mText = (TextView) view.findViewById(R.id.list_text);
        mIcon.setImageDrawable(context.getResources().getDrawable(id));
        mText.setText(text);
        mText.setTextColor(0xffffffff);
        addView(view);
    }

    public void setText(String words) {
        mText.setText(words);
    }

    public void setIcon(Drawable bullet) {
        mIcon.setImageDrawable(bullet);
    }
}
