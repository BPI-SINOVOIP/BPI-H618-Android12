package com.softwinner.shared;

import android.app.Dialog;
import android.content.Context;
import android.os.Environment;
import android.os.UserHandle;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.R;

import java.io.File;


public class InstallPathDialog extends Dialog {


    public InstallPathDialog(Context context, boolean cancelable,
                             OnCancelListener cancelListener) {
        super(context, cancelable, cancelListener);

    }

    public InstallPathDialog(Context context, int theme) {
        super(context, theme);
    }

    public InstallPathDialog(Context context) {
        super(context);
    }



    public static class Builder {
        private final String TAG = "InstallPathDialog";
        private Context context;
        private OnClickListener mOnClickListener;
        private RadioGroup radioGroup_gender;
        private boolean existPath =false;
        private String path="";
        private RadioButton radioButton1;
        private RadioButton radioButton2;

        public Builder(Context context) {
            this.context = context;
        }

        private void checkVolume() {
            final int userId = UserHandle.myUserId();
            final StorageVolume[] volumes =
                    StorageManager.getVolumeList(userId, StorageManager.FLAG_FOR_WRITE);
            for (StorageVolume volume : volumes) {
                if (volume.isEmulated()) continue;
                if (!volume.isRemovable()) continue;
                if (!Environment.MEDIA_MOUNTED.equals(volume.getState())) continue;
                File sdCard = volume.getPathFile();
                if (sdCard.isDirectory()) {
                    /**
                     * If sdcard is visible, it's path starts with "/storage/" and internal path
                     * starts with "/mnt/media_rw". gsi service only take path that start with
                     * "/mnt/media_rw" as external storage path, so we use the internal path here.
                     */
                    existPath = true;
                    path = volume.getInternalPath();
                    Log.e(TAG,"path:"+path);
                    break;
                }
                if (path.isEmpty()) {
                    existPath = false;
                }
            }
        }

        public Builder setClickListener(final OnClickListener listener) {
            this.mOnClickListener = listener;
            return this;
        }


        public InstallPathDialog create() {
            checkVolume();
            LayoutInflater inflater = (LayoutInflater) context
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            final InstallPathDialog dialog = new InstallPathDialog(
                    context, R.style.Theme_Dialog_Install);
            View layout = inflater.inflate(R.layout.install_path_dialog_layout,
                    null);
            radioButton1=(RadioButton)layout.findViewById(R.id.rb1);
            radioButton2=(RadioButton)layout.findViewById(R.id.rb2);
            if(existPath){
                radioButton1.setEnabled(true);
            }else{
                radioButton1.setEnabled(false);
            }
            dialog.addContentView(layout, new LayoutParams(
                    LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
            radioGroup_gender= (RadioGroup) layout.findViewById(R.id.rg_main);
           // radioGroup_gender.setOnCheckedChangeListener(InstallPathDialog.this);
            radioGroup_gender.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    switch(checkedId) {
                        case R.id.rb1:
                            Log.e(TAG,"setpath:"+path);
                            PropertiesHelper.set(context, "os.aot.path",path);
                            break;
                        case R.id.rb2:
                            PropertiesHelper.set(context, "os.aot.path","/data/gsi");
                            break;
                    }
                }
            });
            Button confirmButton = (Button) layout.findViewById(R.id.bt_confirm);
            Button cancelButton= (Button) layout.findViewById(R.id.bt_cancel);
            confirmButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    mOnClickListener.onClick(dialog, 0);
                }
            });
            cancelButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    mOnClickListener.onClick(dialog, 1);
                }
            });
            return dialog;
        }


    }

}
