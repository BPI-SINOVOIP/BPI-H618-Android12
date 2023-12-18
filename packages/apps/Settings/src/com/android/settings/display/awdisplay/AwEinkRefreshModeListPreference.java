/*
 * Add by Setting Aw Eink refresh mode
 *
 * @author humingming@allwinnertech.com
 */

package com.android.settings.display.awdisplay;

import static com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import androidx.appcompat.app.AlertDialog.Builder;

import com.android.settings.R;
import com.android.settings.RestrictedListPreference;
import com.android.settingslib.RestrictedLockUtils;

import java.util.ArrayList;


public class AwEinkRefreshModeListPreference extends RestrictedListPreference {
    private static final String TAG = "AwEinkRefreshModeListPreference";

    public AwEinkRefreshModeListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onPrepareDialogBuilder(Builder builder,
            DialogInterface.OnClickListener listener) {
        super.onPrepareDialogBuilder(builder, listener);
        builder.setView(null);
    }

    @Override
    protected void onDialogCreated(Dialog dialog) {
        super.onDialogCreated(dialog);
        dialog.create();
    }

}
