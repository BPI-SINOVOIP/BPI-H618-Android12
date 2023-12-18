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

package com.android.car.dialer.framework;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.net.Uri;
import android.provider.CallLog;
import android.telecom.Call;
import android.telecom.DisconnectCause;
import android.telecom.GatewayInfo;
import android.telecom.InCallService;
import android.util.Log;

import com.android.car.dialer.framework.testdata.CallLogDataHandler;
import com.android.car.dialer.framework.testdata.CallLogRawData;
import com.android.car.telephony.common.CallDetail;

import com.google.common.base.Predicate;
import com.google.common.collect.Lists;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

import javax.inject.Inject;
import javax.inject.Singleton;

/**
 * Manager class for creating and mocking calls.
 */
@Singleton
public class MockCallManager {
    private static final String TAG = "CD.MockCallManager";

    private InCallService mInCallService;
    private final CallLogDataHandler mCallLogDataHandler;

    private List<Call> mCallList = new ArrayList<>();
    private List<Call> mConferenceList = new ArrayList<>();
    private List<Call> mOngoingCallList = new ArrayList<>();
    private Call mPrimaryCall;
    private Call mSecondaryCall;
    private Call mConferenceCall;

    private Map<Call, List<Call.Callback>> mCallbacks = new HashMap<>();

    @Inject
    public MockCallManager(CallLogDataHandler callLogDataHandler) {
        mCallLogDataHandler = callLogDataHandler;
    }

    /**
     * Registers an InCallService.
     */
    public void registerInCallService(InCallService inCallService) {
        mInCallService = inCallService;
    }

    /**
     * Returns the mocked call list.
     */
    public final List<Call> getCallList() {
        return mCallList;
    }

    /**
     * Adds a call to the call list. Mocks call placement.
     */
    public void addCall(String id, int direction, int state) {
        Log.d(TAG, "Adding " + id);
        // Only allow two ongoing calls at once (primary and secondary). A conference call is
        // considered as a single call.
        if (mOngoingCallList.size() == 2) {
            Log.d(TAG, "Max number of ongoing calls reached. skip adding");
            return;
        }

        // Create a mock call and hold the existing call.
        Call call = createMockCall(id, direction, state);
        if (mPrimaryCall != null) {
            hold(mPrimaryCall);
        }
        mCallList.add(call);
        mInCallService.onCallAdded(call);
        updateList();
    }

    /**
     * Ends a call tied to the id.
     */
    public void endCall(String id) {
        Call call = findCallById(id);
        disconnect(call);
    }

    /**
     * Places a hold on a call
     */
    private void hold(Call call) {
        Log.d(TAG, "Holding " + call);
        when(call.getState()).thenReturn(Call.STATE_HOLDING);

        // Holding a call will automatically swap the primary and secondary calls. See updateList().
        if (call.equals(mPrimaryCall) && mSecondaryCall != null) {
            unhold(mSecondaryCall);
        }
        updateList();

        List<Call.Callback> callbacks = mCallbacks.get(call);
        for (Call.Callback callback : callbacks) {
            callback.onStateChanged(call, Call.STATE_HOLDING);
        }
    }

    /**
     * Unhold a call
     */
    private void unhold(Call call) {
        Log.d(TAG, "Unholding " + call);
        when(call.getState()).thenReturn(Call.STATE_ACTIVE);
        updateList();

        List<Call.Callback> callbacks = mCallbacks.get(call);
        for (Call.Callback callback : callbacks) {
            callback.onStateChanged(call, Call.STATE_ACTIVE);
        }
    }

    /**
     * Removes the call from the call list.
     *
     * Upon disconnecting:
     * 1. Conference call
     *    - All children must be disconnected and removed from the call list
     * 2. Child of a conference call
     *    - Current conference call parent child relation must be removed
     * 3. Single caller
     *    - Remove the call
     */
    private void disconnect(Call call) {
        Log.d(TAG, "Disconnecting " + call);

        // Remove call and its children from the call list
        mCallList.remove(call);
        for (Call child : call.getChildren()) {
            mCallList.remove(child);
            mInCallService.onCallRemoved(child);
            mCallbacks.remove(child);
        }

        // If the call is a conference, remove the reference
        boolean isConference = call.equals(mConferenceCall);
        if (isConference) {
            mConferenceCall = null;
        }
        Call parent = call.getParent();
        mInCallService.onCallRemoved(call);
        updateList();

        // If the call is child of a conference call, make sure to call callbacks and...
        if (parent != null) {
            List<Call.Callback> callbacks = mCallbacks.get(call);
            for (Call.Callback callback : callbacks) {
                callback.onChildrenChanged(parent, mConferenceList);
            }
        }

        mCallbacks.remove(call);

        // ...check if the conference still has participants
        if (mConferenceCall != null && mConferenceCall.getChildren().size() == 0) {
            mCallList.remove(mConferenceCall);
            mInCallService.onCallRemoved(mConferenceCall);
            updateList();
        }

        // Add call log if direction is defined (All calls except conference)
        if (!isConference) {
            CallDetail detail = CallDetail.fromTelecomCallDetail(call.getDetails());
            int callDuration = (int) (System.currentTimeMillis() - detail.getConnectTimeMillis());

            CallLogRawData data = new CallLogRawData();
            data.setNumberType(
                    // See CallLog.Calls#TYPE and Call.Details#CallDirections
                    call.getDetails().getDisconnectCause().getCode() == DisconnectCause.MISSED
                            ? CallLog.Calls.MISSED_TYPE
                            : call.getDetails().getCallDirection() + 1);
            data.setNumber(detail.getNumber());
            data.setInterval(callDuration);
            mCallLogDataHandler.addOneCallLog(data);
        }
    }

    /**
     * Removes all calls from the call list
     */
    public void clearCalls() {
        Log.d(TAG, "Clearing all calls");
        mCallList.clear();
        updateList();
    }

    /**
     * Merges the primary and secondary calls into a conferenced call.
     *
     * A conference call will be created if there is no current conference. Otherwise, the single
     * caller will be merged into the existing conference.
     */
    public void mergeCalls() {
        Log.d(TAG, "Merging " + mPrimaryCall + " and " + mSecondaryCall);
        if (mPrimaryCall == null || mSecondaryCall == null) {
            Log.w(TAG, "Cannot merge. Two ongoing calls must exist.");
            return;
        }

        boolean created = false;

        // If no conference call exists, create it.
        if (mConferenceCall == null) {
            created = true;
            mConferenceCall = createMockConference();
            mCallList.add(mConferenceCall);
            Log.d(TAG, "conf" + mConferenceCall + mConferenceCall.getChildren() + mConferenceList);
        }

        // Merge single caller to existing conference call.
        if (mPrimaryCall != mConferenceCall) {
            when(mPrimaryCall.getParent()).thenReturn(mConferenceCall);
            List<Call.Callback> callbacks = mCallbacks.get(mPrimaryCall);
            for (Call.Callback callback : callbacks) {
                callback.onParentChanged(mPrimaryCall, mConferenceCall);
            }
        }
        if (mSecondaryCall != mConferenceCall) {
            when(mSecondaryCall.getParent()).thenReturn(mConferenceCall);
            List<Call.Callback> callbacks = mCallbacks.get(mSecondaryCall);
            for (Call.Callback callback : callbacks) {
                callback.onParentChanged(mSecondaryCall, mConferenceCall);
            }
        }

        updateList();

        if (created) {
            Log.d(TAG, "created conf" + mConferenceCall);
            mInCallService.onCallAdded(mConferenceCall);
        }
    }

    /**
     * Answers a received call.
     */
    private void answerCall(Call call) {
        Log.d(TAG, "answering call: " + call);

        when(call.getState()).thenReturn(Call.STATE_ACTIVE);
        updateList();

        List<Call.Callback> callbacks = mCallbacks.get(call);
        for (Call.Callback callback : callbacks) {
            callback.onStateChanged(call, Call.STATE_ACTIVE);
        }
    }

    /**
     * Rejects a received call.
     */
    private void rejectCall(Call call) {
        Log.d(TAG, "rejecting call: " + call);

        disconnect(call);
    }

    /**
     * Updates the state of all mock call management objects. The order of the calls will be sorted
     * based on the {@link CallComparator}.
     */
    private void updateList() {
        List<Call> activeCallList = filter(mCallList,
                call -> call != null && call.getState() != Call.STATE_RINGING);
        mConferenceList = filter(activeCallList,
                call -> call.getParent() != null);
        mOngoingCallList = filter(activeCallList,
                call -> call.getParent() == null);
        mOngoingCallList.sort(new CallComparator());
        if (mConferenceList.isEmpty()) {
            mConferenceCall = null;
        } else {
            when(mConferenceCall.getChildren()).thenReturn(mConferenceList);
        }

        Log.d(TAG, "size:" + activeCallList.size() + " activeList" + activeCallList);
        Log.d(TAG, "conf:" + mConferenceList.size() + mConferenceList);
        Log.d(TAG, "ongoing:" + mOngoingCallList.size() + mOngoingCallList);
        Log.d(TAG, "conf:" + mConferenceCall);

        mPrimaryCall = null;
        mSecondaryCall = null;
        if (mOngoingCallList.size() > 0) {
            mPrimaryCall = mOngoingCallList.get(0);
        }
        if (mOngoingCallList.size() > 1) {
            mSecondaryCall = mOngoingCallList.get(1);
        }
    }

    /**
     * Finds the {@link Call} in mCallList by its id
     */
    private Call findCallById(String id) {
        for (Call call : mCallList) {
            CallDetail detail = CallDetail.fromTelecomCallDetail(call.getDetails());
            if (detail.getNumber().equals(id)) {
                return call;
            }
        }

        return null;
    }

    /**
     * Registers a {@link Call.Callback} to a {@link Call}
     */
    private void registerCallback(Call call, Call.Callback callback) {
        unregisterCallback(call, callback);
        List<Call.Callback> callbacks = mCallbacks.get(call);
        if (callbacks == null) {
            callbacks = new CopyOnWriteArrayList<>();
            mCallbacks.put(call, callbacks);
        }
        callbacks.add(callback);
    }

    /**
     * Unregisters the {@link Call.Callback} from a {@link Call}
     */
    private void unregisterCallback(Call call, Call.Callback callback) {
        List<Call.Callback> callbacks = mCallbacks.get(call);
        if (callbacks != null) {
            callbacks.remove(callback);
        }
    }

    private static List<Call> filter(List<Call> callList, Predicate<Call> predicate) {
        if (callList == null || predicate == null) {
            return Collections.emptyList();
        }

        List<Call> filteredResults = new ArrayList<>();
        for (Call call : callList) {
            if (predicate.apply(call)) {
                filteredResults.add(call);
            }
        }
        return filteredResults;
    }

    private static class CallComparator implements Comparator<Call> {
        /**
         * The rank of call state. Used for sorting active calls. Rank is listed from lowest to
         * highest.
         */
        private static final List<Integer> CALL_STATE_RANK = Lists.newArrayList(
                Call.STATE_RINGING,
                Call.STATE_DISCONNECTED,
                Call.STATE_DISCONNECTING,
                Call.STATE_NEW,
                Call.STATE_CONNECTING,
                Call.STATE_SELECT_PHONE_ACCOUNT,
                Call.STATE_HOLDING,
                Call.STATE_ACTIVE,
                Call.STATE_DIALING);

        @Override
        public int compare(Call call, Call otherCall) {
            boolean callHasParent = call.getParent() != null;
            boolean otherCallHasParent = otherCall.getParent() != null;

            if (callHasParent && !otherCallHasParent) {
                return 1;
            } else if (!callHasParent && otherCallHasParent) {
                return -1;
            }
            int carCallRank = CALL_STATE_RANK.indexOf(call.getState());
            int otherCarCallRank = CALL_STATE_RANK.indexOf(otherCall.getState());

            return otherCarCallRank - carCallRank;
        }
    }

    private Call createMockConference() {
        return new Builder()
                .setPhoneNumber("Conference")
                .setIsConference(true)
                .setInitialState(Call.STATE_ACTIVE)
                .buildMock();
    }

    private Call createMockCall(String number, int callDirection, int callState) {
        return new Builder()
                .setPhoneNumber(number)
                .setCallDirection(callDirection)
                .setInitialState(callState)
                .buildMock();
    }

    /**
     * Builder class for mocked Calls.
     */
    public class Builder {
        private Call mCall;
        private Call.Details mDetails;

        private String mNumber;
        private boolean mIsConference;
        private int mCallDirection = Call.Details.DIRECTION_UNKNOWN;
        private int mCallState = Call.STATE_NEW;

        public Builder() {
            mCall = mock(Call.class);
            mDetails = mock(Call.Details.class);
        }

        /**
         * Builds the mocked call
         */
        public Call buildMock() {
            updateCallDetails();
            mockCallFunctions();
            return mCall;
        }

        /**
         * Sets the phone number
         */
        public Builder setPhoneNumber(String number) {
            mNumber = number;
            return this;
        }

        /**
         * Sets whether the call is a conference
         */
        public Builder setIsConference(boolean isConference) {
            mIsConference = isConference;
            return this;
        }

        /**
         * Sets the call direction
         */
        public Builder setCallDirection(int callDirection) {
            mCallDirection = callDirection;
            return this;
        }

        /**
         * Sets the initial call state
         */
        public Builder setInitialState(int callState) {
            mCallState = callState;
            return this;
        }

        private void updateCallDetails() {
            // Set up updated details
            Uri uri = Uri.fromParts("tel", mNumber, null);
            GatewayInfo gatewayInfo = new GatewayInfo("", uri, uri);
            CharSequence label = "DisconnectCause";
            DisconnectCause disconnectCause = new DisconnectCause(1, label, null, "");
            long connectTimeMillis = System.currentTimeMillis();

            when(mDetails.getCallDirection()).thenReturn(mCallDirection);
            when(mDetails.getHandle()).thenReturn(uri);
            when(mDetails.getDisconnectCause()).thenReturn(disconnectCause);
            when(mDetails.getGatewayInfo()).thenReturn(gatewayInfo);
            when(mDetails.getConnectTimeMillis()).thenReturn(connectTimeMillis);
            if (mIsConference) {
                when(mDetails.hasProperty(Call.Details.PROPERTY_CONFERENCE)).thenReturn(true);
            }

            when(mCall.getDetails()).thenReturn(mDetails);
            when(mCall.getState()).thenReturn(mCallState);
        }

        private void mockCallFunctions() {
            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    Call.Callback callback = inv.getArgument(0);
                    registerCallback(mCall, callback);
                    return null;
                }
            }).when(mCall).registerCallback(any(Call.Callback.class));

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    Call.Callback callback = inv.getArgument(0);
                    unregisterCallback(mCall, callback);
                    return null;
                }
            }).when(mCall).unregisterCallback(any(Call.Callback.class));

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    disconnect(mCall);
                    return null;
                }
            }).when(mCall).disconnect();

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    hold(mCall);
                    return null;
                }
            }).when(mCall).hold();

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    unhold(mCall);
                    return null;
                }
            }).when(mCall).unhold();

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    mergeCalls();
                    return null;
                }
            }).when(mCall).conference(any(Call.class));

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    answerCall(mCall);
                    return null;
                }
            }).when(mCall).answer(anyInt());

            doAnswer(new Answer<Void>() {
                @Override
                public Void answer(InvocationOnMock inv) {
                    rejectCall(mCall);
                    return null;
                }
            }).when(mCall).reject(anyBoolean(), anyString());
        }
    }
}
