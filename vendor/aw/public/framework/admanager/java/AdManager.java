/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.softwinner;

import java.io.IOException;
import java.lang.String;
import android.util.Log;

public class AdManager
{
	public static final String TAG = "AdManager";

    private AdManager()
    {
    }

	static {
		System.loadLibrary("admanager_jni");
		nativeInit();
	}

	private static native void nativeInit();
	private static native int nativeSetBootAnimation(String pathName);
	private static native int nativeSetBootLogo(String pathName);

	public static int setBootAnimation(String path){
		Log.d(TAG,"setBootanimation : " + path);
        return nativeSetBootAnimation(path);
    }

	public static int setBootLogo(String path){
		Log.d(TAG,"setBootlogo : " + path);
        return nativeSetBootLogo(path);
    }
}

