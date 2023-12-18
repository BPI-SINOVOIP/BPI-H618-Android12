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

package android.mediapc.cts;

import android.util.Pair;

import androidx.test.filters.LargeTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import static org.junit.Assert.assertTrue;

/**
 * The following test class calculates the maximum number of concurrent decode sessions that it can
 * support by the two hardware (mime - decoder) pair calculated via the
 * CodecCapabilities.getMaxSupportedInstances() and
 * VideoCapabilities.getSupportedPerformancePoints() methods. Splits the maximum supported instances
 * between the two pairs and ensures that all the supported sessions succeed in decoding
 * with meeting the expected frame rate.
 */
@RunWith(Parameterized.class)
public class MultiDecoderPairPerfTest extends MultiCodecPerfTestBase {
    private static final String LOG_TAG = MultiDecoderPairPerfTest.class.getSimpleName();

    private final Pair<String, String> mFirstPair;
    private final Pair<String, String> mSecondPair;

    public MultiDecoderPairPerfTest(Pair<String, String> firstPair, Pair<String, String> secondPair,
            boolean isAsync) {
        super(null, null, isAsync);
        mFirstPair = firstPair;
        mSecondPair = secondPair;
    }

    // Returns the list of params with two hardware (mime - decoder) pairs in both
    // sync and async modes.
    // Parameters {0}_{1}_{2} -- Pair(Mime DecoderName)_Pair(Mime DecoderName)_isAsync
    @Parameterized.Parameters(name = "{index}({0}_{1}_{2})")
    public static Collection<Object[]> inputParams() {
        final List<Object[]> argsList = new ArrayList<>();
        ArrayList<Pair<String, String>> mimeTypeDecoderPairs = new ArrayList<>();
        for (String mime : mMimeList) {
            ArrayList<String> listOfDecoders = getHardwareCodecsFor720p(mime, false);
            for (String decoder : listOfDecoders) {
                mimeTypeDecoderPairs.add(Pair.create(mime, decoder));
            }
        }
        for (int i = 0; i < mimeTypeDecoderPairs.size(); i++) {
            for (int j = i + 1; j < mimeTypeDecoderPairs.size(); j++) {
                Pair<String, String> pair1 = mimeTypeDecoderPairs.get(i);
                Pair<String, String> pair2 = mimeTypeDecoderPairs.get(j);
                for (boolean isAsync : boolStates) {
                    argsList.add(new Object[]{pair1, pair2, isAsync});
                }
            }
        }
        return argsList;
    }

    /**
     * This test calculates the number of 720p 30 fps decoder instances that the given two
     * (mime - decoder) pairs can support. Assigns the same number of instances to the two pairs
     * (if max instances are even), or one more to one pair (if odd) and ensures that all the
     * concurrent sessions succeed in decoding with meeting the expected frame rate.
     */
    @LargeTest
    @Test(timeout = CodecTestBase.PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void test720p() throws Exception {
        ArrayList<Pair<String, String>> mimeDecoderPairs = new ArrayList<>();
        mimeDecoderPairs.add(mFirstPair);
        mimeDecoderPairs.add(mSecondPair);
        int maxInstances = checkAndGetMaxSupportedInstancesFor720p(mimeDecoderPairs);
        int secondPairInstances = maxInstances / 2;
        int firstPairInstances = maxInstances - secondPairInstances;
        ExecutorService pool = Executors.newFixedThreadPool(maxInstances);
        List<Decode> testList = new ArrayList<>();
        for (int i = 0; i < firstPairInstances; i++) {
            testList.add(new Decode(mFirstPair.first, mTestFiles.get(mFirstPair.first),
                    mFirstPair.second, mIsAsync));
        }
        for (int i = 0; i < secondPairInstances; i++) {
            testList.add(new Decode(mSecondPair.first, mTestFiles.get(mSecondPair.first),
                    mSecondPair.second, mIsAsync));
        }
        List<Future<Double>> resultList = pool.invokeAll(testList);
        double achievedFrameRate = 0.0;
        for (Future<Double> result : resultList) {
            achievedFrameRate += result.get();
        }
        assertTrue("Unable to achieve the maxFrameRate supported. act/exp: " + achievedFrameRate
                + "/" + mMaxFrameRate + " for " + maxInstances + " instances.",
                achievedFrameRate >= mMaxFrameRate);
    }
}
