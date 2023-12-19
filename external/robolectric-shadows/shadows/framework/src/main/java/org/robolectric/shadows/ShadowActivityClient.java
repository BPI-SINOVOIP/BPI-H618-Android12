// BEGIN-INTERNAL

package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.S;

import android.app.ActivityClient;
import android.app.IActivityClientController;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

@Implements(value = ActivityClient.class, minSdk = S, isInAndroidSdk = false)
public class ShadowActivityClient {
  @Implementation
  protected static IActivityClientController getActivityClientController() {
    return (IActivityClientController) ReflectionHelpers.createNullProxy(
      IActivityClientController.class);
  }
}
// END-INTERNAL
