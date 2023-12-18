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

package android.gamemanager.cts;

import static com.google.common.truth.Truth.assertThat;

import android.app.GameManager;
import android.content.Context;
import android.util.Log;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.ShellIdentityUtils;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

public class GameManagerTest {
    private static final String TAG = "GameManagerTest";

    private GameManagerCtsActivity mActivity;
    private Context mContext;
    private GameManager mGameManager;

    @Rule
    public ActivityScenarioRule<GameManagerCtsActivity> mActivityRule =
            new ActivityScenarioRule<>(GameManagerCtsActivity.class);

    @Before
    public void setUp() {
        mActivityRule.getScenario().onActivity(activity -> {
            mActivity = activity;
        });

        mContext = getInstrumentation().getContext();
        mGameManager = mContext.getSystemService(GameManager.class);
    }

    /**
     * Test that GameManager::getGameMode() returns the correct value when forcing the Game Mode to
     * GAME_MODE_UNSUPPORTED.
     */
    @Test
    public void testGetGameModeUnsupported() {
        ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mGameManager,
                (gameManager) -> gameManager.setGameMode(mActivity.getPackageName(),
                        GameManager.GAME_MODE_UNSUPPORTED));

        int gameMode = mActivity.getGameMode();

        Assert.assertEquals("Game Manager returned incorrect value.",
                GameManager.GAME_MODE_UNSUPPORTED, gameMode);
    }

    /**
     * Test that GameManager::getGameMode() returns the correct value when forcing the Game Mode to
     * GAME_MODE_STANDARD.
     */
    @Test
    public void testGetGameModeStandard() {
        ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mGameManager,
                (gameManager) -> gameManager.setGameMode(mActivity.getPackageName(),
                        GameManager.GAME_MODE_STANDARD));

        int gameMode = mActivity.getGameMode();

        Assert.assertEquals("Game Manager returned incorrect value.",
                GameManager.GAME_MODE_STANDARD, gameMode);
    }

    /**
     * Test that GameManager::getGameMode() returns the correct value when forcing the Game Mode to
     * GAME_MODE_PERFORMANCE.
     */
    @Test
    public void testGetGameModePerformance() {
        ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mGameManager,
                (gameManager) -> gameManager.setGameMode(mActivity.getPackageName(),
                        GameManager.GAME_MODE_PERFORMANCE));

        int gameMode = mActivity.getGameMode();

        Assert.assertEquals("Game Manager returned incorrect value.",
                GameManager.GAME_MODE_PERFORMANCE, gameMode);
    }

    /**
     * Test that GameManager::getGameMode() returns the correct value when forcing the Game Mode to
     * GAME_MODE_BATTERY.
     */
    @Test
    public void testGetGameModeBattery() {
        ShellIdentityUtils.invokeMethodWithShellPermissionsNoReturn(mGameManager,
                (gameManager) -> gameManager.setGameMode(mActivity.getPackageName(),
                        GameManager.GAME_MODE_BATTERY));

        int gameMode = mActivity.getGameMode();

        Assert.assertEquals("Game Manager returned incorrect value.",
                GameManager.GAME_MODE_BATTERY, gameMode);
    }
}
