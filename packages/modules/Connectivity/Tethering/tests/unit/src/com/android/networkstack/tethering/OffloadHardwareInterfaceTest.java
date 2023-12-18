/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.networkstack.tethering;

import static android.net.util.TetheringUtils.uint16;
import static android.system.OsConstants.AF_INET;
import static android.system.OsConstants.AF_UNIX;
import static android.system.OsConstants.SOCK_STREAM;

import static com.android.networkstack.tethering.OffloadHardwareInterface.OFFLOAD_HAL_VERSION_1_0;
import static com.android.networkstack.tethering.OffloadHardwareInterface.OFFLOAD_HAL_VERSION_1_1;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.hardware.tetheroffload.config.V1_0.IOffloadConfig;
import android.hardware.tetheroffload.control.V1_0.IOffloadControl;
import android.hardware.tetheroffload.control.V1_0.NatTimeoutUpdate;
import android.hardware.tetheroffload.control.V1_0.NetworkProtocol;
import android.hardware.tetheroffload.control.V1_1.ITetheringOffloadCallback;
import android.hardware.tetheroffload.control.V1_1.OffloadCallbackEvent;
import android.net.netlink.StructNfGenMsg;
import android.net.netlink.StructNlMsgHdr;
import android.net.util.SharedLog;
import android.os.Handler;
import android.os.NativeHandle;
import android.os.test.TestLooper;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.util.Pair;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.FileDescriptor;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;

@RunWith(AndroidJUnit4.class)
@SmallTest
public final class OffloadHardwareInterfaceTest {
    private static final String RMNET0 = "test_rmnet_data0";

    private final TestLooper mTestLooper = new TestLooper();

    private OffloadHardwareInterface mOffloadHw;
    private ITetheringOffloadCallback mTetheringOffloadCallback;
    private OffloadHardwareInterface.ControlCallback mControlCallback;

    @Mock private IOffloadConfig mIOffloadConfig;
    private IOffloadControl mIOffloadControl;
    @Mock private NativeHandle mNativeHandle;

    // Random values to test Netlink message.
    private static final short TEST_TYPE = 184;
    private static final short TEST_FLAGS = 263;

    class MyDependencies extends OffloadHardwareInterface.Dependencies {
        private final int mMockControlVersion;
        MyDependencies(SharedLog log, final int mockControlVersion) {
            super(log);
            mMockControlVersion = mockControlVersion;
        }

        @Override
        public IOffloadConfig getOffloadConfig() {
            return mIOffloadConfig;
        }

        @Override
        public Pair<IOffloadControl, Integer> getOffloadControl() {
            switch (mMockControlVersion) {
                case OFFLOAD_HAL_VERSION_1_0:
                    mIOffloadControl = mock(IOffloadControl.class);
                    break;
                case OFFLOAD_HAL_VERSION_1_1:
                    mIOffloadControl =
                            mock(android.hardware.tetheroffload.control.V1_1.IOffloadControl.class);
                    break;
                default:
                    throw new IllegalArgumentException("Invalid offload control version "
                            + mMockControlVersion);
            }
            return new Pair<IOffloadControl, Integer>(mIOffloadControl, mMockControlVersion);
        }

        @Override
        public NativeHandle createConntrackSocket(final int groups) {
            return mNativeHandle;
        }
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mControlCallback = spy(new OffloadHardwareInterface.ControlCallback());
    }

    private void startOffloadHardwareInterface(int controlVersion) throws Exception {
        final SharedLog log = new SharedLog("test");
        mOffloadHw = new OffloadHardwareInterface(new Handler(mTestLooper.getLooper()), log,
                new MyDependencies(log, controlVersion));
        mOffloadHw.initOffloadConfig();
        mOffloadHw.initOffloadControl(mControlCallback);
        final ArgumentCaptor<ITetheringOffloadCallback> mOffloadCallbackCaptor =
                ArgumentCaptor.forClass(ITetheringOffloadCallback.class);
        verify(mIOffloadControl).initOffload(mOffloadCallbackCaptor.capture(), any());
        mTetheringOffloadCallback = mOffloadCallbackCaptor.getValue();
    }

    @Test
    public void testGetForwardedStats() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final OffloadHardwareInterface.ForwardedStats stats = mOffloadHw.getForwardedStats(RMNET0);
        verify(mIOffloadControl).getForwardedStats(eq(RMNET0), any());
        assertNotNull(stats);
    }

    @Test
    public void testSetLocalPrefixes() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final ArrayList<String> localPrefixes = new ArrayList<>();
        localPrefixes.add("127.0.0.0/8");
        localPrefixes.add("fe80::/64");
        mOffloadHw.setLocalPrefixes(localPrefixes);
        verify(mIOffloadControl).setLocalPrefixes(eq(localPrefixes), any());
    }

    @Test
    public void testSetDataLimit() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final long limit = 12345;
        mOffloadHw.setDataLimit(RMNET0, limit);
        verify(mIOffloadControl).setDataLimit(eq(RMNET0), eq(limit), any());
    }

    @Test
    public void testSetDataWarningAndLimit() throws Exception {
        // Verify V1.0 control HAL would reject the function call with exception.
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final long warning = 12345;
        final long limit = 67890;
        assertThrows(IllegalArgumentException.class,
                () -> mOffloadHw.setDataWarningAndLimit(RMNET0, warning, limit));
        reset(mIOffloadControl);

        // Verify V1.1 control HAL could receive this function call.
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_1);
        mOffloadHw.setDataWarningAndLimit(RMNET0, warning, limit);
        verify((android.hardware.tetheroffload.control.V1_1.IOffloadControl) mIOffloadControl)
                .setDataWarningAndLimit(eq(RMNET0), eq(warning), eq(limit), any());
    }

    @Test
    public void testSetUpstreamParameters() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final String v4addr = "192.168.10.1";
        final String v4gateway = "192.168.10.255";
        final ArrayList<String> v6gws = new ArrayList<>(0);
        v6gws.add("2001:db8::1");
        mOffloadHw.setUpstreamParameters(RMNET0, v4addr, v4gateway, v6gws);
        verify(mIOffloadControl).setUpstreamParameters(eq(RMNET0), eq(v4addr), eq(v4gateway),
                eq(v6gws), any());

        final ArgumentCaptor<ArrayList<String>> mArrayListCaptor =
                ArgumentCaptor.forClass(ArrayList.class);
        mOffloadHw.setUpstreamParameters(null, null, null, null);
        verify(mIOffloadControl).setUpstreamParameters(eq(""), eq(""), eq(""),
                mArrayListCaptor.capture(), any());
        assertEquals(mArrayListCaptor.getValue().size(), 0);
    }

    @Test
    public void testUpdateDownstreamPrefix() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        final String ifName = "wlan1";
        final String prefix = "192.168.43.0/24";
        mOffloadHw.addDownstreamPrefix(ifName, prefix);
        verify(mIOffloadControl).addDownstream(eq(ifName), eq(prefix), any());

        mOffloadHw.removeDownstreamPrefix(ifName, prefix);
        verify(mIOffloadControl).removeDownstream(eq(ifName), eq(prefix), any());
    }

    @Test
    public void testTetheringOffloadCallback() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);

        mTetheringOffloadCallback.onEvent(OffloadCallbackEvent.OFFLOAD_STARTED);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onStarted();

        mTetheringOffloadCallback.onEvent(OffloadCallbackEvent.OFFLOAD_STOPPED_ERROR);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onStoppedError();

        mTetheringOffloadCallback.onEvent(OffloadCallbackEvent.OFFLOAD_STOPPED_UNSUPPORTED);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onStoppedUnsupported();

        mTetheringOffloadCallback.onEvent(OffloadCallbackEvent.OFFLOAD_SUPPORT_AVAILABLE);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onSupportAvailable();

        mTetheringOffloadCallback.onEvent(OffloadCallbackEvent.OFFLOAD_STOPPED_LIMIT_REACHED);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onStoppedLimitReached();

        final NatTimeoutUpdate tcpParams = buildNatTimeoutUpdate(NetworkProtocol.TCP);
        mTetheringOffloadCallback.updateTimeout(tcpParams);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onNatTimeoutUpdate(eq(OsConstants.IPPROTO_TCP),
                eq(tcpParams.src.addr),
                eq(uint16(tcpParams.src.port)),
                eq(tcpParams.dst.addr),
                eq(uint16(tcpParams.dst.port)));

        final NatTimeoutUpdate udpParams = buildNatTimeoutUpdate(NetworkProtocol.UDP);
        mTetheringOffloadCallback.updateTimeout(udpParams);
        mTestLooper.dispatchAll();
        verify(mControlCallback).onNatTimeoutUpdate(eq(OsConstants.IPPROTO_UDP),
                eq(udpParams.src.addr),
                eq(uint16(udpParams.src.port)),
                eq(udpParams.dst.addr),
                eq(uint16(udpParams.dst.port)));
        reset(mControlCallback);

        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_1);

        // Verify the interface will process the events that comes from V1.1 HAL.
        mTetheringOffloadCallback.onEvent_1_1(OffloadCallbackEvent.OFFLOAD_STARTED);
        mTestLooper.dispatchAll();
        final InOrder inOrder = inOrder(mControlCallback);
        inOrder.verify(mControlCallback).onStarted();
        inOrder.verifyNoMoreInteractions();

        mTetheringOffloadCallback.onEvent_1_1(OffloadCallbackEvent.OFFLOAD_WARNING_REACHED);
        mTestLooper.dispatchAll();
        inOrder.verify(mControlCallback).onWarningReached();
        inOrder.verifyNoMoreInteractions();
    }

    @Test
    public void testSendIpv4NfGenMsg() throws Exception {
        startOffloadHardwareInterface(OFFLOAD_HAL_VERSION_1_0);
        FileDescriptor writeSocket = new FileDescriptor();
        FileDescriptor readSocket = new FileDescriptor();
        try {
            Os.socketpair(AF_UNIX, SOCK_STREAM, 0, writeSocket, readSocket);
        } catch (ErrnoException e) {
            fail();
            return;
        }
        when(mNativeHandle.getFileDescriptor()).thenReturn(writeSocket);

        mOffloadHw.sendIpv4NfGenMsg(mNativeHandle, TEST_TYPE, TEST_FLAGS);

        ByteBuffer buffer = ByteBuffer.allocate(9823);  // Arbitrary value > expectedLen.
        buffer.order(ByteOrder.nativeOrder());

        int read = Os.read(readSocket, buffer);
        final int expectedLen = StructNlMsgHdr.STRUCT_SIZE + StructNfGenMsg.STRUCT_SIZE;
        assertEquals(expectedLen, read);

        buffer.flip();
        assertEquals(expectedLen, buffer.getInt());
        assertEquals(TEST_TYPE, buffer.getShort());
        assertEquals(TEST_FLAGS, buffer.getShort());
        assertEquals(0 /* seq */, buffer.getInt());
        assertEquals(0 /* pid */, buffer.getInt());
        assertEquals(AF_INET, buffer.get());             // nfgen_family
        assertEquals(0 /* error */, buffer.get());       // version
        assertEquals(0 /* error */, buffer.getShort());  // res_id
        assertEquals(expectedLen, buffer.position());
    }

    private NatTimeoutUpdate buildNatTimeoutUpdate(final int proto) {
        final NatTimeoutUpdate params = new NatTimeoutUpdate();
        params.proto = proto;
        params.src.addr = "192.168.43.200";
        params.src.port = 100;
        params.dst.addr = "172.50.46.169";
        params.dst.port = 150;
        return params;
    }
}
