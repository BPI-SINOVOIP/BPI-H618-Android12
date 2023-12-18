/*
 * Copyright (C) 2020 The Android Open Source Project
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
package android.smartspace.cts;

import static android.app.smartspace.SmartspaceTarget.FEATURE_ALARM;

import static com.google.common.truth.Truth.assertThat;

import android.app.smartspace.SmartspaceAction;
import android.app.smartspace.SmartspaceTarget;
import android.appwidget.AppWidgetProviderInfo;
import android.content.ComponentName;
import android.net.Uri;
import android.os.Parcel;
import android.os.Process;

import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link SmartspaceTarget}
 *
 * atest CtsSmartspaceServiceTestCases
 */
@RunWith(AndroidJUnit4.class)
public class SmartspaceTargetTest {

    private static final String TAG = "SmartspaceTargetTest";

    @Test
    public void testCreateSmartspaceTarget() {
        ComponentName testComponentName = new ComponentName("package_name", "class_name");
        AppWidgetProviderInfo info = new AppWidgetProviderInfo();
        info.provider = testComponentName;
        SmartspaceTarget testTarget = new SmartspaceTarget.Builder("test_target_id",
                testComponentName, Process.myUserHandle())
                .setHeaderAction(createSmartspaceAction("header"))
                .setBaseAction(createSmartspaceAction("base"))
                .setCreationTimeMillis(10)
                .setExpiryTimeMillis(15)
                .setScore(0.5f)
                .setActionChips(createSmartspaceActionList("chips"))
                .setIconGrid(createSmartspaceActionList("icon_grid"))
                .setFeatureType(FEATURE_ALARM)
                .setSensitive(true)
                .setShouldShowExpanded(true)
                .setSourceNotificationKey("source_notification_key")
                .setAssociatedSmartspaceTargetId("associated_target_id")
                .setSliceUri(Uri.EMPTY)
                .build();


        assertThat(testTarget.getSmartspaceTargetId()).isEqualTo("test_target_id");
        assertThat(testTarget.getHeaderAction()).isEqualTo(createSmartspaceAction("header"));
        assertThat(testTarget.getBaseAction()).isEqualTo(createSmartspaceAction("base"));
        assertThat(testTarget.getCreationTimeMillis()).isEqualTo(10);
        assertThat(testTarget.getExpiryTimeMillis()).isEqualTo(15);
        assertThat(testTarget.getScore()).isEqualTo(0.5f);
        assertThat(testTarget.getActionChips().size()).isEqualTo(1);
        assertThat(testTarget.getActionChips().get(0)).isEqualTo(createSmartspaceAction("chips"));
        assertThat(testTarget.getIconGrid().size()).isEqualTo(1);
        assertThat(testTarget.getIconGrid().get(0)).isEqualTo(createSmartspaceAction("icon_grid"));
        assertThat(testTarget.getFeatureType()).isEqualTo(FEATURE_ALARM);
        assertThat(testTarget.isSensitive()).isEqualTo(true);
        assertThat(testTarget.shouldShowExpanded()).isEqualTo(true);
        assertThat(testTarget.getSourceNotificationKey()).isEqualTo("source_notification_key");
        assertThat(testTarget.getAssociatedSmartspaceTargetId()).isEqualTo("associated_target_id");
        assertThat(testTarget.getComponentName()).isEqualTo(testComponentName);
        assertThat(testTarget.getUserHandle()).isEqualTo(Process.myUserHandle());
        assertThat(testTarget.getSliceUri()).isEqualTo(Uri.EMPTY);
        Parcel parcel = Parcel.obtain();
        parcel.setDataPosition(0);
        testTarget.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        SmartspaceTarget copyTarget = SmartspaceTarget.CREATOR.createFromParcel(parcel);
        assertThat(testTarget).isEqualTo(copyTarget);
        parcel.recycle();
    }

    @Test
    public void testCreateSmartspaceTargetFromWidget() {
        ComponentName testComponentName = new ComponentName("package_name", "class_name");
        AppWidgetProviderInfo info = new AppWidgetProviderInfo();
        info.provider = testComponentName;
        SmartspaceTarget testTarget = new SmartspaceTarget.Builder("test_target_id",
                testComponentName, Process.myUserHandle())
                .setWidget(info)
                .build();

        assertThat(testTarget.getWidget().provider).isEqualTo(testComponentName);
    }

    private List<SmartspaceAction> createSmartspaceActionList(String id) {
        List<SmartspaceAction> actionList = new ArrayList<>();
        actionList.add(new SmartspaceAction.Builder(id, "test title").build());
        return actionList;
    }

    private SmartspaceAction createSmartspaceAction(String id) {
        return new SmartspaceAction.Builder(id, "test title").build();
    }

}
