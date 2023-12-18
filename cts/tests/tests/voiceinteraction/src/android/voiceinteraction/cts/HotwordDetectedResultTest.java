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

package android.voiceinteraction.cts;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.media.AudioRecord;
import android.media.MediaSyncEvent;
import android.os.Parcel;
import android.os.PersistableBundle;
import android.service.voice.HotwordDetectedResult;
import android.voiceinteraction.common.Utils;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class HotwordDetectedResultTest {

    @Test
    public void testHotwordDetectedResult_getMaxBundleSize() throws Exception {
        assertThat(HotwordDetectedResult.getMaxBundleSize() >= 0).isTrue();
    }

    @Test
    public void testHotwordDetectedResult_totalSize() throws Exception {
        final int bitsForConfidenceLevel = Utils.bitCount(
                HotwordDetectedResult.CONFIDENCE_LEVEL_VERY_HIGH);
        final int bitsForHotwordOffsetMillis = Utils.bitCount(Utils.LIMIT_HOTWORD_OFFSET_MAX_VALUE);
        final int bitsForHotwordDurationMillis = Utils.bitCount(
                AudioRecord.getMaxSharedAudioHistoryMillis());
        final int bitsForAudioChannel = Utils.bitCount(Utils.LIMIT_AUDIO_CHANNEL_MAX_VALUE);
        final int bitsForHotwordDetectionPersonalized = 1;
        final int bitsForScore = Utils.bitCount(HotwordDetectedResult.getMaxScore());
        final int bitsForPersonalizedScore = Utils.bitCount(HotwordDetectedResult.getMaxScore());
        final int bitsForHotwordPhraseId = Utils.bitCount(
                HotwordDetectedResult.getMaxHotwordPhraseId());

        final int totalSize =
                bitsForConfidenceLevel + bitsForHotwordOffsetMillis + bitsForHotwordDurationMillis
                        + bitsForAudioChannel + bitsForHotwordDetectionPersonalized + bitsForScore
                        + bitsForPersonalizedScore + bitsForHotwordPhraseId
                        + HotwordDetectedResult.getMaxBundleSize() * Byte.SIZE;

        assertThat(totalSize <= Utils.MAX_HOTWORD_DETECTED_RESULT_SIZE * Byte.SIZE).isTrue();
    }

    @Test
    public void testHotwordDetectedResult_bundleExceedMaxBundleSize() throws Exception {
        final PersistableBundle persistableBundle = new PersistableBundle();
        int key = 0;
        do {
            persistableBundle.putInt(Integer.toString(key), 0);
            key++;
        } while (Utils.getParcelableSize(persistableBundle)
                <= HotwordDetectedResult.getMaxBundleSize());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setExtras(persistableBundle).build());
    }

    @Test
    public void testHotwordDetectedResult_getMaxHotwordPhraseId() throws Exception {
        assertThat(HotwordDetectedResult.getMaxHotwordPhraseId() >= 63).isTrue();
    }

    @Test
    public void testHotwordDetectedResult_setInvalidHotwordPhraseId() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordPhraseId(-1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordPhraseId(
                        HotwordDetectedResult.getMaxHotwordPhraseId() + 1).build());
    }

    @Test
    public void testHotwordDetectedResult_getMaxScore() throws Exception {
        assertThat(HotwordDetectedResult.getMaxScore() >= 255).isTrue();
    }

    @Test
    public void testHotwordDetectedResult_setInvalidScore() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setScore(-1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setScore(
                        HotwordDetectedResult.getMaxScore() + 1).build());
    }

    @Test
    public void testHotwordDetectedResult_setInvalidPersonalizedScore() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setPersonalizedScore(-1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setPersonalizedScore(
                        HotwordDetectedResult.getMaxScore() + 1).build());
    }

    @Test
    public void testHotwordDetectedResult_setInvalidHotwordDurationMillis() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordDurationMillis(-1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordDurationMillis(
                        (int) AudioRecord.getMaxSharedAudioHistoryMillis() + 1).build());
    }

    @Test
    public void testHotwordDetectedResult_setInvalidHotwordOffsetMillis() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordOffsetMillis(
                        HotwordDetectedResult.HOTWORD_OFFSET_UNSET - 1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setHotwordOffsetMillis(
                        Utils.LIMIT_HOTWORD_OFFSET_MAX_VALUE + 1).build());
    }

    @Test
    public void testHotwordDetectedResult_setInvalidAudioChannel() throws Exception {
        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setAudioChannel(
                        HotwordDetectedResult.AUDIO_CHANNEL_UNSET - 1).build());

        assertThrows(IllegalArgumentException.class,
                () -> new HotwordDetectedResult.Builder().setAudioChannel(
                        Utils.LIMIT_AUDIO_CHANNEL_MAX_VALUE + 1).build());
    }

    @Test
    public void testHotwordDetectedResultBuilder() throws Exception {
        final HotwordDetectedResult hotwordDetectedResult =
                buildHotwordDetectedResult(
                        HotwordDetectedResult.CONFIDENCE_LEVEL_LOW,
                        MediaSyncEvent.createEvent(MediaSyncEvent.SYNC_EVENT_PRESENTATION_COMPLETE),
                        /* hotwordOffsetMillis= */ 100,
                        /* hotwordDurationMillis= */ 1000,
                        /* audioChannel= */ 1,
                        /* hotwordDetectionPersonalized= */ true,
                        /* score= */ 100,
                        /* personalizedScore= */ 100,
                        /* hotwordPhraseId= */ 1,
                        new PersistableBundle());

        assertHotwordDetectedResult(hotwordDetectedResult);
    }

    @Test
    public void testHotwordDetectedResultParcelizeDeparcelize() throws Exception {
        final HotwordDetectedResult hotwordDetectedResult =
                buildHotwordDetectedResult(
                        HotwordDetectedResult.CONFIDENCE_LEVEL_LOW,
                        MediaSyncEvent.createEvent(MediaSyncEvent.SYNC_EVENT_PRESENTATION_COMPLETE),
                        /* hotwordOffsetMillis= */ 100,
                        /* hotwordDurationMillis= */ 1000,
                        /* audioChannel= */ 1,
                        /* hotwordDetectionPersonalized= */ true,
                        /* score= */ 100,
                        /* personalizedScore= */ 100,
                        /* hotwordPhraseId= */ 1,
                        new PersistableBundle());

        final Parcel p = Parcel.obtain();
        hotwordDetectedResult.writeToParcel(p, 0);
        p.setDataPosition(0);

        final HotwordDetectedResult targetHotwordDetectedResult =
                HotwordDetectedResult.CREATOR.createFromParcel(p);
        p.recycle();

        assertHotwordDetectedResult(targetHotwordDetectedResult);
    }

    private HotwordDetectedResult buildHotwordDetectedResult(
            int confidenceLevel,
            MediaSyncEvent mediaSyncEvent,
            int hotwordOffsetMillis,
            int hotwordDurationMillis,
            int audioChannel,
            boolean hotwordDetectionPersonalized,
            int score,
            int personalizedScore,
            int hotwordPhraseId,
            PersistableBundle extras) {
        return new HotwordDetectedResult.Builder()
                .setConfidenceLevel(confidenceLevel)
                .setMediaSyncEvent(mediaSyncEvent)
                .setHotwordOffsetMillis(hotwordOffsetMillis)
                .setHotwordDurationMillis(hotwordDurationMillis)
                .setAudioChannel(audioChannel)
                .setHotwordDetectionPersonalized(hotwordDetectionPersonalized)
                .setScore(score)
                .setPersonalizedScore(personalizedScore)
                .setHotwordPhraseId(hotwordPhraseId)
                .setExtras(extras)
                .build();
    }

    private void assertHotwordDetectedResult(HotwordDetectedResult hotwordDetectedResult) {
        assertThat(hotwordDetectedResult.getConfidenceLevel()).isEqualTo(
                HotwordDetectedResult.CONFIDENCE_LEVEL_LOW);
        assertThat(hotwordDetectedResult.getMediaSyncEvent()).isNotNull();
        assertThat(hotwordDetectedResult.getHotwordOffsetMillis()).isEqualTo(100);
        assertThat(hotwordDetectedResult.getHotwordDurationMillis()).isEqualTo(1000);
        assertThat(hotwordDetectedResult.getAudioChannel()).isEqualTo(1);
        assertThat(hotwordDetectedResult.isHotwordDetectionPersonalized()).isTrue();
        assertThat(hotwordDetectedResult.getScore()).isEqualTo(100);
        assertThat(hotwordDetectedResult.getPersonalizedScore()).isEqualTo(100);
        assertThat(hotwordDetectedResult.getHotwordPhraseId()).isEqualTo(1);
        assertThat(hotwordDetectedResult.getExtras()).isNotNull();
    }
}
