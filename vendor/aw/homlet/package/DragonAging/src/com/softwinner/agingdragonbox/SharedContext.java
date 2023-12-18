package com.softwinner.agingdragonbox;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager.NameNotFoundException;

/**
 * @author zengsc
 * @date 2013-7-30
 */
public class SharedContext extends ContextWrapper {

    public SharedContext(Context context, String packageName) {
        super(null);
        try {
            context = context.createPackageContext(packageName, Context.CONTEXT_IGNORE_SECURITY
                    | Context.CONTEXT_INCLUDE_CODE);
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        attachBaseContext(context);
    }

    public int getIdentifier(String name, String defType) {
        return getResources().getIdentifier(name, defType, getPackageName());
    }
}
