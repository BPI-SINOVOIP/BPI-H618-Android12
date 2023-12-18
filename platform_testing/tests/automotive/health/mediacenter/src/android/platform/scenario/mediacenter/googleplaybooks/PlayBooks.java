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
package android.platform.test.scenario.mediacenter.googleplaybooks;

import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoMediaCenterHelper;
import android.platform.test.scenario.annotation.Scenario;
import androidx.test.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;

/** Plays an audio book assuming Google Play Books is open */
@Scenario
@RunWith(JUnit4.class)
public class PlayBooks {

    private String mNameOftheSong;
    static HelperAccessor<IAutoMediaCenterHelper> sMediaCenterHelper =
            new HelperAccessor<>(IAutoMediaCenterHelper.class);

    @Test
    public void testOpen() throws IOException {
        mNameOftheSong =
                InstrumentationRegistry.getArguments()
                        .getString("song_name", "Once Gone (a Riley Paige Mystery--Book #1)");
        sMediaCenterHelper.get().selectMediaTrack("Library", mNameOftheSong);
    }
}
