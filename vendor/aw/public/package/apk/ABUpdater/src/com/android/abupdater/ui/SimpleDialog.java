package com.android.abupdater.ui;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Button;
import android.util.Log;

import com.android.abupdater.R;

public class SimpleDialog {

    private static final String TAG = "SimpleDialog";
    private static final boolean debug = true;
    private Context mContext;

    protected SimpleDialog(Context context) {
        mContext = context;
    }

    public void showUseDialog(String title, String msg,
                String positiveMsg, PositiveInterface posInterface,
                String negativeMsg, NegativeInterface negInterface) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View view = inflater.inflate(R.layout.dialog_layout, null);
        TextView showMsgTitle = (TextView) view.findViewById(R.id.show_msg_title);
        TextView showMsgContent = (TextView) view.findViewById(R.id.show_msg_content);
        Button showMsgConfirm = (Button) view.findViewById(R.id.show_msg_confirm);
        Button showMsgCancel = (Button) view.findViewById(R.id.show_msg_cancel);

        showMsgTitle.setText(title);
        showMsgContent.setText(msg);
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(mContext)
            .setView(view)
            .setCancelable(false);
        AlertDialog dialog = dialogBuilder.create();
        showMsgConfirm.setText(positiveMsg);
        showMsgCancel.setText(negativeMsg);
        showMsgConfirm.setOnClickListener(v -> {
            if (posInterface != null) {
                posInterface.positiveFunc();
            }
            dialog.dismiss();
        });
        showMsgCancel.setOnClickListener(v -> {
            if (negInterface != null) {
                negInterface.negativeFunc();
            }
            dialog.dismiss();
        });
        dialog.show();
    }

    @FunctionalInterface
    interface PositiveInterface {
        void positiveFunc();
    }

    @FunctionalInterface
    interface NegativeInterface {
        void negativeFunc();
    }

}
