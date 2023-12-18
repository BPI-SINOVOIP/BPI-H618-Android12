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

package com.android.cts.verifier.audio;

import java.io.IOException;
import java.util.ArrayList;

import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiReceiver;
import android.os.Bundle;
import android.util.Log;

import com.android.cts.verifier.R;

import com.android.cts.verifier.audio.midilib.MidiIODevice;

/*
 * A note about the USB MIDI device.
 * Any USB MIDI peripheral with standard female DIN jacks can be used. A standard MIDI cable
 * plugged into both input and output is required for the USB Loopback Test. A Bluetooth MIDI
 * device like the Yamaha MD-BT01 plugged into both input and output is required for the
 * Bluetooth Loopback test.
 */

/*
 *  A note about the "virtual MIDI" device...
 * See file MidiEchoService for implementation of the echo server itself.
 * This service is started by the main manifest file (AndroidManifest.xml).
 */

/*
 * A note about Bluetooth MIDI devices...
 * Any Bluetooth MIDI device needs to be paired with the DUT with the "MIDI+BTLE" application
 * available in the Play Store:
 * (https://play.google.com/store/apps/details?id=com.mobileer.example.midibtlepairing).
 */

/**
 * CTS Verifier Activity for MIDI test
 */
public class MidiJavaTestActivity extends MidiTestActivityBase {
    private static final String TAG = "MidiJavaTestActivity";
    private static final boolean DEBUG = false;

    public MidiJavaTestActivity() {
        super();
        initTestModules(new JavaMidiTestModule(MidiDeviceInfo.TYPE_USB),
                new JavaMidiTestModule(MidiDeviceInfo.TYPE_VIRTUAL),
                new BTMidiTestModule());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (DEBUG) {
            Log.i(TAG, "---- onCreate()");
        }

        setContentView(R.layout.midi_activity);

        super.onCreate(savedInstanceState);

        startMidiEchoServer();
        scanMidiDevices();

        connectDeviceListener();
    }

    //
    // MIDI Messages
    //
    // channel-oriented message (Commands)
    public static final byte MIDICMD_NOTEON = 9;
    public static final byte MIDICMD_NOTEOFF = 8;
    public static final byte MIDICMD_POLYPRESS = 10;
    public static final byte MIDICMD_CONTROL = 11;
    public static final byte MIDICMD_PROGRAMCHANGE = 12;
    public static final byte MIDICMD_CHANNELPRESS = 13;
    public static final byte MIDICMD_PITCHWHEEL = 14;
    public static final byte MIDICMD_SYSEX = (byte)0xF0;
    public static final byte MIDICMD_EOSYSEX = (byte)0xF7; // (byte)0b11110111;    // 0xF7

    private class TestMessage {
        public byte[]   mMsgBytes;

        public boolean matches(byte[] msg, int offset, int count) {
            // Length
            if (DEBUG) {
                Log.i(TAG, "  count [" + count + " : " + mMsgBytes.length + "]");
            }
            if (count != mMsgBytes.length) {
                return false;
            }

            // Data
            for(int index = 0; index < count; index++) {
                if (DEBUG) {
                    Log.i(TAG, "  [" + msg[offset + index] + " : " + mMsgBytes[index] + "]");
                }
                if (msg[offset + index] != mMsgBytes[index]) {
                    return false;
                }
            }
            return true;
        }
    } /* class TestMessage */

    private static byte makeMIDICmd(int cmd, int channel) {
        return (byte)((cmd << 4) | (channel & 0x0F));
    }

    //
    // Logging Utility
    //
    static void logByteArray(String prefix, byte[] value, int offset, int count) {
        StringBuilder builder = new StringBuilder(prefix);
        for (int i = 0; i < count; i++) {
            builder.append(String.format("0x%02X", value[offset + i]));
            if (i != value.length - 1) {
                builder.append(", ");
            }
        }
        Log.d(TAG, builder.toString());
    }

    static void logByteArray(String prefix, ArrayList<Byte> value, int offset) {
        StringBuilder builder = new StringBuilder(prefix);
        for (int i = 0; i < value.size(); i++) {
            builder.append(String.format("0x%02X", value.get(offset + i)));
            if (i != value.size() - 1) {
                builder.append(", ");
            }
        }
        Log.d(TAG, builder.toString());
    }

    /**
     * A class to control and represent the state of a given test.
     * It hold the data needed for IO, and the logic for sending, receiving and matching
     * the MIDI data stream.
     */
    private class JavaMidiTestModule extends MidiTestModule {
        private static final String TAG = "JavaMidiTestModule";

        protected boolean mTestMismatched;

        // Test Data
        // - The set of messages to send
        private TestMessage[] mTestMessages;

        // - The stream of message data to walk through when MIDI data is received.
        // NOTE: To work on USB Audio Peripherals that drop the first message
        // (AudioBoxUSB), have 2 streams to match against, one with the "warm-up"
        // message in tact ("Nominal") and one where it is absent.
        private ArrayList<Byte> mMatchStream = new ArrayList<Byte>();

        private int mReceiveStreamPos;
        private static final int MESSAGE_MAX_BYTES = 1024;

        // Some MIDI interfaces have been know to consistently drop the first message
        // Send one to throw away. If it shows up, ignore it. If it doesn't then
        // there is nothing there to ignore and the remainder should be legitimate.
        // Use the MIDI CONTROL command to identify this "warm-up" message
        private byte[] mWarmUpMsg = {makeMIDICmd(MIDICMD_CONTROL, 0), 0, 0};

        public JavaMidiTestModule(int deviceType) {
            super(deviceType);
            setupTestMessages();
        }

        @Override
        void startLoopbackTest(int testId) {
            synchronized (mTestLock) {
                mTestRunning = true;
                enableTestButtons(false);
            }

            if (DEBUG) {
                Log.i(TAG, "---- startLoopbackTest()");
            }

            mTestStatus = TESTSTATUS_NOTRUN;
            mTestMismatched = false;
            mReceiveStreamPos = 0;

            mRunningTestID = testId;

            // These might be left open due to a failing, previous test
            // so just to be sure...
            closePorts();

            if (mIODevice.mSendDevInfo != null) {
                mMidiManager.openDevice(mIODevice.mSendDevInfo, new TestModuleOpenListener(), null);
            }

            startTimeoutHandler();
        }

        protected void openPorts(MidiDevice device) {
            mIODevice.openPorts(device, new MidiMatchingReceiver());
        }

        protected void closePorts() {
            mIODevice.closePorts();
        }

        @Override
        boolean hasTestPassed() {
            return mTestStatus == TESTSTATUS_PASSED;
        }

        // A little explanation here... It seems reasonable to send complete MIDI messages, i.e.
        // as a set of discrete pakages.
        // However the looped-back data may not be delivered in message-size packets, so it makes more
        // sense to look at that as a stream of bytes.
        // So we build a set of messages to send, and then create the equivalent stream of bytes
        // from that to match against when received back in from the looped-back device.
        private void setupTestMessages() {
            if (DEBUG) {
                Log.i(TAG, "setupTestMessages()");
            }
            int NUM_TEST_MESSAGES = 3;
            mTestMessages = new TestMessage[NUM_TEST_MESSAGES];

            //TODO - Investgate using ByteArrayOutputStream for these data streams.

            //
            // Set up any set of messages you want
            // Except for the command IDs, the data values are purely arbitrary and meaningless
            // outside of being matched.
            // KeyDown
            mTestMessages[0] = new TestMessage();
            mTestMessages[0].mMsgBytes = new byte[]{makeMIDICmd(MIDICMD_NOTEON, 0), 64, 12};

            // KeyUp
            mTestMessages[1] = new TestMessage();
            mTestMessages[1].mMsgBytes = new byte[]{makeMIDICmd(MIDICMD_NOTEOFF, 0), 73, 65};

            // SysEx
            // NOTE: A sysex on the MT-BT01 seems to top out at sometimes as low as 40 bytes.
            // It is not clear, but needs more research. For now choose a conservative size.
            int sysExSize = 32;
            byte[] sysExMsg = new byte[sysExSize];
            sysExMsg[0] = MIDICMD_SYSEX;
            for(int index = 1; index < sysExSize-1; index++) {
                sysExMsg[index] = (byte)index;
            }
            sysExMsg[sysExSize-1] = (byte) MIDICMD_EOSYSEX;
            mTestMessages[2] = new TestMessage();
            mTestMessages[2].mMsgBytes = sysExMsg;

            //
            // Now build the stream to match against
            //
            mMatchStream.clear();
            for(int byteIndex = 0; byteIndex < mWarmUpMsg.length; byteIndex++) {
                mMatchStream.add(mWarmUpMsg[byteIndex]);
            }
            for (int msgIndex = 0; msgIndex < mTestMessages.length; msgIndex++) {
                for(int byteIndex = 0; byteIndex < mTestMessages[msgIndex].mMsgBytes.length; byteIndex++) {
                    mMatchStream.add(mTestMessages[msgIndex].mMsgBytes[byteIndex]);
                }
            }

            mReceiveStreamPos = 0;

            if (DEBUG) {
                logByteArray("mMatchStream: ", mMatchStream, 0);
            }
        }

        /**
         * Compares the supplied bytes against the sent message stream at the current position
         * and advances the stream position.
         */
        private boolean matchStream(byte[] bytes, int offset, int count) {
            if (DEBUG) {
                Log.i(TAG, "---- matchStream() offset:" + offset + " count:" + count);
            }
            // a little bit of checking here...
            if (count < 0) {
                Log.e(TAG, "Negative Byte Count in MidiActivity::matchStream()");
                return false;
            }

            if (count > MESSAGE_MAX_BYTES) {
                Log.e(TAG, "Too Large Byte Count (" + count + ") in MidiActivity::matchStream()");
                return false;
            }

            boolean matches = true;

            for (int index = 0; index < count; index++) {
                // Avoid a buffer overrun. Still don't understand why it happens
                if (mReceiveStreamPos >= mMatchStream.size()) {
                    // report an error here
                    Log.d(TAG, "matchStream buffer overrun @" + index +
                            " of " + mMatchStream.size());
                    // Dump the bufer here
                    logByteArray("Expected: ", mMatchStream, 0);
                    matches = false;
                    break;  // bail
                }

                if (bytes[offset + index] != mMatchStream.get(mReceiveStreamPos)) {
                    matches = false;
                    if (DEBUG) {
                        int gotValue = bytes[offset + index] & 0x000000FF;
                        int expectedValue = mMatchStream.get(mReceiveStreamPos) & 0x000000FF;
                        Log.i(TAG, "---- mismatch @"
                                + index
                                + " [0x" + Integer.toHexString(gotValue)
                                + " : 0x" + Integer.toHexString(expectedValue)
                                + "]");
                    }
                    break;
                }
                mReceiveStreamPos++;
            }

            if (DEBUG) {
                Log.i(TAG, "  returns:" + matches);
            }

            return matches;
        }

        // In some instances, BlueTooth MIDI in particular, it is possible to overrun
        // the bandwidth, resulting in lost data. In this case, slow the data stream
        // down.
        private static final int THROTTLE_PERIOD_MS = 10;
        private void portSend(MidiInputPort inputPort, byte[] bytes, int offset, int length,
                              boolean throttle) {
            if (DEBUG) {
                Log.i(TAG, "portSend() throttle:" + throttle);
            }
            try {
                if (throttle) {
                    try {
                        for (int index = 0; index < length; index++) {
                            inputPort.send(bytes, offset + index, 1);
                            Thread.sleep(THROTTLE_PERIOD_MS);
                        }
                    } catch (InterruptedException ex) {
                        Log.i(TAG, "---- InterruptedException " + ex);
                    }
                } else {
                    inputPort.send(bytes, offset, length);
                }
            } catch (IOException ex) {
                Log.i(TAG, "---- IOException " + ex);
            }
        }

        /**
         * Writes out the list of MIDI messages to the output port.
         */
        private void sendMessages() {
            if (DEBUG) {
                Log.i(TAG, "---- sendMessages()...");
            }

            synchronized (mTestLock) {
                int totalSent = 0;
                if (mIODevice.mSendPort != null) {
                    // Send a warm-up message...
                    logByteArray("warm-up: ", mWarmUpMsg, 0, mWarmUpMsg.length);
                    portSend(mIODevice.mSendPort, mWarmUpMsg, 0, mWarmUpMsg.length,
                            mRunningTestID == TESTID_BTLOOPBACK);

                    for (TestMessage msg : mTestMessages) {
                        if (DEBUG) {
                            logByteArray("send: ", msg.mMsgBytes, 0, msg.mMsgBytes.length);
                        }
                        portSend(mIODevice.mSendPort, msg.mMsgBytes, 0, msg.mMsgBytes.length,
                                mRunningTestID == TESTID_BTLOOPBACK);
                        totalSent += msg.mMsgBytes.length;
                    }
                }
                if (DEBUG) {
                    Log.i(TAG, "---- totalSent:" + totalSent);
                }
            }
        }

        /**
         * Listens for MIDI device opens. Opens I/O ports and sends out the apriori
         * setup messages.
         */
        class TestModuleOpenListener implements MidiManager.OnDeviceOpenedListener {
            @Override
            public void onDeviceOpened(MidiDevice device) {
                if (DEBUG) {
                    Log.i(TAG, "---- onDeviceOpened()");
                }
                openPorts(device);
                sendMessages();
            }
        }

        /**
         * A MidiReceiver subclass whose job it is to monitor incomming messages
         * and match them against the stream sent by the test.
         */
        private class MidiMatchingReceiver extends MidiReceiver {
            private static final String TAG = "MidiMatchingReceiver";

            @Override
            public void onSend(byte[] msg, int offset, int count, long timestamp) throws IOException {
                if (DEBUG) {
                    Log.i(TAG, "---- onSend(offset:" + offset + " count:" + count);
                    logByteArray("bytes-received: ", msg, offset, count);
                }
                synchronized (mTestLock) {
                    if (!mTestRunning) {
                        return;
                    }

                    // Check for "Warm Up" message
                    if (mReceiveStreamPos == 0 && msg[offset] != makeMIDICmd(MIDICMD_CONTROL, 0)) {
                        // advance the match stream past the "warm-up" message
                        mReceiveStreamPos += mWarmUpMsg.length;
                        if (DEBUG) {
                            Log.i(TAG, "---- No Warm Up Message Detected.");
                        }
                    }

                    mTestMismatched = !matchStream(msg, offset, count);

                    if (mTestMismatched || mReceiveStreamPos == mMatchStream.size()) {
                        mTestRunning = false;
                        mRunningTestID = TESTID_NONE;

                        if (DEBUG) {
                            Log.i(TAG, "---- Test Complete");
                        }
                        // defer closing the ports to outside of this callback.
                        new Thread(new Runnable() {
                            public void run() {
                                mTestStatus = mTestMismatched
                                        ? TESTSTATUS_FAILED_MISMATCH : TESTSTATUS_PASSED;
                                closePorts();
                            }
                        }).start();

                        enableTestButtons(true);
                        updateTestStateUI();
                    }
                }
            }
        } /* class MidiMatchingReceiver */
    } /* class JavaMidiTestModule */

    /**
     * Test Module for Bluetooth Loopback.
     * This is a specialization of JavaMidiTestModule (which has the connections for the BL device
     * itself) with and added MidiIODevice object for the USB audio device which does the
     * "looping back".
     */
    private class BTMidiTestModule extends JavaMidiTestModule {
        private static final String TAG = "BTMidiTestModule";
        private MidiIODevice mUSBLoopbackDevice = new MidiIODevice(MidiDeviceInfo.TYPE_USB);

        public BTMidiTestModule() {
            super(MidiDeviceInfo.TYPE_BLUETOOTH );
        }

        @Override
        public void scanDevices(MidiDeviceInfo[] devInfos) {
            // (normal) Scan for BT MIDI device
            super.scanDevices(devInfos);
            // Find a USB Loopback Device
            mUSBLoopbackDevice.scanDevices(devInfos);
        }

        private void openUSBEchoDevice(MidiDevice device) {
            MidiDeviceInfo deviceInfo = device.getInfo();
            int numOutputs = deviceInfo.getOutputPortCount();
            if (numOutputs > 0) {
                mUSBLoopbackDevice.mReceivePort = device.openOutputPort(0);
                mUSBLoopbackDevice.mReceivePort.connect(new USBMidiEchoReceiver());
            }

            int numInputs = deviceInfo.getInputPortCount();
            if (numInputs != 0) {
                mUSBLoopbackDevice.mSendPort = device.openInputPort(0);
            }
        }

        protected void closePorts() {
            super.closePorts();
            mUSBLoopbackDevice.closePorts();
        }

        @Override
        void startLoopbackTest(int testID) {
            if (DEBUG) {
                Log.i(TAG, "---- startLoopbackTest()");
            }

            // Setup the USB Loopback Device
            mUSBLoopbackDevice.closePorts();

            if (mIODevice.mSendDevInfo != null) {
                mMidiManager.openDevice(
                        mUSBLoopbackDevice.mSendDevInfo, new USBLoopbackOpenListener(), null);
            }

            // Now start the test as usual
            super.startLoopbackTest(testID);
        }

        /**
         * We need this OnDeviceOpenedListener to open the USB-Loopback device
         */
        private class USBLoopbackOpenListener implements MidiManager.OnDeviceOpenedListener {
            @Override
            public void onDeviceOpened(MidiDevice device) {
                if (DEBUG) {
                    Log.i("USBLoopbackOpenListener", "---- onDeviceOpened()");
                }
                mUSBLoopbackDevice.openPorts(device, new USBMidiEchoReceiver());
            }
        } /* class USBLoopbackOpenListener */

        /**
         * MidiReceiver subclass for BlueTooth Loopback Test
         *
         * This class receives bytes from the USB Interface (presumably coming from the
         * Bluetooth MIDI peripheral) and echoes them back out (presumably to the Bluetooth
         * MIDI peripheral).
         */
        //TODO - This could be pulled out into a separate class and shared with the identical
        // code in MidiNativeTestActivity if we pass in the USB Loopback Device object rather
        // than accessing it from the enclosing BTMidiTestModule class.
        private class USBMidiEchoReceiver extends MidiReceiver {
            private int mTotalBytesEchoed;

            @Override
            public void onSend(byte[] msg, int offset, int count, long timestamp) throws IOException {
                mTotalBytesEchoed += count;
                if (DEBUG) {
                    logByteArray("echo: ", msg, offset, count);
                }
                if (mUSBLoopbackDevice.mSendPort == null) {
                    Log.e(TAG, "(java) mUSBLoopbackDevice.mSendPort is null");
                } else {
                    mUSBLoopbackDevice.mSendPort.onSend(msg, offset, count, timestamp);
                }
            }
        } /* class USBMidiEchoReceiver */
    } /* class BTMidiTestModule */
} /* class MidiActivity */
