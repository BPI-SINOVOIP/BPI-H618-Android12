package com.softwinner.update.Widget;

import com.softwinner.update.R;

import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.ViewGroup.LayoutParams;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

public class CustomToast {

	public static void show(Context context, String tip, boolean isLongTime) {

		Toast toast = new Toast(context);
		if (isLongTime) {

			toast.setDuration(Toast.LENGTH_LONG);
		} else {

			toast.setDuration(Toast.LENGTH_SHORT);
		}
		toast.setGravity(Gravity.CENTER, 0, 300);// 设置Toast的位置
		RelativeLayout rl = new RelativeLayout(context);
		rl.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT));
		TextView textView = new TextView(context);
		textView.setTextColor(Color.WHITE);
		textView.setTextSize(18);
		textView.setText(tip);
		textView.setBackgroundResource(R.drawable.bubble_background);// 自定义的图片
		textView.setGravity(Gravity.CENTER);
		rl.setPadding(40, 0, 40, 0);// 设置Toast距离屏幕左右两边的距离
		rl.addView(textView);

		toast.setView(rl);
		toast.show();

	}

	public static void show(Context context, String tip) {

		CustomToast.show(context, tip, false);
	}

}