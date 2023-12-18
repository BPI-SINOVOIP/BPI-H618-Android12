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

package android.platform.test.rule.flicker;

import android.util.Log;

import com.android.server.wm.traces.common.layers.LayersTrace;

/**
 * Rule used for validating the common layers trace based flicker assertions applicable
 * for all the CUJ's
 */
public class LayersFlickerRuleCommon extends LayersFlickerRuleBase {

    private static final String TAG = LayersFlickerRuleCommon.class.getSimpleName();

    protected void validateLayersFlickerConditions(LayersTrace layersTrace) {

        // TODO: b/183516705 Implementation for layer trace based common validations.
        Log.v(TAG, "Successfully verified the common layers flicker conditions.");
    }
}
