/**
 * Copyright (c) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.softwinner;

interface IEinkMode {
    const int INIT     = 0x00;
    const int DU       = 0x01;
    const int GC16     = 0x02;
    const int GC4      = 0x03;
    const int A2       = 0x04;
    const int GL16     = 0x05;
    const int GLR16    = 0x06;
    const int GLD16    = 0x07;
    const int GU16     = 0x08;
	const int CLEAR    = 0x09;
	const int GC4L     = 0x0a;
	const int GCC16    = 0x0b;
	const int RECT     = 0x0c;
}
