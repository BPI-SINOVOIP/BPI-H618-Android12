/*
 * Copyright 2020 The Android Open Source Project
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
package org.hyphonate.megaaudio.player;

import android.media.AudioDeviceInfo;

import org.hyphonate.megaaudio.common.BuilderBase;

public class PlayerBuilder extends BuilderBase {
    private AudioSourceProvider mSourceProvider;

    public PlayerBuilder() {

    }

    public PlayerBuilder setPlayerType(int playerType) {
        mType = playerType;
        return this;
    }

    public PlayerBuilder setSourceProvider(AudioSourceProvider sourceProvider) {
        mSourceProvider = sourceProvider;
        return this;
    }

    public Player build() throws BadStateException {
        if (mSourceProvider == null) {
            throw new BadStateException();
        }

        Player player = null;
        int playerType = mType & TYPE_MASK;
        switch (playerType) {
            case TYPE_NONE:
                // NOP
                break;

            case TYPE_JAVA:
                player = new JavaPlayer(mSourceProvider);
                break;

            case TYPE_OBOE:{
                int playerSubType = mType & SUB_TYPE_MASK;
                player = new OboePlayer(mSourceProvider, playerSubType);
            }
            break;

            default:
                throw new BadStateException();
        }

        return player;
    }

    public class BadStateException extends Throwable {

    }
}
