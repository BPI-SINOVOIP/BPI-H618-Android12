package com.softwinner.shared;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.TextView;

import com.softwinner.update.R;


public class InstallPowerDialog extends Dialog {

    public InstallPowerDialog(Context context, boolean cancelable,
                              OnCancelListener cancelListener) {
        super(context, cancelable, cancelListener);

    }

    public InstallPowerDialog(Context context, int theme) {
        super(context, theme);
    }

    public InstallPowerDialog(Context context) {
        super(context);
    }

    public static class Builder {

        private Context context;
        private DialogInterface.OnClickListener mOnClickListener;

        public Builder(Context context) {
            this.context = context;
        }

        public Builder setClickListener(final OnClickListener listener) {
            this.mOnClickListener = listener;
            return this;
        }


        public InstallPowerDialog create() {
            LayoutInflater inflater = (LayoutInflater) context
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            final InstallPowerDialog dialog = new InstallPowerDialog(
                    context, R.style.Theme_Dialog_Install);
            View layout = inflater.inflate(R.layout.install_dialog_layout,
                    null);
            dialog.addContentView(layout, new LayoutParams(
                    LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
            TextView cancelTxt = (TextView) layout
                    .findViewById(R.id.dialog_cancel);
            TextView confirmTxt = (TextView) layout
                    .findViewById(R.id.dialog_confirm);

            cancelTxt.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    mOnClickListener.onClick(dialog, 0);
                }
            });

            confirmTxt.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    mOnClickListener.onClick(dialog, 1);
                }
            });

            return dialog;
        }

    }

}
