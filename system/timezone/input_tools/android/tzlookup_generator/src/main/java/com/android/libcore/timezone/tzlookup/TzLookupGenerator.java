/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.libcore.timezone.countryzones.proto.CountryZonesFile;
import com.android.libcore.timezone.tzlookup.zonetree.CountryZoneTree;
import com.android.libcore.timezone.tzlookup.zonetree.CountryZoneUsage;
import com.android.libcore.timezone.util.Errors;
import com.android.libcore.timezone.util.Errors.HaltExecutionException;
import com.android.timezone.tzids.TimeZoneIds;
import com.android.timezone.tzids.proto.TzIdsProto;
import com.ibm.icu.util.BasicTimeZone;
import com.ibm.icu.util.Calendar;
import com.ibm.icu.util.GregorianCalendar;
import com.ibm.icu.util.TimeZone;
import com.ibm.icu.util.TimeZoneRule;

import java.io.File;
import java.io.IOException;
import java.text.ParseException;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.xml.stream.XMLStreamException;

/**
 * Generates Android's tzlookup.xml and tzids.prototxt file using ICU4J, Android's countryzones.txt
 * file, and TZDB's backwards and zones.tab files.
 */
public final class TzLookupGenerator {

    /**
     * The start time (inclusive) for calculating country zone rules. 19700101 00:00:00 UTC. Chosen
     * because this is the point in time for which the tzdb zone.tab data is supposed to be correct.
     */
    public static final Instant ZONE_USAGE_CALCS_START = Instant.EPOCH;

    /**
     * The end time (exclusive) for generating country zone usage. 20380119 03:14:07 UTC. Any times
     * after this will be considered "infinity" for the "notafter" value and not included. Chosen
     * because this is a "nice round number" and has historical significance for people that deal
     * with computer time. There is no particular reason to choose this over another time; any
     * future time after the last time we expect the code to reasonably encounter will do.
     */
    public static final Instant ZONE_USAGE_NOT_AFTER_CUT_OFF =
            Instant.ofEpochSecond(Integer.MAX_VALUE);

    /**
     * The end time (exclusive) for calculating country zone usage. The time zone periods are
     * calculated to this point. The main requirement is that it's after
     * {@link #ZONE_USAGE_NOT_AFTER_CUT_OFF} by an amount larger than the usual daylight savings
     * period; here we use 2 years.
     */
    public static final Instant ZONE_USAGE_CALCS_END =
            ZONE_USAGE_NOT_AFTER_CUT_OFF.plus(2 * 365, ChronoUnit.DAYS);

    private final String countryZonesFileIn;
    private final String zoneTabFileIn;
    private final String backwardFileIn;
    private final String tzLookupXmlFileOut;
    private final String timeZoneIdsFileOut;

    /**
     * Executes the generator.
     */
    public static void main(String[] args) throws Exception {
        if (args.length != 5) {
            System.err.println(
                    "usage: java com.android.libcore.timezone.tzlookup.TzLookupGenerator"
                            + " <[in] countryzones.txt file> <[in] zone.tab file>"
                            + " <[in] backward file>"
                            + " <[out] tzlookup.xml file> <[out] zone IDs file>");
            System.exit(0);
        }
        TzLookupGenerator tzLookupGenerator =
                new TzLookupGenerator(args[0], args[1], args[2], args[3], args[4]);
        boolean success = tzLookupGenerator.execute();
        System.exit(success ? 0 : 1);
    }

    TzLookupGenerator(String countryZonesFileIn, String zoneTabFileIn, String backwardFileIn,
            String tzLookupXmlFileOut, String timeZoneIdsFileOut) {
        this.countryZonesFileIn = countryZonesFileIn;
        this.zoneTabFileIn = zoneTabFileIn;
        this.backwardFileIn = backwardFileIn;
        this.tzLookupXmlFileOut = tzLookupXmlFileOut;
        this.timeZoneIdsFileOut = timeZoneIdsFileOut;
    }

    boolean execute() {
        Errors errors = new Errors();
        try {
            // Parse the countryzones input file.
            CountryZonesFile.CountryZones countryZonesIn =
                    parseAndValidateCountryZones(countryZonesFileIn, errors);

            // Check the countryzones.txt rules version matches the version that ICU is using.
            String icuTzDataVersion = TimeZone.getTZDataVersion();
            String inputIanaVersion = countryZonesIn.getIanaVersion();
            if (!icuTzDataVersion.equals(inputIanaVersion)) {
                throw errors.addFatalAndHalt("Input data (countryzones.txt) is for "
                        + inputIanaVersion + " but the ICU you have is for " + icuTzDataVersion);
            }

            // Pull out information we want to validate against from zone.tab (which we have to
            // assume matches the ICU version since it doesn't contain its own version info).
            Map<String, List<String>> zoneTabMapping = parseZoneTabFile(zoneTabFileIn, errors);

            List<CountryZonesFile.Country> countriesIn = countryZonesIn.getCountriesList();
            List<String> countriesInIsos = CountryZonesFileSupport.extractIsoCodes(countriesIn);

            // Confidence check the countryzones file only contains lower-case country codes. The
            // output file uses them and the on-device code assumes lower case.
            if (!Utils.allLowerCaseAscii(countriesInIsos)) {
                throw errors.addFatalAndHalt(
                        "Non-lowercase country ISO codes found in: " + countriesInIsos);
            }
            // Confidence check the countryzones file doesn't contain duplicate country entries.
            if (!Utils.allUnique(countriesInIsos)) {
                throw errors.addFatalAndHalt(
                        "Duplicate input country entries found: " + countriesInIsos);
            }

            // Validate the country iso codes found in the countryzones.txt against those in
            // zone.tab. zone.tab uses upper case, countryzones uses lower case.
            List<String> upperCaseCountriesInIsos = Utils.toUpperCase(countriesInIsos);
            Set<String> timezonesCountryIsos = new HashSet<>(upperCaseCountriesInIsos);
            Set<String> zoneTabCountryIsos = zoneTabMapping.keySet();
            if (!zoneTabCountryIsos.equals(timezonesCountryIsos)) {
                throw errors.addFatalAndHalt(zoneTabFileIn + " contains "
                        + Utils.subtract(zoneTabCountryIsos, timezonesCountryIsos)
                        + " not present in countryzones, "
                        + countryZonesFileIn + " contains "
                        + Utils.subtract(timezonesCountryIsos, zoneTabCountryIsos)
                        + " not present in zonetab.");
            }

            // Obtain and validate a mapping from old IDs to new IDs.
            BackwardFile backwardIn = parseAndValidateBackwardFile(backwardFileIn, errors);
            errors.throwIfError("Errors accumulated");

            OutputData outputData = createOutputData(
                    inputIanaVersion, zoneTabMapping, countriesIn, backwardIn, errors);

            // Write the output structure if there wasn't an error.
            errors.throwIfError("Errors accumulated");
            writeOutputData(outputData, tzLookupXmlFileOut, timeZoneIdsFileOut, errors);
            return true;
        } catch (HaltExecutionException e) {
            logError("Stopping due to fatal condition", e);
            return false;
        } finally {
            // Report all warnings / errors
            if (!errors.isEmpty()) {
                logInfo("Issues:\n" + errors.asString());
            }
        }
    }

    private Map<String, List<String>> parseZoneTabFile(String zoneTabFile, Errors errors)
            throws HaltExecutionException {
        errors.pushScope("Parsing " + zoneTabFile);
        try {
            ZoneTabFile zoneTabIn;
            zoneTabIn = ZoneTabFile.parse(zoneTabFile);
            return ZoneTabFile.createCountryToOlsonIdsMap(zoneTabIn);
        } catch (ParseException | IOException e) {
            throw errors.addFatalAndHalt("Unable to parse " + zoneTabFile, e);
        } finally {
            errors.popScope();
        }
    }

    /**
     * Load the backward file and return the links contained within. This is used as the source of
     * equivalent time zone IDs.
     */
    private static BackwardFile parseAndValidateBackwardFile(String backwardFile, Errors errors) {
        errors.pushScope("Parsing " + backwardFile);
        try {
            BackwardFile backward = BackwardFile.parse(backwardFile);

            // Validate the links.
            Map<String, String> zoneIdLinks = backward.getLinks();
            zoneIdLinks.forEach(
                    (k, v) -> {
                        if (invalidTimeZoneId(k)) {
                            errors.addError("Bad 'from' link: " + k + "->" + v);
                        }
                        if (invalidTimeZoneId(v)) {
                            errors.addError("Bad 'to' link: " + k + "->" + v);
                        }
                    });
            return backward;
        } catch (ParseException | IOException e) {
            errors.addError("Unable to parse " + backwardFile, e);
            return null;
        } finally {
            errors.popScope();
        }
    }

    private static CountryZonesFile.CountryZones parseAndValidateCountryZones(
            String countryZonesFile, Errors errors) throws HaltExecutionException {
        errors.pushScope("Parsing " + countryZonesFile);
        try {
            CountryZonesFile.CountryZones countryZonesIn;
            countryZonesIn = CountryZonesFileSupport.parseCountryZonesTextFile(countryZonesFile);
            return countryZonesIn;
        } catch (ParseException | IOException e) {
            throw errors.addFatalAndHalt("Unable to parse " + countryZonesFile, e);
        } finally {
            errors.popScope();
        }
    }

    private static void writeOutputData(OutputData outputData,
            String tzLookupXmlFileName, String timeZoneIdsFileName, Errors errors)
            throws HaltExecutionException {
        errors.pushScope("write " + tzLookupXmlFileName);
        try {
            // Write out the file used on device.
            logInfo("Writing " + tzLookupXmlFileName);

            TzLookupFile.TimeZones timeZonesOut = outputData.getTzLookupTimeZones();
            TzLookupFile.write(timeZonesOut, tzLookupXmlFileName);
        } catch (IOException | XMLStreamException e) {
            errors.addFatalAndHalt("Unable to write " + tzLookupXmlFileName, e);
        } finally {
            errors.popScope();
        }

        errors.pushScope("write " + timeZoneIdsFileName);
        try {
            // Write out the tz IDs file used during later stages of the pipeline.
            logInfo("Writing " + timeZoneIdsFileName);

            TimeZoneIds timeZoneIds = outputData.getTimeZoneIds();
            timeZoneIds.store(new File(timeZoneIdsFileName));
        } catch (IOException e) {
            errors.addFatalAndHalt("Unable to write " + timeZoneIdsFileName, e);
        } finally {
            errors.popScope();
        }
    }

    private static OutputData createOutputData(String inputIanaVersion,
            Map<String, List<String>> zoneTabMapping, List<CountryZonesFile.Country> countriesIn,
            BackwardFile backwardIn, Errors errors) throws HaltExecutionException {

        // Start constructing the output structure.
        TzLookupFile.TimeZones timeZonesOut = new TzLookupFile.TimeZones(inputIanaVersion);
        TzLookupFile.CountryZones tzLookupCountryZones = new TzLookupFile.CountryZones();
        timeZonesOut.setCountryZones(tzLookupCountryZones);

        // The time use when sampling the offsets for a zone.
        final long offsetSampleTimeMillis = getSampleOffsetTimeMillisForData(inputIanaVersion);

        // The start time to use when working out whether a zone has used UTC.
        // We don't care about historical use of UTC (e.g. parts of Europe like France prior
        // to WW2) so we start looking at the beginning of "this year".
        long everUseUtcStartTimeMillis = getYearStartTimeMillisForData(inputIanaVersion);

        TzIdsProto.TimeZoneIds.Builder tzIdsBuilder = TzIdsProto.TimeZoneIds.newBuilder()
                .setIanaVersion(inputIanaVersion);

        // Process each Country.
        for (CountryZonesFile.Country countryIn : countriesIn) {
            String isoCode = countryIn.getIsoCode();
            List<String> zoneTabCountryTimeZoneIds = zoneTabMapping.get(isoCode.toUpperCase());
            if (zoneTabCountryTimeZoneIds == null) {
                errors.addError("Country=" + isoCode + " missing from zone.tab");
                // No point in continuing.
                continue;
            }

            CountryOutputData countryOutputData = processCountry(
                    offsetSampleTimeMillis, everUseUtcStartTimeMillis, countryIn,
                    zoneTabCountryTimeZoneIds, backwardIn, errors);
            if (countryOutputData == null) {
                // Continue processing countries if there are only errors.
                continue;
            }

            tzLookupCountryZones.addCountry(countryOutputData.getTzLookupCountry());
            tzIdsBuilder.addCountryMappings(countryOutputData.getTimeZoneIdsCountryMapping());
        }
        errors.throwIfError("One or more countries failed");
        TimeZoneIds timeZoneIds = new TimeZoneIds(tzIdsBuilder.build());
        return new OutputData(timeZonesOut, timeZoneIds);
    }

    private static CountryOutputData processCountry(long offsetSampleTimeMillis,
            long everUseUtcStartTimeMillis, CountryZonesFile.Country countryIn,
            List<String> zoneTabCountryTimeZoneIds, BackwardFile backwardIn,
            Errors errors) {
        String isoCode = countryIn.getIsoCode();
        errors.pushScope("country=" + isoCode);
        try {
            // Each Country must have >= 1 time zone.
            List<CountryZonesFile.TimeZoneMapping> timeZonesIn =
                    countryIn.getTimeZoneMappingsList();
            if (timeZonesIn.isEmpty()) {
                errors.addError("No time zones");
                // No point in continuing.
                return null;
            }

            List<String> countryTimeZoneIds = CountryZonesFileSupport.extractIds(timeZonesIn);

            // Look for duplicate time zone IDs.
            if (!Utils.allUnique(countryTimeZoneIds)) {
                errors.addError("country's zones=" + countryTimeZoneIds + " contains duplicates");
                // No point in continuing.
                return null;
            }

            // Each Country needs a default time zone ID (but we can guess in some cases).
            String defaultTimeZoneId = determineCountryDefaultZoneId(countryIn, errors);
            if (defaultTimeZoneId == null) {
                // No point in continuing.
                return null;
            }
            boolean defaultTimeZoneBoost =
                    determineCountryDefaultTimeZoneBoost(countryIn, errors);

            // Validate the default.
            if (!countryTimeZoneIds.contains(defaultTimeZoneId)) {
                errors.addError("defaultTimeZoneId=" + defaultTimeZoneId
                        + " is not one of the country's zones=" + countryTimeZoneIds);
                // No point in continuing.
                return null;
            }

            // Validate the other zone IDs.
            try {
                errors.pushScope("validate country zone ids");
                for (String countryTimeZoneId : countryTimeZoneIds) {
                    if (invalidTimeZoneId(countryTimeZoneId)) {
                        errors.addError("countryTimeZoneId=" + countryTimeZoneId
                                + " is not a valid zone ID");
                    }
                }
                if (errors.hasError()) {
                    // No point in continuing.
                    return null;
                }
            } finally {
                errors.popScope();
            }

            // Work out the hint for whether the country uses a zero offset from UTC.
            boolean everUsesUtc = anyZonesUseUtc(countryTimeZoneIds, everUseUtcStartTimeMillis);

            // Validate the country information against the equivalent information in zone.tab.
            errors.pushScope("zone.tab comparison");
            try {
                // Look for unexpected duplicate time zone IDs in zone.tab
                if (!Utils.allUnique(zoneTabCountryTimeZoneIds)) {
                    errors.addError("Duplicate time zone IDs found:" + zoneTabCountryTimeZoneIds);
                    // No point in continuing.
                    return null;
                }

                // Validate the IDs being used against the IANA data for the country. If it fails
                // the countryzones.txt needs to be updated with new IDs (or an alias can be added
                // if there's some reason to keep using the old ID).
                validateCountryZonesTzIdsAgainstIana(isoCode, zoneTabCountryTimeZoneIds,
                        timeZonesIn, backwardIn.getDirectLinks(), errors);
                if (errors.hasError()) {
                    // No point in continuing.
                    return null;
                }
            } finally {
                errors.popScope();
            }

            // Calculate countryZoneUsage.
            CountryZoneUsage countryZoneUsage = calculateCountryZoneUsage(countryIn, errors);
            if (countryZoneUsage == null) {
                // No point in continuing with this country.
                return null;
            }

            // Create the tzlookup country structure.
            TzLookupFile.Country countryOut = new TzLookupFile.Country(
                    isoCode, defaultTimeZoneId, defaultTimeZoneBoost, everUsesUtc);

            // Process each input time zone.
            for (CountryZonesFile.TimeZoneMapping timeZoneIn : timeZonesIn) {
                errors.pushScope(
                        "id=" + timeZoneIn.getId() + ", offset=" + timeZoneIn.getUtcOffset()
                                + ", shownInPicker=" + timeZoneIn.getShownInPicker());
                try {
                    String timeZoneInId = timeZoneIn.getId();

                    // The notUsedAfterInstant can be null if the zone is used until at least
                    // ZONE_CALCS_END_INSTANT. That's what we want.
                    Instant notUsedAfterInstant =
                            countryZoneUsage.getNotUsedAfterInstant(timeZoneInId);
                    String notUsedReplacementId =
                            countryZoneUsage.getNotUsedReplacementId(timeZoneInId);

                    // Validate the offset information in countryIn.
                    validateNonDstOffset(offsetSampleTimeMillis, countryIn, timeZoneIn, errors);

                    boolean shownInPicker = timeZoneIn.getShownInPicker();
                    if (!countryZoneUsage.hasEntry(timeZoneInId)) {
                        // This implies a programming error.
                        errors.addError("No entry in CountryZoneUsage for " + timeZoneInId);
                        return null;
                    }

                    // Find all the alternative zone IDs for the chosen zone ID.
                    List<String> alternativeZoneIds =
                            new ArrayList<>(backwardIn.getAllAlternativeIds(timeZoneInId));
                    Collections.sort(alternativeZoneIds);

                    // Add the id mapping and associated metadata.
                    TzLookupFile.TimeZoneMapping timeZoneIdOut = new TzLookupFile.TimeZoneMapping(
                            timeZoneInId, shownInPicker, notUsedAfterInstant, notUsedReplacementId,
                            alternativeZoneIds);
                    countryOut.addTimeZoneMapping(timeZoneIdOut);
                } finally {
                    errors.popScope();
                }
            }

            // CountryMapping contains only information that is available from Country so we can
            // currently build one from the other.
            TzIdsProto.CountryMapping countryMappingProto =
                    TzLookupFile.Country.createCountryMappingProto(countryOut);

            return new CountryOutputData(countryOut, countryMappingProto);
        } finally{
            // End of country processing.
            errors.popScope();
        }
    }

    private static void validateCountryZonesTzIdsAgainstIana(String isoCode,
            List<String> zoneTabCountryTimeZoneIds,
            List<CountryZonesFile.TimeZoneMapping> timeZoneMappings,
            Map<String, String> zoneIdLinks, Errors errors) {

        List<String> expectedIanaTimeZoneIds = new ArrayList<>();
        for (CountryZonesFile.TimeZoneMapping mapping : timeZoneMappings) {
            String timeZoneId = mapping.getId();
            String expectedIanaTimeZoneId;
            if (!mapping.hasAliasId()) {
                expectedIanaTimeZoneId = timeZoneId;
            } else {
                String aliasTimeZoneId = mapping.getAliasId();

                // Confirm the alias is valid.
                if (!aliasTimeZoneId.equals(zoneIdLinks.get(timeZoneId))) {
                    errors.addError(timeZoneId + " does not link to " + aliasTimeZoneId);
                    return;
                }
                expectedIanaTimeZoneId = aliasTimeZoneId;
            }
            expectedIanaTimeZoneIds.add(expectedIanaTimeZoneId);
        }

        if (!Utils.setEquals(zoneTabCountryTimeZoneIds, expectedIanaTimeZoneIds)) {
            errors.addError("IANA lists " + isoCode
                    + " as having zones: " + zoneTabCountryTimeZoneIds
                    + ", but countryzones has " + expectedIanaTimeZoneIds);
        }
    }

    /**
     * Determines the default zone ID for the country.
     */
    private static String determineCountryDefaultZoneId(
            CountryZonesFile.Country countryIn, Errors errors) {
        List<CountryZonesFile.TimeZoneMapping> timeZonesIn = countryIn.getTimeZoneMappingsList();
        String defaultTimeZoneId;
        if (countryIn.hasDefaultTimeZoneId()) {
            defaultTimeZoneId = countryIn.getDefaultTimeZoneId();
            if (invalidTimeZoneId(defaultTimeZoneId)) {
                errors.addError(
                        "Default time zone ID " + defaultTimeZoneId + " is not valid");
                // No point in continuing.
                return null;
            }
        } else {
            if (timeZonesIn.size() > 1) {
                errors.addError(
                        "To pick a default time zone there must be a single offset group");
                // No point in continuing.
                return null;
            }
            defaultTimeZoneId = timeZonesIn.get(0).getId();
        }
        return defaultTimeZoneId;
    }

    /**
     * Determines the defaultTimeZoneBoost value for the country.
     */
    private static boolean determineCountryDefaultTimeZoneBoost(
            CountryZonesFile.Country countryIn, Errors errors) {
        if (!countryIn.hasDefaultTimeZoneBoost()) {
            return false;
        }

        boolean defaultTimeZoneBoost = countryIn.getDefaultTimeZoneBoost();
        if (!countryIn.hasDefaultTimeZoneId() && defaultTimeZoneBoost) {
            errors.addError(
                    "defaultTimeZoneBoost is specified but defaultTimeZoneId is not explicit");
        }

        return defaultTimeZoneBoost;
    }

    /**
     * Returns true if any of the zones use UTC after the time specified.
     */
    private static boolean anyZonesUseUtc(List<String> timeZoneIds, long startTimeMillis) {
        for (String timeZoneId : timeZoneIds) {
            BasicTimeZone timeZone = (BasicTimeZone) TimeZone.getTimeZone(timeZoneId);
            TimeZoneRule[] rules = timeZone.getTimeZoneRules(startTimeMillis);
            for (TimeZoneRule rule : rules) {
                int utcOffset = rule.getRawOffset() + rule.getDSTSavings();
                if (utcOffset == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Returns a sample time related to the IANA version to enable any offset validation to be
     * repeatable (rather than depending on the current time when the tool is run).
     */
    private static long getSampleOffsetTimeMillisForData(String inputIanaVersion) {
        // Uses <year>/07/02 12:00:00 UTC, where year is taken from the IANA version + 1.
        // This is fairly arbitrary, but reflects the fact that we want a point in the future
        // WRT to the data, and once a year has been picked then half-way through seems about right.
        Calendar calendar = getYearStartForData(inputIanaVersion);
        calendar.set(calendar.get(Calendar.YEAR) + 1, Calendar.JULY, 2, 12, 0, 0);
        return calendar.getTimeInMillis();
    }

    /**
     * Returns the 1st Jan 00:00:00 UTC time on the year the IANA version relates to. Therefore
     * guaranteed to be before the data is ever used and can be treated as "the beginning of time"
     * (assuming derived information won't be used for historical calculations).
     */
    private static long getYearStartTimeMillisForData(String inputIanaVersion) {
        return getYearStartForData(inputIanaVersion).getTimeInMillis();
    }

    private static Calendar getYearStartForData(String inputIanaVersion) {
        String yearString = inputIanaVersion.substring(0, inputIanaVersion.length() - 1);
        int year = Integer.parseInt(yearString);
        Calendar calendar = new GregorianCalendar(TimeZone.GMT_ZONE);
        calendar.clear();
        calendar.set(year, Calendar.JANUARY, 1, 0, 0, 0);
        return calendar;
    }

    private static boolean invalidTimeZoneId(String timeZoneId) {
        TimeZone zone = TimeZone.getTimeZone(timeZoneId);
        return !(zone instanceof BasicTimeZone) || zone.getID().equals(TimeZone.UNKNOWN_ZONE_ID);
    }

    private static void validateNonDstOffset(long offsetSampleTimeMillis,
            CountryZonesFile.Country country, CountryZonesFile.TimeZoneMapping timeZoneIn,
            Errors errors) {
        String utcOffsetString = timeZoneIn.getUtcOffset();
        long utcOffsetMillis;
        try {
            utcOffsetMillis = Utils.parseUtcOffsetToMillis(utcOffsetString);
        } catch (ParseException e) {
            errors.addError("Bad offset string: " + utcOffsetString);
            return;
        }

        final long minimumGranularity = TimeUnit.MINUTES.toMillis(15);
        if (utcOffsetMillis % minimumGranularity != 0) {
            errors.addWarning(
                    "Unexpected granularity: not a multiple of 15 minutes: " + utcOffsetString);
        }

        String timeZoneIdIn = timeZoneIn.getId();
        if (invalidTimeZoneId(timeZoneIdIn)) {
            errors.addError("Time zone ID=" + timeZoneIdIn + " is not valid");
            return;
        }

        // Check the offset Android has matches what ICU thinks.
        TimeZone timeZone = TimeZone.getTimeZone(timeZoneIdIn);
        int[] offsets = new int[2];
        timeZone.getOffset(offsetSampleTimeMillis, false /* local */, offsets);
        int actualOffsetMillis = offsets[0];
        if (actualOffsetMillis != utcOffsetMillis) {
            errors.addError("Offset mismatch: You will want to confirm the ordering for "
                    + country.getIsoCode() + " still makes sense. Raw offset for "
                    + timeZoneIdIn + " is " + Utils.toUtcOffsetString(actualOffsetMillis)
                    + " and not " + Utils.toUtcOffsetString(utcOffsetMillis)
                    + " at " + Utils.formatUtc(offsetSampleTimeMillis));
        }
    }

    private static CountryZoneUsage calculateCountryZoneUsage(
            CountryZonesFile.Country countryIn, Errors errors) {
        errors.pushScope("Building zone tree");
        try {
            CountryZoneTree countryZoneTree = CountryZoneTree.create(
                    countryIn, ZONE_USAGE_CALCS_START, ZONE_USAGE_CALCS_END);
            List<String> countryIssues = countryZoneTree.validateNoPriorityClashes();
            if (!countryIssues.isEmpty()) {
                errors.addError("Issues validating country zone trees. Adjust priorities:");
                countryIssues.forEach(errors::addError);
                return null;
            }
            return countryZoneTree.calculateCountryZoneUsage(ZONE_USAGE_NOT_AFTER_CUT_OFF);
        } finally {
            errors.popScope();
        }
    }

    private static void logError(String msg) {
        System.err.println("E: " + msg);
    }

    private static void logError(String s, Throwable e) {
        logError(s);
        e.printStackTrace(System.err);
    }

    private static void logInfo(String msg) {
        System.err.println("I: " + msg);
    }

    private static class CountryOutputData {
        private final TzLookupFile.Country tzLookupCountry;
        private final TzIdsProto.CountryMapping timeZoneIdsCountryMapping;

        private CountryOutputData(TzLookupFile.Country tzLookupCountry,
                TzIdsProto.CountryMapping timeZoneIdsCountryMapping) {
            this.tzLookupCountry = Objects.requireNonNull(tzLookupCountry);
            this.timeZoneIdsCountryMapping = Objects.requireNonNull(timeZoneIdsCountryMapping);
        }

        private TzLookupFile.Country getTzLookupCountry() {
            return tzLookupCountry;
        }

        private TzIdsProto.CountryMapping getTimeZoneIdsCountryMapping() {
            return timeZoneIdsCountryMapping;
        }
    }

    private static class OutputData {

        private final TzLookupFile.TimeZones tzLookupTimeZones;
        private final TimeZoneIds timeZoneIds;

        private OutputData(TzLookupFile.TimeZones tzLookupTimeZones, TimeZoneIds timeZoneIds) {
            this.tzLookupTimeZones = Objects.requireNonNull(tzLookupTimeZones);
            this.timeZoneIds = Objects.requireNonNull(timeZoneIds);
        }

        private TzLookupFile.TimeZones getTzLookupTimeZones() {
            return tzLookupTimeZones;
        }

        private TimeZoneIds getTimeZoneIds() {
            return timeZoneIds;
        }
    }
}
