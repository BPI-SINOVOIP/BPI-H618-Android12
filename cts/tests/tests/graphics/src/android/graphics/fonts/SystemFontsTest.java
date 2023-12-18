/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.graphics.fonts;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.nio.ReadOnlyBufferException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Set;

@SmallTest
@RunWith(Parameterized.class)
public class SystemFontsTest {

    @Parameterized.Parameter(0)
    public Set<Font> availableFonts;

    @Parameterized.Parameters
    public static Collection<Object[]> getParameters() {
        ArrayList<Object[]> allParams = new ArrayList<>();
        allParams.add(new Object[] { SystemFonts.getAvailableFonts() });
        return allParams;
    }

    @Test
    public void testAvailableFonts_NotEmpty() {
        assertNotEquals("System available fonts must not be empty", 0, availableFonts.size());
    }

    @Test
    public void testAvailableFonts_ReadOnlyFile() throws ErrnoException {
        for (Font font : availableFonts) {
            assertNotNull("System font must provide file path to the font file.", font.getFile());

            final String absPath = font.getFile().getAbsolutePath();

            // The system font must be read-only file.
            assertTrue(absPath + " must exists", font.getFile().exists());
            assertTrue(absPath + " must be a file", font.getFile().isFile());
            assertTrue(absPath + " must be readable", font.getFile().canRead());
            assertFalse(absPath + " must not executable", font.getFile().canExecute());
            assertFalse(absPath + " must not writable", font.getFile().canWrite());

            // The update font files will be in /data directory which is not usually under the
            // read-only file system.
            if (!absPath.startsWith("/data/fonts/")) {
                // The system font must be in read-only file system.
                assertTrue(absPath + " is not in the read-only file system.",
                        (Os.statvfs(absPath).f_flag & OsConstants.ST_RDONLY) != 0);
            }
        }
    }

    @Test
    public void testAvailableFonts_ReadOnlyBuffer() {
        for (Font font : availableFonts) {
            try {
                font.getBuffer().put((byte) 0);
                fail("System font must be read only");
            } catch (ReadOnlyBufferException e) {
                // pass
            }
        }
    }

    @Test
    public void testAvailableFonts_FontAttributeGetters() {
        // Because system fonts are configurable by device, we cannot assert specific values.
        // Instead, we call attribute getter methods and verify if they returns valid values.
        for (Font font : availableFonts) {
            assertNotNull(font.getStyle());
            assertNotNull(font.getLocaleList());
            assertThat(font.getTtcIndex()).isAtLeast(0);
            FontVariationAxis[] axes = font.getAxes();
            if (axes != null) {
                for (FontVariationAxis axis : axes) {
                    assertNotNull(axis);
                }
            }
        }
    }
}
