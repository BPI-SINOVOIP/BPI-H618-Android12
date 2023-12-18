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

import android.os.Environment;
import android.util.Log;

import com.android.server.wm.traces.common.layers.LayersTrace;

/**
 * Base class that encapsulates the logic for enabling the layers trace, parsing the
 * surface flinger trace. Extend this class to add validation for layers trace based
 * flicker conditions.
 */
public abstract class LayersFlickerRuleBase extends FlickerRuleBase {

    private static final String TAG = LayersFlickerRuleBase.class.getSimpleName();

    LayersFlickerRuleBase() {
        enableLayerTrace();
        Log.v(TAG, "Enabled the surface flinger trace.");
    }

    protected void validateFlickerConditions() {
        validateLayersFlickerConditions(getLayersTrace());
    }

    /**
     * Override this method to provide layers trace based flicker validations.
     *
     * @param layersTrace
     */
    protected abstract void validateLayersFlickerConditions(LayersTrace layersTrace);
}
