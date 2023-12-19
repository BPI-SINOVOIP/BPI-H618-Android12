// BEGIN-INTERNAL

package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.S;

import android.permission.PermissionManager;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import java.util.Collections;
import java.util.List;

@Implements(value = PermissionManager.class, minSdk = S, isInAndroidSdk = false)
public class ShadowPermissionManager {
  @Implementation
  public List<PermissionManager.SplitPermissionInfo> getSplitPermissions() {
    return Collections.emptyList();
  }
}
// END-INTERNAL
