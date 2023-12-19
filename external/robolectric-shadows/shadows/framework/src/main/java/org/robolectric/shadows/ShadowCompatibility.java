package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.S;

import android.compat.Compatibility;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@Implements(value = Compatibility.class, minSdk = S)
public class ShadowCompatibility {
  // override to avoid calling to System.logW in the uninstrumented interface default method
  @Implementation
  public static boolean isChangeEnabled(long changeId) {
    return true;
  }
}
