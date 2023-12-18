/*
 * Copyright (C) 2018 The Android Open Source Project
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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package test_package;
interface ITest {
  String GetName();
  void TestVoidReturn();
  oneway void TestOneway();
  int GiveMeMyCallingPid();
  int GiveMeMyCallingUid();
  oneway void CacheCallingInfoFromOneway();
  int GiveMeMyCallingPidFromOneway();
  int GiveMeMyCallingUidFromOneway();
  int RepeatInt(int value);
  long RepeatLong(long value);
  float RepeatFloat(float value);
  double RepeatDouble(double value);
  boolean RepeatBoolean(boolean value);
  char RepeatChar(char value);
  byte RepeatByte(byte value);
  test_package.ByteEnum RepeatByteEnum(test_package.ByteEnum value);
  test_package.IntEnum RepeatIntEnum(test_package.IntEnum value);
  test_package.LongEnum RepeatLongEnum(test_package.LongEnum value);
  IBinder RepeatBinder(IBinder value);
  @nullable IBinder RepeatNullableBinder(@nullable IBinder value);
  test_package.IEmpty RepeatInterface(test_package.IEmpty value);
  @nullable test_package.IEmpty RepeatNullableInterface(@nullable test_package.IEmpty value);
  ParcelFileDescriptor RepeatFd(in ParcelFileDescriptor fd);
  @nullable ParcelFileDescriptor RepeatNullableFd(in @nullable ParcelFileDescriptor fd);
  String RepeatString(String value);
  @nullable String RepeatNullableString(@nullable String value);
  test_package.RegularPolygon RepeatPolygon(in test_package.RegularPolygon value);
  @nullable test_package.RegularPolygon RepeatNullablePolygon(in @nullable test_package.RegularPolygon value);
  void RenamePolygon(inout test_package.RegularPolygon value, String newName);
  boolean[] RepeatBooleanArray(in boolean[] input, out boolean[] repeated);
  byte[] RepeatByteArray(in byte[] input, out byte[] repeated);
  char[] RepeatCharArray(in char[] input, out char[] repeated);
  int[] RepeatIntArray(in int[] input, out int[] repeated);
  long[] RepeatLongArray(in long[] input, out long[] repeated);
  float[] RepeatFloatArray(in float[] input, out float[] repeated);
  double[] RepeatDoubleArray(in double[] input, out double[] repeated);
  test_package.ByteEnum[] RepeatByteEnumArray(in test_package.ByteEnum[] input, out test_package.ByteEnum[] repeated);
  test_package.IntEnum[] RepeatIntEnumArray(in test_package.IntEnum[] input, out test_package.IntEnum[] repeated);
  test_package.LongEnum[] RepeatLongEnumArray(in test_package.LongEnum[] input, out test_package.LongEnum[] repeated);
  String[] RepeatStringArray(in String[] input, out String[] repeated);
  test_package.RegularPolygon[] RepeatRegularPolygonArray(in test_package.RegularPolygon[] input, out test_package.RegularPolygon[] repeated);
  ParcelFileDescriptor[] RepeatFdArray(in ParcelFileDescriptor[] input, out ParcelFileDescriptor[] repeated);
  List<String> Repeat2StringList(in List<String> input, out List<String> repeated);
  List<test_package.RegularPolygon> Repeat2RegularPolygonList(in List<test_package.RegularPolygon> input, out List<test_package.RegularPolygon> repeated);
  @nullable boolean[] RepeatNullableBooleanArray(in @nullable boolean[] input);
  @nullable byte[] RepeatNullableByteArray(in @nullable byte[] input);
  @nullable char[] RepeatNullableCharArray(in @nullable char[] input);
  @nullable int[] RepeatNullableIntArray(in @nullable int[] input);
  @nullable long[] RepeatNullableLongArray(in @nullable long[] input);
  @nullable float[] RepeatNullableFloatArray(in @nullable float[] input);
  @nullable double[] RepeatNullableDoubleArray(in @nullable double[] input);
  @nullable test_package.ByteEnum[] RepeatNullableByteEnumArray(in @nullable test_package.ByteEnum[] input);
  @nullable test_package.IntEnum[] RepeatNullableIntEnumArray(in @nullable test_package.IntEnum[] input);
  @nullable test_package.LongEnum[] RepeatNullableLongEnumArray(in @nullable test_package.LongEnum[] input);
  @nullable String[] RepeatNullableStringArray(in @nullable String[] input);
  @nullable String[] DoubleRepeatNullableStringArray(in @nullable String[] input, out @nullable String[] repeated);
  test_package.Foo repeatFoo(in test_package.Foo inFoo);
  void renameFoo(inout test_package.Foo foo, String name);
  void renameBar(inout test_package.Foo foo, String name);
  int getF(in test_package.Foo foo);
  test_package.GenericBar<int> repeatGenericBar(in test_package.GenericBar<int> bar);
  void RepeatExtendableParcelable(in test_package.ExtendableParcelable input, out test_package.ExtendableParcelable output);
  test_package.SimpleUnion RepeatSimpleUnion(in test_package.SimpleUnion u);
  IBinder getICompatTest();
  void RepeatExtendableParcelableWithoutExtension(in test_package.ExtendableParcelable input, out test_package.ExtendableParcelable output);
  const int kZero = 0;
  const int kOne = 1;
  const int kOnes = -1;
  const byte kByteOne = 1;
  const long kLongOnes = -1;
  const String kEmpty = "";
  const String kFoo = "foo";
}
