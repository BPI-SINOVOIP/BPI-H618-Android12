package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR1;
import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
import static android.os.Build.VERSION_CODES.KITKAT_WATCH;
import static android.os.Build.VERSION_CODES.LOLLIPOP;
import static android.os.Build.VERSION_CODES.N_MR1;
import static android.os.Build.VERSION_CODES.O;
import static android.os.Build.VERSION_CODES.P;
import static android.os.Build.VERSION_CODES.S;

import android.media.AudioAttributes;
import android.os.Build.VERSION_CODES;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemVibrator;
import android.os.VibrationEffect;
import android.os.vibrator.VibrationEffectSegment;

import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

import java.util.Arrays;

@Implements(value = SystemVibrator.class, isInAndroidSdk = false)
public class ShadowSystemVibrator extends ShadowVibrator {

  private Handler handler = new Handler(Looper.myLooper());
  private Runnable stopVibratingRunnable = () -> vibrating = false;

  @Implementation
  protected boolean hasVibrator() {
    return hasVibrator;
  }

  @Implementation(minSdk = O)
  protected boolean hasAmplitudeControl() {
    return hasAmplitudeControl;
  }

  @Implementation(maxSdk = JELLY_BEAN_MR1)
  protected void vibrate(long[] pattern, int repeat) {
    recordVibratePattern(pattern, repeat);
  }

  @Implementation(minSdk = JELLY_BEAN_MR2, maxSdk = KITKAT_WATCH)
  protected void vibrate(int owningUid, String owningPackage, long[] pattern, int repeat) {
    recordVibratePattern(pattern, repeat);
  }

  @Implementation(minSdk = LOLLIPOP, maxSdk = N_MR1)
  protected void vibrate(int uid, String opPkg, long[] pattern, int repeat, AudioAttributes attributes) {
    recordVibratePattern(pattern, repeat);
  }

  @Implementation(maxSdk = JELLY_BEAN_MR1)
  public void vibrate(long milliseconds) {
    recordVibrate(milliseconds);
  }

  @Implementation(minSdk = JELLY_BEAN_MR2, maxSdk = KITKAT_WATCH)
  public void vibrate(int owningUid, String owningPackage, long milliseconds) {
    recordVibrate(milliseconds);
  }

  @Implementation(minSdk = LOLLIPOP, maxSdk = N_MR1)
  protected void vibrate(int uid, String opPkg, long milliseconds, AudioAttributes attributes) {
    recordVibrate(milliseconds);
  }

  @Implementation(minSdk = O, maxSdk = VERSION_CODES.P)
  protected void vibrate(int uid, String opPkg, VibrationEffect effect, AudioAttributes attributes) {
    vibrate(uid, opPkg, effect, null, attributes);
  }

  @Implementation(minSdk = VERSION_CODES.CUR_DEVELOPMENT)
  protected void vibrate(int uid, String opPkg, VibrationEffect effect,
          String reason, AudioAttributes attributes) {
    long duration;
    if (RuntimeEnvironment.getApiLevel() >= P) {
      duration = effect.getDuration();
    } else {
      duration = ReflectionHelpers.callInstanceMethod(effect, "getTiming");
    }

    if (RuntimeEnvironment.getApiLevel() >= S) {
      long[] pattern = ((VibrationEffect.Composed) effect).getSegments().stream()
              .mapToLong(VibrationEffectSegment::getDuration)
              .toArray();
      int repeatIndex = ((VibrationEffect.Composed) effect).getRepeatIndex();
      recordVibratePattern(duration, pattern, repeatIndex);
      return;
    }

    if (RuntimeEnvironment.getApiLevel() >= O) {
      try {
        // Try to extract fields from VibrationEffect.Waveform.
        long[] pattern = ReflectionHelpers.callInstanceMethod(effect, "getTimings");
        int repeatIndex = ReflectionHelpers.callInstanceMethod(effect, "getRepeatIndex");
        recordVibratePattern(pattern, repeatIndex);
        return;
      } catch (RuntimeException e) {
        // Not a VibrationEffect.Waveform, fallback to generic effect.
      }
    }

    recordVibrate(duration);
  }

  private void recordVibrate(long milliseconds) {
    vibrating = true;
    this.milliseconds = milliseconds;
    handler.removeCallbacks(stopVibratingRunnable);
    handler.postDelayed(stopVibratingRunnable, this.milliseconds);
  }

  private void recordVibratePattern(long[] pattern, int repeat) {
    long duration = repeat < 0 ? Arrays.stream(pattern).sum() : Long.MAX_VALUE;
    recordVibratePattern(duration, pattern, repeat);
  }

  private void recordVibratePattern(long milliseconds, long[] pattern, int repeat) {
    vibrating = true;
    this.milliseconds = milliseconds;
    this.pattern = pattern;
    this.repeat = repeat;
    handler.removeCallbacks(stopVibratingRunnable);
    if (milliseconds < Long.MAX_VALUE) {
      handler.postDelayed(stopVibratingRunnable, milliseconds);
    }
  }

  @Implementation
  protected void cancel() {
    cancelled = true;
    vibrating = false;
    handler.removeCallbacks(stopVibratingRunnable);
  }
}
