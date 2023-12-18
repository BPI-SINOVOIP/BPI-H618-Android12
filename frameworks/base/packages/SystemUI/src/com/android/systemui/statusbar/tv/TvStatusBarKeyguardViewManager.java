package com.android.systemui.statusbar.tv;

import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewRootImpl;

import com.android.keyguard.KeyguardViewController;
import com.android.systemui.statusbar.phone.BiometricUnlockController;
import com.android.systemui.statusbar.phone.KeyguardBypassController;
import com.android.systemui.statusbar.phone.NotificationPanelViewController;
import com.android.systemui.statusbar.phone.StatusBar;

import javax.inject.Inject;

public class TvStatusBarKeyguardViewManager implements KeyguardViewController {
    @Inject
    public TvStatusBarKeyguardViewManager() {

    }

    @Override
    public void show(Bundle options) {

    }

    @Override
    public void hide(long startTime, long fadeoutDuration) {

    }

    @Override
    public void reset(boolean hideBouncerWhenShowing) {

    }

    @Override
    public void setNeedsInput(boolean needsInput) {

    }

    @Override
    public void onCancelClicked() {

    }

    @Override
    public void setOccluded(boolean occluded, boolean animate) {

    }

    @Override
    public boolean isShowing() {
        return false;
    }

    @Override
    public void dismissAndCollapse() {

    }

    @Override
    public void keyguardGoingAway() {

    }

    @Override
    public void setKeyguardGoingAwayState(boolean isKeyguardGoingAway) {

    }

    @Override
    public boolean shouldDisableWindowAnimationsForUnlock() {
        return false;
    }

    @Override
    public boolean isGoingToNotificationShade() {
        return false;
    }

    @Override
    public boolean isUnlockWithWallpaper() {
        return false;
    }

    @Override
    public boolean shouldSubtleWindowAnimationsForUnlock() {
        return false;
    }

    @Override
    public void startPreHideAnimation(Runnable finishRunnable) {

    }

    @Override
    public void blockPanelExpansionFromCurrentTouch() {

    }

    @Override
    public ViewRootImpl getViewRootImpl() {
        return null;
    }

    @Override
    public void notifyKeyguardAuthenticated(boolean strongAuth) {

    }

    @Override
    public void showBouncer(boolean scrimmed) {

    }

    @Override
    public boolean isBouncerShowing() {
        return false;
    }

    @Override
    public boolean bouncerIsOrWillBeShowing() {
        return false;
    }

    @Override
    public void registerStatusBar(StatusBar statusBar, ViewGroup container, NotificationPanelViewController notificationPanelViewController, BiometricUnlockController biometricUnlockController, View notificationContainer, KeyguardBypassController bypassController) {

    }
}
