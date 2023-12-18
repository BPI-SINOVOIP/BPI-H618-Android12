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

package com.android.libcore.timezone.tzlookup;

import static junit.framework.TestCase.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.android.libcore.timezone.testing.TestUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.text.ParseException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class BackwardFileTest {

    private Path tempDir;

    @Before
    public void setUp() throws Exception {
        tempDir = Files.createTempDirectory("BackwardFileTest");
    }

    @After
    public void tearDown() throws Exception {
        TestUtils.deleteDir(tempDir);
    }

    @Test
    public void parseEmpty() throws Exception {
        String file = createFile("");
        BackwardFile backward = BackwardFile.parse(file);
        assertTrue(backward.getDirectLinks().isEmpty());
    }

    @Test
    public void parseIgnoresCommentsAndEmptyLines() throws Exception {
        String file = createFile(
                "# This is a comment",
                "",
                "# And another",
                "Link\tAmerica/Nuuk\t\tAmerica/Godthab"
        );
        BackwardFile backward = BackwardFile.parse(file);

        Map<String, String> expectedLinks = new HashMap<>();
        expectedLinks.put("America/Godthab", "America/Nuuk");
        assertEquals(expectedLinks, backward.getDirectLinks());
    }

    @Test
    public void parse() throws Exception {
        String file = createFile(
                "# This is a comment",
                "Link\tAmerica/Nuuk\t\tAmerica/Godthab",
                "# This is a comment",
                "Link\tAfrica/Nairobi\t\tAfrica/Asmera",
                "# This is a comment",
                "Link\tAfrica/Abidjan\t\tAfrica/Timbuktu",
                "# This is a comment",
                "Link\tAfrica/Timbuktu\t\tAfrica/Timbuktu2"
        );
        BackwardFile backward = BackwardFile.parse(file);
        Map<String, String> expectedLinks = new HashMap<>();
        expectedLinks.put("America/Godthab", "America/Nuuk");
        expectedLinks.put("Africa/Asmera", "Africa/Nairobi");
        expectedLinks.put("Africa/Timbuktu", "Africa/Abidjan");
        expectedLinks.put("Africa/Timbuktu2", "Africa/Timbuktu");
        assertEquals(expectedLinks, backward.getLinks());

        Map<String, String> expectedDirectLinks = new HashMap<>();
        expectedDirectLinks.put("America/Godthab", "America/Nuuk");
        expectedDirectLinks.put("Africa/Asmera", "Africa/Nairobi");
        expectedDirectLinks.put("Africa/Timbuktu", "Africa/Abidjan");
        expectedDirectLinks.put("Africa/Timbuktu2", "Africa/Abidjan");
        assertEquals(expectedDirectLinks, backward.getDirectLinks());

        assertEquals(set("Africa/Abidjan", "Africa/Timbuktu2"),
                backward.getAllAlternativeIds("Africa/Timbuktu"));
        assertEquals(set("Africa/Abidjan", "Africa/Timbuktu"),
                backward.getAllAlternativeIds("Africa/Timbuktu2"));
        assertEquals(set("Africa/Timbuktu", "Africa/Timbuktu2"),
                backward.getAllAlternativeIds("Africa/Abidjan"));
    }

    @Test(expected = IllegalStateException.class)
    public void getDirectLinksWithLoop() throws Exception {
        String file = createFile(
                "Link\tAmerica/New_York\t\tAmerica/Los_Angeles",
                "Link\tAmerica/Los_Angeles\t\tAmerica/Phoenix",
                "Link\tAmerica/Phoenix\t\tAmerica/New_York"
        );
        BackwardFile backward = BackwardFile.parse(file);
        backward.getDirectLinks();
    }

    @Test(expected = IllegalStateException.class)
    public void parseWithDupes() throws Exception {
        String file = createFile(
                "Link\tAmerica/New_York\t\tAmerica/Los_Angeles",
                "Link\tAmerica/Phoenix\t\tAmerica/Los_Angeles"
        );
        BackwardFile.parse(file);
    }

    @Test(expected = ParseException.class)
    public void parseMalformedFile() throws Exception {
        // Mapping lines are expected to have at least three tab-separated columns.
        String file = createFile("NotLink\tBooHoo");
        BackwardFile.parse(file);
    }

    private String createFile(String... lines) throws IOException {
        return TestUtils.createFile(tempDir, lines);
    }

    private static <T> Set<T> set(T... values) {
        return new HashSet<>(Arrays.asList(values));
    }
}
