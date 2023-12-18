package com.android.systemui.statusbar.tv;

import android.content.Context;

import com.android.systemui.statusbar.policy.HeadsUpManager;

import javax.inject.Inject;

public class TvHeadsUpManager extends HeadsUpManager {
    @Inject
    public TvHeadsUpManager(Context context) {
        super(context);
    }
}
