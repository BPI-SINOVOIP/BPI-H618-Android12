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
package org.hyphonate.megaaudio.common;

public class BuilderBase {
    //TODO exlain the structure of these constants
    // API Types - enumerated in high nibble
    public static final int TYPE_MASK = 0xF000;
    public static final int TYPE_UNDEFINED = 0xF000;
    public static final int TYPE_NONE = 0x0000;
    public static final int TYPE_JAVA = 0x1000;
    public static final int TYPE_OBOE = 0x2000;

    // API subtypes - enumerated in low nibble
    public static final int SUB_TYPE_MASK = 0x0000F;
    public static final int SUB_TYPE_OBOE_DEFAULT = 0x0000;
    public static final int SUB_TYPE_OBOE_AAUDIO = 0x0001;
    public static final int SUB_TYPE_OBOE_OPENSL_ES = 0x0002;

    protected int mType = TYPE_UNDEFINED;
}
