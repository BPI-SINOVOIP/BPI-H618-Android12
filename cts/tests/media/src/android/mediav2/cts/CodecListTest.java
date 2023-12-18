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

package android.mediav2.cts;

import android.media.MediaFormat;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;

import static org.junit.Assert.assertTrue;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class CodecListTest {

    /**
     * Tests if the device under test has support for required components as guided by CDD.
     * The test only checks for the components availability. It doesn't check if the component
     * adheres to additional requirements (support for min profile/level, ...) set by cdd.
     */
    @Test
    public void testCddRequiredCodecsAvailability() {
        final boolean needAudio = true;
        final boolean needVideo = true;
        boolean[] modes = {true, false};
        for (boolean isEncoder : modes) {
            ArrayList<String> cddRequiredMimeList =
                    CodecTestBase.compileRequiredMimeList(isEncoder, needAudio, needVideo);
            for (String mime : cddRequiredMimeList) {
                String log = String.format("no %s found for mime %s as required by cdd ",
                        isEncoder ? "encoder" : "decoder", mime);
                assertTrue(log, isEncoder ? CodecTestBase.hasEncoder(mime) :
                                        CodecTestBase.hasDecoder(mime));
            }
        }
        if (CodecTestBase.hasCamera()) {
            assertTrue("device doesn't support either VP8 or AVC video encoders",
                    CodecTestBase.hasEncoder(MediaFormat.MIMETYPE_VIDEO_AVC) ||
                            CodecTestBase.hasEncoder(MediaFormat.MIMETYPE_VIDEO_VP8));
        }
    }
}

