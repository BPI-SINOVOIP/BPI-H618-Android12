package com.android.systemui.statusbar.tv;

import com.android.systemui.doze.DozeHost;

import javax.inject.Inject;

public class TvDozeServiceHost implements DozeHost {
    @Inject
    public TvDozeServiceHost() {

    }

    @Override
    public void addCallback(Callback callback) {

    }

    @Override
    public void removeCallback(Callback callback) {

    }

    @Override
    public void startDozing() {

    }

    @Override
    public void pulseWhileDozing(PulseCallback callback, int reason) {

    }

    @Override
    public void stopDozing() {

    }

    @Override
    public void dozeTimeTick() {

    }

    @Override
    public boolean isPowerSaveActive() {
        return false;
    }

    @Override
    public boolean isPulsingBlocked() {
        return false;
    }

    @Override
    public boolean isProvisioned() {
        return false;
    }

    @Override
    public boolean isBlockingDoze() {
        return false;
    }

    @Override
    public void extendPulse(int reason) {

    }

    @Override
    public void setAnimateWakeup(boolean animateWakeup) {

    }

    @Override
    public void setAnimateScreenOff(boolean animateScreenOff) {

    }

    @Override
    public void onSlpiTap(float x, float y) {

    }

    @Override
    public void setDozeScreenBrightness(int value) {

    }

    @Override
    public void prepareForGentleSleep(Runnable onDisplayOffCallback) {

    }

    @Override
    public void cancelGentleSleep() {

    }

    @Override
    public void onIgnoreTouchWhilePulsing(boolean ignore) {

    }

    @Override
    public void stopPulsing() {

    }

    @Override
    public boolean isDozeSuppressed() {
        return false;
    }
}
