/**
 * Copyright (c) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner;

import android.view.Surface;
import android.graphics.Bitmap;

/**
 *
 * @hide
 */
interface IEinkService {
  void init(in Surface surface, in int[] location);
  void start();
  void stop();
  void setWindowFocus(boolean winFocus);
  void clear();
  void refresh();
  void setBackground(in Bitmap bitmap);
  boolean save(@utf8InCpp String path, boolean withBackground);
  int getRefreshMode();
  oneway void setRefreshMode(int mode);
  int forceGlobalRefresh(boolean rightNow);
  int autoTest(boolean start, int interval, int pressureMin, int pressureMax);
}
