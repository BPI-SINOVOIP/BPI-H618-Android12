package com.android.systemui.statusbar.tv;

import com.android.systemui.statusbar.phone.ShadeController;

import javax.inject.Inject;

public class TvShadeController implements ShadeController {
    @Inject
    public TvShadeController() {

    }

    @Override
    public void instantExpandNotificationsPanel() {

    }

    @Override
    public void animateCollapsePanels() {

    }

    @Override
    public void animateCollapsePanels(int flags) {

    }

    @Override
    public void animateCollapsePanels(int flags, boolean force) {

    }

    @Override
    public void animateCollapsePanels(int flags, boolean force, boolean delayed) {

    }

    @Override
    public void animateCollapsePanels(int flags, boolean force, boolean delayed, float speedUpFactor) {

    }

    @Override
    public boolean closeShadeIfOpen() {
        return false;
    }

    @Override
    public void postOnShadeExpanded(Runnable action) {

    }

    @Override
    public void addPostCollapseAction(Runnable action) {

    }

    @Override
    public void runPostCollapseRunnables() {

    }

    @Override
    public boolean collapsePanel() {
        return false;
    }

    @Override
    public void collapsePanel(boolean animate) {

    }
}
