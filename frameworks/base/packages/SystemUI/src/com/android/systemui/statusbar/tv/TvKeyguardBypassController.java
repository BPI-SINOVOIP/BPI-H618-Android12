package com.android.systemui.statusbar.tv;

import com.android.systemui.statusbar.notification.stack.StackScrollAlgorithm;
import javax.inject.Inject;
public class TvKeyguardBypassController implements StackScrollAlgorithm.BypassController {
    @Inject
    public TvKeyguardBypassController() {

    }

    @Override
    public boolean isBypassEnabled() {
        return false;
    }
}
