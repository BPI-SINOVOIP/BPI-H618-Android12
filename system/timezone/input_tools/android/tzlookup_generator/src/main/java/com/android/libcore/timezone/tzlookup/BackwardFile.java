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

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.ParseException;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * A class that knows about the structure of the IANA tzdb backward file.
 */
final class BackwardFile {

    private final Map<String, String> links = new HashMap<>();
    private Map<String, String> directLinks;

    private BackwardFile() {}

    static BackwardFile parse(String backwardFile) throws IOException, ParseException {
        BackwardFile backward = new BackwardFile();

        List<String> lines = Files
                .readAllLines(Paths.get(backwardFile), StandardCharsets.US_ASCII);

        // Remove comments
        List<String> linkLines =
                lines.stream()
                        .filter(s -> !(s.startsWith("#") || s.isEmpty()))
                        .collect(Collectors.toList());

        for (String linkLine : linkLines) {
            String[] fields = linkLine.split("\t+");
            if (fields.length < 3 || !fields[0].equals("Link")) {
                throw new ParseException("Line is malformed: " + linkLine, 0);
            }
            backward.addLink(fields[1], fields[2]);
        }
        return backward;
    }

    /**
     * Add a link entry.
     *
     * @param target the new tz ID
     * @param linkName the old tz ID
     */
    private void addLink(String target, String linkName) {
        String oldValue = links.put(linkName, target);
        if (oldValue != null) {
            throw new IllegalStateException("Duplicate link from " + linkName);
        }
    }

    /**
     * Returns a set of IDs linked to the supplied ID, even intermediate ones in a chain of links.
     */
    Set<String> getAllAlternativeIds(String zoneId) {
        Set<String> knownAlternativeIds = new HashSet<>();
        // Add the ID we're searching for. We don't need to look for it.
        knownAlternativeIds.add(zoneId);

        LinkedList<String> searchIdQueue = new LinkedList<>();
        searchIdQueue.add(zoneId);
        while (!searchIdQueue.isEmpty()) {
            String searchId = searchIdQueue.removeLast();
            for (Map.Entry<String, String> entry : links.entrySet()) {
                String fromId = entry.getKey();
                String toId = entry.getValue();
                if (fromId.equals(searchId)) {
                    if (knownAlternativeIds.add(toId)) {
                        searchIdQueue.add(toId);
                    }
                } else if (toId.equals(searchId)) {
                    if (knownAlternativeIds.add(fromId)) {
                        searchIdQueue.add(fromId);
                    }
                }
            }
        }

        // Remove the zone we were searching for - it's not an alternative for itself.
        knownAlternativeIds.remove(zoneId);
        return Collections.unmodifiableSet(knownAlternativeIds);
    }

    Map<String, String> getLinks() {
        return Collections.unmodifiableMap(links);
    }

    /** Returns a mapping from linkName (old tz ID) to target (new tz ID). */
    Map<String, String> getDirectLinks() {
        if (directLinks == null) {
            // Validate links for cycles and collapse the links to remove intermediates if there are
            // links to links. There's a simple check to confirm that no chain is longer than a
            // fixed length, to guard against cycles.
            final int maxChainLength = 2;
            Map<String, String> collapsedLinks = new HashMap<>();
            for (String fromId : links.keySet()) {
                int chainLength = 0;
                String currentId = fromId;
                String lastId = null;
                while ((currentId = links.get(currentId)) != null) {
                    chainLength++;
                    lastId = currentId;
                    if (chainLength > maxChainLength) {
                        throw new IllegalStateException(
                                "Chain from " + fromId + " is longer than " + maxChainLength);
                    }
                }
                if (chainLength == 0) {
                    throw new IllegalStateException("Null Link targetId for " + fromId);
                }
                collapsedLinks.put(fromId, lastId);
            }
            directLinks = Collections.unmodifiableMap(collapsedLinks);
        }
        return directLinks;
    }
}
