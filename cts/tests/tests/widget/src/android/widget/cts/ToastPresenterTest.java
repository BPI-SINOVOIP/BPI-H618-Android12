/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.widget.cts;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertEquals;

import android.app.INotificationManager;
import android.content.Context;
import android.os.Binder;
import android.os.ServiceManager;
import android.view.View;
import android.view.WindowManager;
import android.view.accessibility.IAccessibilityManager;
import android.widget.FrameLayout;
import android.widget.ToastPresenter;

import androidx.test.annotation.UiThreadTest;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class ToastPresenterTest {
    private static final String PACKAGE_NAME = "pkg";

    private Context mContext;
    private ToastPresenter mToastPresenter;

    @Before
    public void setup() {
        mContext = getInstrumentation().getContext();

        mToastPresenter = new ToastPresenter(
                mContext,
                IAccessibilityManager.Stub.asInterface(
                        ServiceManager.getService(Context.ACCESSIBILITY_SERVICE)),
                INotificationManager.Stub.asInterface(
                        ServiceManager.getService(Context.NOTIFICATION_SERVICE)),
                PACKAGE_NAME);
    }

    @UiThreadTest
    @Test
    public void testUpdateLayoutParams() {
        View view = new FrameLayout(mContext);
        Binder token = new Binder();
        Binder windowToken = new Binder();
        mToastPresenter.show(view, token, windowToken, 0, 0, 0, 0, 0, 0, null);
        mToastPresenter.updateLayoutParams(1, 2, 3, 4, 0);

        WindowManager.LayoutParams lp = (WindowManager.LayoutParams) view.getLayoutParams();
        assertEquals(1, lp.x);
        assertEquals(2, lp.y);
        assertEquals(3, (int) lp.horizontalMargin);
        assertEquals(4, (int) lp.verticalMargin);
    }
}
