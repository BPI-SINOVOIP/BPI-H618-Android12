/*
 * Copyright (C) 2019 The Android Open Source Project
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
package com.android.internal.net.ipsec.ike;

import static android.net.ipsec.ike.IkeSessionConfiguration.EXTENSION_TYPE_FRAGMENTATION;
import static android.net.ipsec.ike.IkeSessionConfiguration.EXTENSION_TYPE_MOBIKE;
import static android.net.ipsec.ike.IkeSessionParams.IKE_OPTION_EAP_ONLY_AUTH;
import static android.net.ipsec.ike.IkeSessionParams.IKE_OPTION_FORCE_PORT_4500;
import static android.net.ipsec.ike.IkeSessionParams.IKE_OPTION_MOBIKE;
import static android.net.ipsec.ike.exceptions.IkeProtocolException.ERROR_TYPE_CHILD_SA_NOT_FOUND;
import static android.net.ipsec.ike.exceptions.IkeProtocolException.ERROR_TYPE_INVALID_SYNTAX;
import static android.net.ipsec.ike.exceptions.IkeProtocolException.ERROR_TYPE_NO_ADDITIONAL_SAS;
import static android.net.ipsec.ike.exceptions.IkeProtocolException.ERROR_TYPE_TEMPORARY_FAILURE;
import static android.net.ipsec.ike.exceptions.IkeProtocolException.ErrorType;
import static android.os.PowerManager.PARTIAL_WAKE_LOCK;

import static com.android.internal.net.ipsec.ike.message.IkeConfigPayload.CONFIG_TYPE_REPLY;
import static com.android.internal.net.ipsec.ike.message.IkeHeader.EXCHANGE_TYPE_INFORMATIONAL;
import static com.android.internal.net.ipsec.ike.message.IkeMessage.DECODE_STATUS_OK;
import static com.android.internal.net.ipsec.ike.message.IkeMessage.DECODE_STATUS_PARTIAL;
import static com.android.internal.net.ipsec.ike.message.IkeMessage.DECODE_STATUS_PROTECTED_ERROR;
import static com.android.internal.net.ipsec.ike.message.IkeMessage.DECODE_STATUS_UNPROTECTED_ERROR;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_COOKIE;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_COOKIE2;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_EAP_ONLY_AUTHENTICATION;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_IKEV2_FRAGMENTATION_SUPPORTED;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_MOBIKE_SUPPORTED;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_REKEY_SA;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_SIGNATURE_HASH_ALGORITHMS;
import static com.android.internal.net.ipsec.ike.message.IkeNotifyPayload.NOTIFY_TYPE_UPDATE_SA_ADDRESSES;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_AUTH;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_CP;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_DELETE;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_EAP;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_NOTIFY;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_SA;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_TS_INITIATOR;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_TS_RESPONDER;
import static com.android.internal.net.ipsec.ike.message.IkePayload.PAYLOAD_TYPE_VENDOR;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_DELETE_CHILD;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_DELETE_IKE;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_DPD;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_KEEPALIVE;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_REKEY_CHILD;
import static com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver.ACTION_REKEY_IKE;

import android.annotation.IntDef;
import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.IpSecManager;
import android.net.IpSecManager.ResourceUnavailableException;
import android.net.IpSecManager.SpiUnavailableException;
import android.net.IpSecManager.UdpEncapsulationSocket;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkRequest;
import android.net.ipsec.ike.ChildSessionCallback;
import android.net.ipsec.ike.ChildSessionParams;
import android.net.ipsec.ike.IkeSaProposal;
import android.net.ipsec.ike.IkeSessionCallback;
import android.net.ipsec.ike.IkeSessionConfiguration;
import android.net.ipsec.ike.IkeSessionConnectionInfo;
import android.net.ipsec.ike.IkeSessionParams;
import android.net.ipsec.ike.IkeSessionParams.IkeAuthConfig;
import android.net.ipsec.ike.IkeSessionParams.IkeAuthDigitalSignLocalConfig;
import android.net.ipsec.ike.IkeSessionParams.IkeAuthDigitalSignRemoteConfig;
import android.net.ipsec.ike.IkeSessionParams.IkeAuthPskConfig;
import android.net.ipsec.ike.TransportModeChildSessionParams;
import android.net.ipsec.ike.exceptions.AuthenticationFailedException;
import android.net.ipsec.ike.exceptions.IkeException;
import android.net.ipsec.ike.exceptions.IkeInternalException;
import android.net.ipsec.ike.exceptions.IkeNetworkLostException;
import android.net.ipsec.ike.exceptions.IkeProtocolException;
import android.net.ipsec.ike.exceptions.InvalidKeException;
import android.net.ipsec.ike.exceptions.InvalidSyntaxException;
import android.net.ipsec.ike.exceptions.NoValidProposalChosenException;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.system.ErrnoException;
import android.util.LongSparseArray;
import android.util.Pair;
import android.util.SparseArray;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.net.eap.EapAuthenticator;
import com.android.internal.net.eap.IEapCallback;
import com.android.internal.net.ipsec.ike.ChildSessionStateMachine.CreateChildSaHelper;
import com.android.internal.net.ipsec.ike.IkeLocalRequestScheduler.ChildLocalRequest;
import com.android.internal.net.ipsec.ike.IkeLocalRequestScheduler.IkeLocalRequest;
import com.android.internal.net.ipsec.ike.IkeLocalRequestScheduler.LocalRequest;
import com.android.internal.net.ipsec.ike.IkeLocalRequestScheduler.LocalRequestFactory;
import com.android.internal.net.ipsec.ike.SaRecord.IkeSaRecord;
import com.android.internal.net.ipsec.ike.SaRecord.SaLifetimeAlarmScheduler;
import com.android.internal.net.ipsec.ike.crypto.IkeCipher;
import com.android.internal.net.ipsec.ike.crypto.IkeMacIntegrity;
import com.android.internal.net.ipsec.ike.crypto.IkeMacPrf;
import com.android.internal.net.ipsec.ike.ike3gpp.Ike3gppExtensionExchange;
import com.android.internal.net.ipsec.ike.keepalive.IkeNattKeepalive;
import com.android.internal.net.ipsec.ike.message.IkeAuthDigitalSignPayload;
import com.android.internal.net.ipsec.ike.message.IkeAuthPayload;
import com.android.internal.net.ipsec.ike.message.IkeAuthPskPayload;
import com.android.internal.net.ipsec.ike.message.IkeCertPayload;
import com.android.internal.net.ipsec.ike.message.IkeCertX509CertPayload;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload.ConfigAttribute;
import com.android.internal.net.ipsec.ike.message.IkeDeletePayload;
import com.android.internal.net.ipsec.ike.message.IkeEapPayload;
import com.android.internal.net.ipsec.ike.message.IkeHeader;
import com.android.internal.net.ipsec.ike.message.IkeHeader.ExchangeType;
import com.android.internal.net.ipsec.ike.message.IkeIdPayload;
import com.android.internal.net.ipsec.ike.message.IkeInformationalPayload;
import com.android.internal.net.ipsec.ike.message.IkeKePayload;
import com.android.internal.net.ipsec.ike.message.IkeMessage;
import com.android.internal.net.ipsec.ike.message.IkeMessage.DecodeResult;
import com.android.internal.net.ipsec.ike.message.IkeMessage.DecodeResultError;
import com.android.internal.net.ipsec.ike.message.IkeMessage.DecodeResultOk;
import com.android.internal.net.ipsec.ike.message.IkeMessage.DecodeResultPartial;
import com.android.internal.net.ipsec.ike.message.IkeMessage.DecodeResultProtectedError;
import com.android.internal.net.ipsec.ike.message.IkeNoncePayload;
import com.android.internal.net.ipsec.ike.message.IkeNotifyPayload;
import com.android.internal.net.ipsec.ike.message.IkePayload;
import com.android.internal.net.ipsec.ike.message.IkeSaPayload;
import com.android.internal.net.ipsec.ike.message.IkeSaPayload.IkeProposal;
import com.android.internal.net.ipsec.ike.message.IkeVendorPayload;
import com.android.internal.net.ipsec.ike.net.IkeDefaultNetworkCallback;
import com.android.internal.net.ipsec.ike.net.IkeLocalAddressGenerator;
import com.android.internal.net.ipsec.ike.net.IkeNetworkCallbackBase;
import com.android.internal.net.ipsec.ike.net.IkeNetworkUpdater;
import com.android.internal.net.ipsec.ike.net.IkeSpecificNetworkCallback;
import com.android.internal.net.ipsec.ike.utils.IkeAlarmReceiver;
import com.android.internal.net.ipsec.ike.utils.IkeSecurityParameterIndex;
import com.android.internal.net.ipsec.ike.utils.IkeSpiGenerator;
import com.android.internal.net.ipsec.ike.utils.IpSecSpiGenerator;
import com.android.internal.net.ipsec.ike.utils.RandomnessFactory;
import com.android.internal.net.ipsec.ike.utils.Retransmitter;
import com.android.internal.util.State;
import com.android.modules.utils.build.SdkLevel;

import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.security.GeneralSecurityException;
import java.security.cert.TrustAnchor;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * IkeSessionStateMachine tracks states and manages exchanges of this IKE session.
 *
 * <p>IkeSessionStateMachine has two types of states. One type are states where there is no ongoing
 * procedure affecting IKE session (non-procedure state), including Initial, Idle and Receiving. All
 * other states are "procedure" states which are named as follows:
 *
 * <pre>
 * State Name = [Procedure Type] + [Exchange Initiator] + [Exchange Type].
 * - An IKE procedure consists of one or two IKE exchanges:
 *      Procedure Type = {CreateIke | DeleteIke | Info | RekeyIke | SimulRekeyIke}.
 * - Exchange Initiator indicates whether local or remote peer is the exchange initiator:
 *      Exchange Initiator = {Local | Remote}
 * - Exchange type defines the function of this exchange. To make it more descriptive, we separate
 *      Delete Exchange from generic Informational Exchange:
 *      Exchange Type = {IkeInit | IkeAuth | Create | Delete | Info}
 * </pre>
 */
public class IkeSessionStateMachine extends AbstractSessionStateMachine
        implements IkeNetworkUpdater {
    // Package private
    static final String TAG = "IkeSessionStateMachine";

    @VisibleForTesting static final String BUSY_WAKE_LOCK_TAG = "mBusyWakeLock";

    // TODO: b/140579254 Allow users to configure fragment size.

    private static final Object IKE_SESSION_LOCK = new Object();

    @GuardedBy("IKE_SESSION_LOCK")
    private static final HashMap<Context, Set<IkeSessionStateMachine>> sContextToIkeSmMap =
            new HashMap<>();

    /** Alarm receiver that will be shared by all IkeSessionStateMachine */
    private static final IkeAlarmReceiver sIkeAlarmReceiver = new IkeAlarmReceiver();

    /** Intent filter for all Intents that should be received by sIkeAlarmReceiver */
    private static final IntentFilter sIntentFilter = new IntentFilter();

    static {
        sIntentFilter.addAction(ACTION_DELETE_CHILD);
        sIntentFilter.addAction(ACTION_DELETE_IKE);
        sIntentFilter.addAction(ACTION_DPD);
        sIntentFilter.addAction(ACTION_REKEY_CHILD);
        sIntentFilter.addAction(ACTION_REKEY_IKE);
        sIntentFilter.addAction(ACTION_KEEPALIVE);
    }

    private static final AtomicInteger sIkeSessionIdGenerator = new AtomicInteger();

    // Bundle key for remote IKE SPI. Package private
    @VisibleForTesting static final String BUNDLE_KEY_IKE_REMOTE_SPI = "BUNDLE_KEY_IKE_REMOTE_SPI";
    // Bundle key for remote Child SPI. Package private
    @VisibleForTesting
    static final String BUNDLE_KEY_CHILD_REMOTE_SPI = "BUNDLE_KEY_CHILD_REMOTE_SPI";

    // Default fragment size in bytes.
    @VisibleForTesting static final int DEFAULT_FRAGMENT_SIZE = 1280;

    // Close IKE Session when all responses during this time were TEMPORARY_FAILURE(s). This
    // indicates that something has gone wrong, and we are out of sync.
    @VisibleForTesting
    static final long TEMP_FAILURE_RETRY_TIMEOUT_MS = TimeUnit.MINUTES.toMillis(5L);

    // The maximum number of attempts allowed for a single DNS resolution.
    static final int MAX_DNS_RESOLUTION_ATTEMPTS = 3;

    // Package private IKE exchange subtypes describe the specific function of a IKE
    // request/response exchange. It helps IkeSessionStateMachine to do message validation according
    // to the subtype specific rules.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        IKE_EXCHANGE_SUBTYPE_INVALID,
        IKE_EXCHANGE_SUBTYPE_IKE_INIT,
        IKE_EXCHANGE_SUBTYPE_IKE_AUTH,
        IKE_EXCHANGE_SUBTYPE_DELETE_IKE,
        IKE_EXCHANGE_SUBTYPE_DELETE_CHILD,
        IKE_EXCHANGE_SUBTYPE_REKEY_IKE,
        IKE_EXCHANGE_SUBTYPE_REKEY_CHILD,
        IKE_EXCHANGE_SUBTYPE_GENERIC_INFO
    })
    @interface IkeExchangeSubType {}

    public static final int IKE_EXCHANGE_SUBTYPE_INVALID = 0;
    public static final int IKE_EXCHANGE_SUBTYPE_IKE_INIT = 1;
    public static final int IKE_EXCHANGE_SUBTYPE_IKE_AUTH = 2;
    public static final int IKE_EXCHANGE_SUBTYPE_CREATE_CHILD = 3;
    public static final int IKE_EXCHANGE_SUBTYPE_DELETE_IKE = 4;
    public static final int IKE_EXCHANGE_SUBTYPE_DELETE_CHILD = 5;
    public static final int IKE_EXCHANGE_SUBTYPE_REKEY_IKE = 6;
    public static final int IKE_EXCHANGE_SUBTYPE_REKEY_CHILD = 7;
    public static final int IKE_EXCHANGE_SUBTYPE_GENERIC_INFO = 8;

    public static final SparseArray<String> EXCHANGE_SUBTYPE_TO_STRING;

    static {
        EXCHANGE_SUBTYPE_TO_STRING = new SparseArray<>();
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_INVALID, "Invalid");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_IKE_INIT, "IKE INIT");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_IKE_AUTH, "IKE AUTH");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_CREATE_CHILD, "Create Child");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_DELETE_IKE, "Delete IKE");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_DELETE_CHILD, "Delete Child");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_REKEY_IKE, "Rekey IKE");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_REKEY_CHILD, "Rekey Child");
        EXCHANGE_SUBTYPE_TO_STRING.put(IKE_EXCHANGE_SUBTYPE_GENERIC_INFO, "Generic Info");
    }

    /** Package private signals accessible for testing code. */
    private static final int CMD_GENERAL_BASE = CMD_PRIVATE_BASE;

    /** Receive encoded IKE packet on IkeSessionStateMachine. */
    static final int CMD_RECEIVE_IKE_PACKET = CMD_GENERAL_BASE + 1;
    /** Receive encoded IKE packet with unrecognized IKE SPI on IkeSessionStateMachine. */
    static final int CMD_RECEIVE_PACKET_INVALID_IKE_SPI = CMD_GENERAL_BASE + 2;
    /** Receive an remote request for a Child procedure. */
    static final int CMD_RECEIVE_REQUEST_FOR_CHILD = CMD_GENERAL_BASE + 3;
    /** Receive payloads from Child Session for building an outbound IKE message. */
    static final int CMD_OUTBOUND_CHILD_PAYLOADS_READY = CMD_GENERAL_BASE + 4;
    /** A Child Session has finished its procedure. */
    static final int CMD_CHILD_PROCEDURE_FINISHED = CMD_GENERAL_BASE + 5;
    /** Send request/response payloads to ChildSessionStateMachine for further processing. */
    static final int CMD_HANDLE_FIRST_CHILD_NEGOTIATION = CMD_GENERAL_BASE + 6;
    /** Receive a local request to execute from the scheduler */
    static final int CMD_EXECUTE_LOCAL_REQ = CMD_GENERAL_BASE + 7;
    /** Trigger a retransmission. */
    public static final int CMD_RETRANSMIT = CMD_GENERAL_BASE + 8;
    /** Send EAP request payloads to EapAuthenticator for further processing. */
    static final int CMD_EAP_START_EAP_AUTH = CMD_GENERAL_BASE + 9;
    /** Send the outbound IKE-wrapped EAP-Response message. */
    static final int CMD_EAP_OUTBOUND_MSG_READY = CMD_GENERAL_BASE + 10;
    /** Proxy to IkeSessionStateMachine handler to notify of errors */
    static final int CMD_EAP_ERRORED = CMD_GENERAL_BASE + 11;
    /** Proxy to IkeSessionStateMachine handler to notify of failures */
    static final int CMD_EAP_FAILED = CMD_GENERAL_BASE + 12;
    /** Proxy to IkeSessionStateMachine handler to notify of success, to continue to post-auth */
    static final int CMD_EAP_FINISH_EAP_AUTH = CMD_GENERAL_BASE + 14;
    /** Alarm goes off for a scheduled event, check {@link Message.arg2} for event type */
    static final int CMD_ALARM_FIRED = CMD_GENERAL_BASE + 15;
    /** Send keepalive packet */
    static final int CMD_SEND_KEEPALIVE = CMD_GENERAL_BASE + 16;
    /** Force close the session. This is initiated locally, but will not go into the scheduler */
    static final int CMD_KILL_SESSION = CMD_GENERAL_BASE + 17;
    /** Update the Session's underlying Network */
    static final int CMD_SET_NETWORK = CMD_GENERAL_BASE + 18;
    /** Force state machine to a target state for testing purposes. */
    static final int CMD_FORCE_TRANSITION = CMD_GENERAL_BASE + 99;

    static final int CMD_IKE_LOCAL_REQUEST_BASE = CMD_GENERAL_BASE + CMD_CATEGORY_SIZE;
    static final int CMD_LOCAL_REQUEST_CREATE_IKE = CMD_IKE_LOCAL_REQUEST_BASE + 1;
    static final int CMD_LOCAL_REQUEST_DELETE_IKE = CMD_IKE_LOCAL_REQUEST_BASE + 2;
    static final int CMD_LOCAL_REQUEST_REKEY_IKE = CMD_IKE_LOCAL_REQUEST_BASE + 3;
    static final int CMD_LOCAL_REQUEST_INFO = CMD_IKE_LOCAL_REQUEST_BASE + 4;
    static final int CMD_LOCAL_REQUEST_DPD = CMD_IKE_LOCAL_REQUEST_BASE + 5;
    static final int CMD_LOCAL_REQUEST_MOBIKE = CMD_IKE_LOCAL_REQUEST_BASE + 6;

    private static final SparseArray<String> CMD_TO_STR;

    static {
        CMD_TO_STR = new SparseArray<>();
        CMD_TO_STR.put(CMD_RECEIVE_IKE_PACKET, "Rcv packet");
        CMD_TO_STR.put(CMD_RECEIVE_PACKET_INVALID_IKE_SPI, "Rcv invalid IKE SPI");
        CMD_TO_STR.put(CMD_RECEIVE_REQUEST_FOR_CHILD, "Rcv Child request");
        CMD_TO_STR.put(CMD_OUTBOUND_CHILD_PAYLOADS_READY, "Out child payloads ready");
        CMD_TO_STR.put(CMD_CHILD_PROCEDURE_FINISHED, "Child procedure finished");
        CMD_TO_STR.put(CMD_HANDLE_FIRST_CHILD_NEGOTIATION, "Negotiate first Child");
        CMD_TO_STR.put(CMD_EXECUTE_LOCAL_REQ, "Execute local request");
        CMD_TO_STR.put(CMD_RETRANSMIT, "Retransmit");
        CMD_TO_STR.put(CMD_EAP_START_EAP_AUTH, "Start EAP");
        CMD_TO_STR.put(CMD_EAP_OUTBOUND_MSG_READY, "EAP outbound msg ready");
        CMD_TO_STR.put(CMD_EAP_ERRORED, "EAP errored");
        CMD_TO_STR.put(CMD_EAP_FAILED, "EAP failed");
        CMD_TO_STR.put(CMD_EAP_FINISH_EAP_AUTH, "Finish EAP");
        CMD_TO_STR.put(CMD_ALARM_FIRED, "Alarm Fired");
        CMD_TO_STR.put(CMD_SET_NETWORK, "Update underlying Network");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_CREATE_IKE, "Create IKE");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_DELETE_IKE, "Delete IKE");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_REKEY_IKE, "Rekey IKE");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_INFO, "Info");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_DPD, "DPD");
        CMD_TO_STR.put(CMD_LOCAL_REQUEST_MOBIKE, "MOBIKE migration event");
    }

    /** Package */
    @VisibleForTesting final IkeSessionParams mIkeSessionParams;

    // Underlying Network for this IKE Session. May change if MOBIKE is enabled.
    @VisibleForTesting Network mNetwork;

    // Network callback used to keep IkeSessionStateMachine aware of Network changes for
    // MOBIKE-enabled sessions. Initialized if MOBIKE support is determined for the IKE Session.
    private IkeNetworkCallbackBase mNetworkCallback;

    private final ConnectivityManager mConnectivityManager;

    /** Map that stores all IkeSaRecords, keyed by locally generated IKE SPI. */
    private final LongSparseArray<IkeSaRecord> mLocalSpiToIkeSaRecordMap;
    /**
     * Map that stores all ChildSessionStateMachines, keyed by remotely generated Child SPI for
     * sending IPsec packet. Different SPIs may point to the same ChildSessionStateMachine if this
     * Child Session is doing Rekey.
     */
    private final SparseArray<ChildSessionStateMachine> mRemoteSpiToChildSessionMap;

    private final int mIkeSessionId;
    private final Context mContext;
    private final IpSecManager mIpSecManager;
    private final AlarmManager mAlarmManager;
    private final IkeLocalRequestScheduler mScheduler;
    private final IkeSessionCallback mIkeSessionCallback;
    private final IkeEapAuthenticatorFactory mEapAuthenticatorFactory;
    private final TempFailureHandler mTempFailHandler;
    private final IkeLocalAddressGenerator mIkeLocalAddressGenerator;

    /** Package private */
    @VisibleForTesting final RandomnessFactory mRandomFactory;

    private final LocalRequestFactory mLocalRequestFactory;

    /**
     * mIkeSpiGenerator will be used by all IKE SA creations in this IKE Session to avoid SPI
     * collision in test mode.
     */
    private final IkeSpiGenerator mIkeSpiGenerator;
    /**
     * mIpSecSpiGenerator will be shared by all Child Sessions under this IKE Session to avoid SPI
     * collision in test mode.
     */
    private final IpSecSpiGenerator mIpSecSpiGenerator;

    /** Ensures that the system does not go to sleep in the middle of an exchange. */
    private final PowerManager.WakeLock mBusyWakeLock;

    @VisibleForTesting
    @GuardedBy("mChildCbToSessions")
    final HashMap<ChildSessionCallback, ChildSessionStateMachine> mChildCbToSessions =
            new HashMap<>();

    /** Peer-selected DH group to use. Defaults to first proposed DH group in first SA proposal. */
    @VisibleForTesting int mPeerSelectedDhGroup;

    /**
     * Package private socket that sends and receives encoded IKE message. Initialized in Initial
     * State.
     */
    @VisibleForTesting IkeSocket mIkeSocket;

    /** Local address assigned on device. Initialized in Initial State. */
    @VisibleForTesting InetAddress mLocalAddress;
    /** Remote address configured by users. Initialized in Initial State. */
    @VisibleForTesting InetAddress mRemoteAddress;
    /** Local port assigned on device. Initialized in Initial State. */
    @VisibleForTesting int mLocalPort;

    /** Available remote addresses that are v4. Resolved in Initial State. */
    @VisibleForTesting final List<Inet4Address> mRemoteAddressesV4 = new ArrayList<>();
    /** Available remote addresses that are v6. Resolved in Initial State. */
    @VisibleForTesting final List<Inet6Address> mRemoteAddressesV6 = new ArrayList<>();

    /**
     * Indicates if the IKE client has checked whether the server supports NAT-T. Sets to true when
     * the first time IKE client sends NAT_DETECTION (in other words the first time IKE client is
     * using IPv4 address since IKE does not support IPv6 NAT-T)
     */
    @VisibleForTesting boolean mHasCheckedNattSupport;
    /**
     * Indicates if the server supports NAT-T. Sets at the first time IKE client sends NAT_DETECTION
     * (in other words the first time IKE client is using IPv4 address since IKE does not support
     * IPv6 NAT-T)
     */
    @VisibleForTesting boolean mSupportNatTraversal;

    /** Indicates if local node is behind a NAT. */
    @VisibleForTesting boolean mLocalNatDetected;
    /** Indicates if remote node is behind a NAT. */
    @VisibleForTesting boolean mRemoteNatDetected;
    /** NATT keepalive scheduler. Initialized when a NAT is detected while using V4 addresses */
    @VisibleForTesting IkeNattKeepalive mIkeNattKeepalive;

    /** Indicates if both sides support fragmentation. Set in IKE INIT */
    @VisibleForTesting boolean mSupportFragment;
    /** Indicates if both sides support MOBIKE. Set in IKE AUTH. */
    @VisibleForTesting boolean mSupportMobike;

    /** Set of peer-supported Signature Hash Algorithms. Optionally set in IKE INIT. */
    @VisibleForTesting Set<Short> mPeerSignatureHashAlgorithms;

    /** Package private IkeSaProposal that represents the negotiated IKE SA proposal. */
    @VisibleForTesting IkeSaProposal mSaProposal;

    @VisibleForTesting IkeCipher mIkeCipher;
    @VisibleForTesting IkeMacIntegrity mIkeIntegrity;
    @VisibleForTesting IkeMacPrf mIkePrf;

    // FIXME: b/131265898 Pass these parameters from CreateIkeLocalIkeInit to CreateIkeLocalIkeAuth
    // as entry data when Android StateMachine can support that.
    @VisibleForTesting byte[] mIkeInitRequestBytes;
    @VisibleForTesting byte[] mIkeInitResponseBytes;
    @VisibleForTesting IkeNoncePayload mIkeInitNoncePayload;
    @VisibleForTesting IkeNoncePayload mIkeRespNoncePayload;
    @VisibleForTesting List<byte[]> mRemoteVendorIds = new ArrayList<>();
    @VisibleForTesting List<Integer> mEnabledExtensions = new ArrayList<>();

    // FIXME: b/131265898 Pass these parameters from CreateIkeLocalIkeAuth through to
    // CreateIkeLocalIkeAuthPostEap as entry data when Android StateMachine can support that.
    @VisibleForTesting IkeIdPayload mInitIdPayload;
    @VisibleForTesting IkeIdPayload mRespIdPayload;
    @VisibleForTesting List<IkePayload> mFirstChildReqList;

    // FIXME: b/131265898 Move into CreateIkeLocalIkeAuth, and pass through to
    // CreateIkeLocalIkeAuthPostEap once passing entry data is supported
    private ChildSessionParams mFirstChildSessionParams;
    private ChildSessionCallback mFirstChildCallbacks;

    /** Package */
    @VisibleForTesting IkeSaRecord mCurrentIkeSaRecord;
    /** Package */
    @VisibleForTesting IkeSaRecord mLocalInitNewIkeSaRecord;
    /** Package */
    @VisibleForTesting IkeSaRecord mRemoteInitNewIkeSaRecord;

    /** Package */
    @VisibleForTesting IkeSaRecord mIkeSaRecordSurviving;
    /** Package */
    @VisibleForTesting IkeSaRecord mIkeSaRecordAwaitingLocalDel;
    /** Package */
    @VisibleForTesting IkeSaRecord mIkeSaRecordAwaitingRemoteDel;

    private final Ike3gppExtensionExchange mIke3gppExtensionExchange;

    // States
    @VisibleForTesting final State mKillIkeSessionParent = new KillIkeSessionParent();

    @VisibleForTesting final State mInitial = new Initial();
    @VisibleForTesting final State mIdle = new Idle();
    @VisibleForTesting final State mChildProcedureOngoing = new ChildProcedureOngoing();
    @VisibleForTesting final State mReceiving = new Receiving();
    @VisibleForTesting final State mCreateIkeLocalIkeInit = new CreateIkeLocalIkeInit();
    @VisibleForTesting final State mCreateIkeLocalIkeAuth = new CreateIkeLocalIkeAuth();
    @VisibleForTesting final State mCreateIkeLocalIkeAuthInEap = new CreateIkeLocalIkeAuthInEap();

    @VisibleForTesting
    final State mCreateIkeLocalIkeAuthPostEap = new CreateIkeLocalIkeAuthPostEap();

    @VisibleForTesting final State mRekeyIkeLocalCreate = new RekeyIkeLocalCreate();
    @VisibleForTesting final State mSimulRekeyIkeLocalCreate = new SimulRekeyIkeLocalCreate();

    @VisibleForTesting
    final State mSimulRekeyIkeLocalDeleteRemoteDelete = new SimulRekeyIkeLocalDeleteRemoteDelete();

    @VisibleForTesting final State mSimulRekeyIkeLocalDelete = new SimulRekeyIkeLocalDelete();
    @VisibleForTesting final State mSimulRekeyIkeRemoteDelete = new SimulRekeyIkeRemoteDelete();
    @VisibleForTesting final State mRekeyIkeLocalDelete = new RekeyIkeLocalDelete();
    @VisibleForTesting final State mRekeyIkeRemoteDelete = new RekeyIkeRemoteDelete();
    @VisibleForTesting final State mDeleteIkeLocalDelete = new DeleteIkeLocalDelete();
    @VisibleForTesting final State mDpdIkeLocalInfo = new DpdIkeLocalInfo();
    @VisibleForTesting final State mMobikeLocalInfo = new MobikeLocalInfo();

    /** Constructor for testing. */
    @VisibleForTesting
    public IkeSessionStateMachine(
            Looper looper,
            Context context,
            IpSecManager ipSecManager,
            ConnectivityManager connectMgr,
            IkeSessionParams ikeParams,
            ChildSessionParams firstChildParams,
            Executor userCbExecutor,
            IkeSessionCallback ikeSessionCallback,
            ChildSessionCallback firstChildSessionCallback,
            IkeEapAuthenticatorFactory eapAuthenticatorFactory,
            IkeLocalAddressGenerator ikeLocalAddressGenerator,
            LocalRequestFactory localRequestFactory) {
        super(TAG, looper, userCbExecutor);

        if (ikeParams.hasIkeOption(IkeSessionParams.IKE_OPTION_MOBIKE)) {
            if (firstChildParams instanceof TransportModeChildSessionParams) {
                throw new IllegalArgumentException(
                        "Transport Mode SAs not supported when MOBIKE is enabled");
            } else if (!SdkLevel.isAtLeastS()) {
                throw new IllegalStateException("MOBIKE only supported for S+");
            }
        }

        synchronized (IKE_SESSION_LOCK) {
            if (!sContextToIkeSmMap.containsKey(context)) {
                // Pass in a Handler so #onReceive will run on the StateMachine thread
                context.registerReceiver(
                        sIkeAlarmReceiver,
                        sIntentFilter,
                        null /*broadcastPermission*/,
                        new Handler(looper));
                sContextToIkeSmMap.put(context, new HashSet<IkeSessionStateMachine>());
            }
            sContextToIkeSmMap.get(context).add(this);

            // TODO: Statically store the ikeSessionCallback to prevent user from providing the same
            // callback instance in the future
        }

        PowerManager pm = context.getSystemService(PowerManager.class);
        mBusyWakeLock = pm.newWakeLock(PARTIAL_WAKE_LOCK, TAG + BUSY_WAKE_LOCK_TAG);
        mBusyWakeLock.setReferenceCounted(false);

        mIkeSessionId = sIkeSessionIdGenerator.getAndIncrement();
        sIkeAlarmReceiver.registerIkeSession(mIkeSessionId, getHandler());

        mIkeSessionParams = ikeParams;
        mConnectivityManager = connectMgr;
        if (mIkeSessionParams.getConfiguredNetwork() != null) {
            mNetwork = mIkeSessionParams.getConfiguredNetwork();
        } else {
            mNetwork = connectMgr.getActiveNetwork();
            if (mNetwork == null) {
                throw new IllegalStateException("No active default network found");
            }
        }

        mEapAuthenticatorFactory = eapAuthenticatorFactory;

        mIkeLocalAddressGenerator = ikeLocalAddressGenerator;

        mLocalRequestFactory = localRequestFactory;

        // SaProposals.Builder guarantees there is at least one SA proposal, and each SA proposal
        // has at least one DH group.
        mPeerSelectedDhGroup =
                mIkeSessionParams.getSaProposals().get(0).getDhGroupTransforms()[0].id;

        mTempFailHandler = new TempFailureHandler(looper);

        // There are at most three IkeSaRecords co-existing during simultaneous rekeying.
        mLocalSpiToIkeSaRecordMap = new LongSparseArray<>(3);
        mRemoteSpiToChildSessionMap = new SparseArray<>();

        mContext = context;
        mIpSecManager = ipSecManager;
        mAlarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);

        mRandomFactory = new RandomnessFactory(mContext, mNetwork);
        mIkeSpiGenerator = new IkeSpiGenerator(mRandomFactory);
        mIpSecSpiGenerator = new IpSecSpiGenerator(mIpSecManager, mRandomFactory);

        mIkeSessionCallback = ikeSessionCallback;

        mFirstChildSessionParams = firstChildParams;
        mFirstChildCallbacks = firstChildSessionCallback;
        registerChildSessionCallback(firstChildParams, firstChildSessionCallback, true);

        mIke3gppExtensionExchange =
                new Ike3gppExtensionExchange(
                        mIkeSessionParams.getIke3gppExtension(), mUserCbExecutor);

        // CHECKSTYLE:OFF IndentationCheck
        addState(mKillIkeSessionParent);
            addState(mInitial, mKillIkeSessionParent);
            addState(mCreateIkeLocalIkeInit, mKillIkeSessionParent);
            addState(mCreateIkeLocalIkeAuth, mKillIkeSessionParent);
            addState(mCreateIkeLocalIkeAuthInEap, mKillIkeSessionParent);
            addState(mCreateIkeLocalIkeAuthPostEap, mKillIkeSessionParent);
            addState(mIdle, mKillIkeSessionParent);
            addState(mChildProcedureOngoing, mKillIkeSessionParent);
            addState(mReceiving, mKillIkeSessionParent);
            addState(mRekeyIkeLocalCreate, mKillIkeSessionParent);
                addState(mSimulRekeyIkeLocalCreate, mRekeyIkeLocalCreate);
            addState(mSimulRekeyIkeLocalDeleteRemoteDelete, mKillIkeSessionParent);
                addState(mSimulRekeyIkeLocalDelete, mSimulRekeyIkeLocalDeleteRemoteDelete);
                addState(mSimulRekeyIkeRemoteDelete, mSimulRekeyIkeLocalDeleteRemoteDelete);
            addState(mRekeyIkeLocalDelete, mKillIkeSessionParent);
            addState(mRekeyIkeRemoteDelete, mKillIkeSessionParent);
            addState(mDeleteIkeLocalDelete, mKillIkeSessionParent);
            addState(mDpdIkeLocalInfo, mKillIkeSessionParent);
            addState(mMobikeLocalInfo, mKillIkeSessionParent);
        // CHECKSTYLE:ON IndentationCheck

        setInitialState(mInitial);

        // TODO: Find a way to make it safe to release WakeLock when #onNewProcedureReady is called
        mScheduler =
                new IkeLocalRequestScheduler(
                        localReq -> {
                            sendMessageAtFrontOfQueue(CMD_EXECUTE_LOCAL_REQ, localReq);
                        },
                        mContext);

        mBusyWakeLock.acquire();
        start();
    }

    /** Construct an instance of IkeSessionStateMachine. */
    public IkeSessionStateMachine(
            Looper looper,
            Context context,
            IpSecManager ipSecManager,
            IkeSessionParams ikeParams,
            ChildSessionParams firstChildParams,
            Executor userCbExecutor,
            IkeSessionCallback ikeSessionCallback,
            ChildSessionCallback firstChildSessionCallback) {
        this(
                looper,
                context,
                ipSecManager,
                context.getSystemService(ConnectivityManager.class),
                ikeParams,
                firstChildParams,
                userCbExecutor,
                ikeSessionCallback,
                firstChildSessionCallback,
                new IkeEapAuthenticatorFactory(),
                new IkeLocalAddressGenerator(),
                new LocalRequestFactory());
    }

    private boolean hasChildSessionCallback(ChildSessionCallback callback) {
        synchronized (mChildCbToSessions) {
            return mChildCbToSessions.containsKey(callback);
        }
    }

    /**
     * Synchronously builds and registers a child session.
     *
     * <p>Setup of the child state machines MUST be done in two stages to ensure that if an external
     * caller calls openChildSession and then calls closeChildSession before the state machine has
     * gotten a chance to negotiate the sessions, a valid callback mapping exists (and does not
     * throw an exception that the callback was not found).
     *
     * <p>In the edge case where a child creation fails, and deletes itself, all pending requests
     * will no longer find the session in the map. Assume it has errored/failed, and skip/ignore.
     * This is safe, as closeChildSession() (previously) validated that the callback was registered.
     */
    @VisibleForTesting
    void registerChildSessionCallback(
            ChildSessionParams childParams, ChildSessionCallback callbacks, boolean isFirstChild) {
        synchronized (mChildCbToSessions) {
            if (!isFirstChild && getCurrentState() == null) {
                throw new IllegalStateException(
                        "Request rejected because IKE Session is being closed. ");
            }

            mChildCbToSessions.put(
                    callbacks,
                    ChildSessionStateMachineFactory.makeChildSessionStateMachine(
                            getHandler().getLooper(),
                            mContext,
                            mIkeSessionId,
                            mAlarmManager,
                            mRandomFactory,
                            mIpSecSpiGenerator,
                            childParams,
                            mUserCbExecutor,
                            callbacks,
                            new ChildSessionSmCallback()));
        }
    }

    /** Initiates IKE setup procedure. */
    public void openSession() {
        sendMessage(
                CMD_LOCAL_REQUEST_CREATE_IKE,
                mLocalRequestFactory.getIkeLocalRequest(CMD_LOCAL_REQUEST_CREATE_IKE));
    }

    /** Schedules a Create Child procedure. */
    public void openChildSession(
            ChildSessionParams childSessionParams, ChildSessionCallback childSessionCallback) {
        if (childSessionCallback == null) {
            throw new IllegalArgumentException("Child Session Callback must be provided");
        }

        if (hasChildSessionCallback(childSessionCallback)) {
            throw new IllegalArgumentException("Child Session Callback handle already registered");
        }

        if (mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)
                && childSessionParams instanceof TransportModeChildSessionParams) {
            throw new IllegalArgumentException(
                    "Transport Mode SAs not supported when MOBIKE is enabled");
        }

        registerChildSessionCallback(
                childSessionParams, childSessionCallback, false /*isFirstChild*/);
        sendMessage(
                CMD_LOCAL_REQUEST_CREATE_CHILD,
                mLocalRequestFactory.getChildLocalRequest(
                        CMD_LOCAL_REQUEST_CREATE_CHILD, childSessionCallback, childSessionParams));
    }

    /** Schedules a Delete Child procedure. */
    public void closeChildSession(ChildSessionCallback childSessionCallback) {
        if (childSessionCallback == null) {
            throw new IllegalArgumentException("Child Session Callback must be provided");
        }

        if (!hasChildSessionCallback(childSessionCallback)) {
            throw new IllegalArgumentException("Child Session Callback handle not registered");
        }

        sendMessage(
                CMD_LOCAL_REQUEST_DELETE_CHILD,
                mLocalRequestFactory.getChildLocalRequest(
                        CMD_LOCAL_REQUEST_DELETE_CHILD, childSessionCallback, null));
    }

    /** Initiates Delete IKE procedure. */
    public void closeSession() {
        sendMessage(
                CMD_LOCAL_REQUEST_DELETE_IKE,
                mLocalRequestFactory.getIkeLocalRequest(CMD_LOCAL_REQUEST_DELETE_IKE));
    }

    /** Forcibly close IKE Session. */
    public void killSession() {
        sendMessage(CMD_KILL_SESSION);
    }

    /** Update the IkeSessionStateMachine to use the specified Network. */
    public void setNetwork(Network network) {
        if (network == null) {
            throw new IllegalArgumentException("network must not be null");
        }

        if (!mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)) {
            throw new IllegalStateException("IKE_OPTION_MOBIKE is not set");
        }

        if (mIkeSessionParams.getConfiguredNetwork() == null) {
            throw new IllegalStateException(
                    "setNetwork() requires this IkeSession to be configured to use caller-specified"
                            + " network instead of default network");
        }

        sendMessage(CMD_SET_NETWORK, network);
    }

    private void scheduleRetry(LocalRequest localRequest) {
        sendMessageDelayed(localRequest.procedureType, localRequest, RETRY_INTERVAL_MS);
    }

    // TODO: Support initiating Delete IKE exchange when IKE SA expires

    // TODO: Add interfaces to initiate IKE exchanges.

    /**
     * This class is for handling temporary failure.
     *
     * <p>Receiving a TEMPORARY_FAILURE is caused by a temporary condition. IKE Session should be
     * closed if it continues to receive this error after several minutes.
     */
    @VisibleForTesting
    class TempFailureHandler extends Handler {
        private static final int TEMP_FAILURE_RETRY_TIMEOUT = 1;

        private boolean mTempFailureReceived = false;

        TempFailureHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (msg.what == TEMP_FAILURE_RETRY_TIMEOUT) {
                IOException error =
                        new IOException(
                                "Kept receiving TEMPORARY_FAILURE error. State information is out"
                                        + " of sync.");
                executeUserCallback(
                        () -> {
                            mIkeSessionCallback.onClosedWithException(
                                    new IkeInternalException(error));
                        });
                loge("Fatal error", error);

                closeAllSaRecords(false /*expectSaClosed*/);
                quitNow();
            } else {
                logWtf("Unknown message.what: " + msg.what);
            }
        }

        /**
         * Schedule temporary failure timeout.
         *
         * <p>Caller of this method is responsible for scheduling retry of the rejected request.
         */
        public void handleTempFailure() {
            logd("TempFailureHandler: Receive TEMPORARY FAILURE");

            if (!mTempFailureReceived) {
                sendEmptyMessageDelayed(TEMP_FAILURE_RETRY_TIMEOUT, TEMP_FAILURE_RETRY_TIMEOUT_MS);
                mTempFailureReceived = true;
            }
        }

        /** Stop tracking temporary condition when request was not rejected by TEMPORARY_FAILURE. */
        public void reset() {
            logd("TempFailureHandler: Reset Temporary failure retry timeout");
            removeMessages(TEMP_FAILURE_RETRY_TIMEOUT);
            mTempFailureReceived = false;
        }
    }

    // TODO: Add methods for building and validating general Informational packet.

    /** Switch all IKE SAs to the new IKE socket due to an underlying network change. */
    private void switchToIkeSocket(IkeSocket newSocket) {
        // Changing IkeSockets - make sure to quit NAT-T keepalive if it's going
        if (mIkeNattKeepalive != null) {
            mIkeNattKeepalive.stop();
            mIkeNattKeepalive = null;
        }

        long currentLocalSpi = mCurrentIkeSaRecord.getLocalSpi();
        migrateSpiToIkeSocket(currentLocalSpi, mIkeSocket, newSocket);

        if (mLocalInitNewIkeSaRecord != null) {
            long newLocalSpi = mLocalInitNewIkeSaRecord.getLocalSpi();
            migrateSpiToIkeSocket(newLocalSpi, mIkeSocket, newSocket);
        }
        if (mRemoteInitNewIkeSaRecord != null) {
            long newLocalSpi = mRemoteInitNewIkeSaRecord.getLocalSpi();
            migrateSpiToIkeSocket(newLocalSpi, mIkeSocket, newSocket);
        }

        mIkeSocket.releaseReference(this);
        mIkeSocket = newSocket;
    }

    private IkeSocket getIkeSocket(boolean isIpv4, boolean useEncapPort)
            throws ErrnoException, IOException, ResourceUnavailableException {
        IkeSocketConfig sockConfig = new IkeSocketConfig(mNetwork, mIkeSessionParams.getDscp());
        if (useEncapPort) {
            if (isIpv4) {
                return IkeUdpEncapSocket.getIkeUdpEncapSocket(
                        sockConfig,
                        mIpSecManager,
                        IkeSessionStateMachine.this,
                        getHandler().getLooper());
            } else {
                return IkeUdp6WithEncapPortSocket.getIkeUdpEncapSocket(
                        sockConfig, IkeSessionStateMachine.this, getHandler());
            }
        } else {
            if (isIpv4) {
                return IkeUdp4Socket.getInstance(
                        sockConfig, IkeSessionStateMachine.this, getHandler());
            } else {
                return IkeUdp6Socket.getInstance(
                        sockConfig, IkeSessionStateMachine.this, getHandler());
            }
        }
    }

    private void getAndSwitchToIkeSocket(boolean isIpv4, boolean useEncapPort) {
        try {
            IkeSocket newSocket = getIkeSocket(isIpv4, useEncapPort);
            if (newSocket == mIkeSocket) {
                // Attempting to switch to current socket - ignore.
                return;
            }
            switchToIkeSocket(newSocket);
            if (isIpv4 && useEncapPort) {
                mIkeNattKeepalive = buildAndStartNattKeepalive();
            }
            mLocalPort = mIkeSocket.getLocalPort();
        } catch (ErrnoException | IOException | ResourceUnavailableException e) {
            handleIkeFatalError(e);
        }
    }

    private void migrateSpiToIkeSocket(long localSpi, IkeSocket oldSocket, IkeSocket newSocket) {
        newSocket.registerIke(localSpi, IkeSessionStateMachine.this);
        oldSocket.unregisterIke(localSpi);
    }

    @VisibleForTesting
    void addIkeSaRecord(IkeSaRecord record) {
        mLocalSpiToIkeSaRecordMap.put(record.getLocalSpi(), record);

        // In IKE_INIT exchange, local SPI was registered with this IkeSessionStateMachine before
        // IkeSaRecord is created. Calling this method at the end of exchange will double-register
        // the SPI but it is safe because the key and value are not changed.
        mIkeSocket.registerIke(record.getLocalSpi(), this);
    }

    @VisibleForTesting
    void removeIkeSaRecord(IkeSaRecord record) {
        mIkeSocket.unregisterIke(record.getLocalSpi());
        mLocalSpiToIkeSaRecordMap.remove(record.getLocalSpi());
    }

    /**
     * Receive IKE packet from remote server.
     *
     * <p>This method is called synchronously from IkeSocket. It proxies the synchronous call as an
     * asynchronous job to the IkeSessionStateMachine handler.
     *
     * @param ikeHeader the decoded IKE header.
     * @param ikePacketBytes the byte array of the entire received IKE packet.
     */
    public void receiveIkePacket(IkeHeader ikeHeader, byte[] ikePacketBytes) {
        sendMessage(CMD_RECEIVE_IKE_PACKET, new ReceivedIkePacket(ikeHeader, ikePacketBytes));
    }

    /**
     * ReceivedIkePacket is a package private data container consists of decoded IkeHeader and
     * encoded IKE packet in a byte array.
     */
    static class ReceivedIkePacket {
        /** Decoded IKE header */
        public final IkeHeader ikeHeader;
        /** Entire encoded IKE message including IKE header */
        public final byte[] ikePacketBytes;

        ReceivedIkePacket(IkeHeader ikeHeader, byte[] ikePacketBytes) {
            this.ikeHeader = ikeHeader;
            this.ikePacketBytes = ikePacketBytes;
        }
    }

    /** Class to group parameters for negotiating the first Child SA. */
    private static class FirstChildNegotiationData {
        public final ChildSessionParams childSessionParams;
        public final ChildSessionCallback childSessionCallback;
        public final List<IkePayload> reqPayloads;
        public final List<IkePayload> respPayloads;

        FirstChildNegotiationData(
                ChildSessionParams childSessionParams,
                ChildSessionCallback childSessionCallback,
                List<IkePayload> reqPayloads,
                List<IkePayload> respPayloads) {
            this.childSessionParams = childSessionParams;
            this.childSessionCallback = childSessionCallback;
            this.reqPayloads = reqPayloads;
            this.respPayloads = respPayloads;
        }
    }

    /** Class to group parameters for building an outbound message for ChildSessions. */
    private static class ChildOutboundData {
        @ExchangeType public final int exchangeType;
        public final boolean isResp;
        public final List<IkePayload> payloadList;
        public final ChildSessionStateMachine childSession;

        ChildOutboundData(
                @ExchangeType int exchangeType,
                boolean isResp,
                List<IkePayload> payloadList,
                ChildSessionStateMachine childSession) {
            this.exchangeType = exchangeType;
            this.isResp = isResp;
            this.payloadList = payloadList;
            this.childSession = childSession;
        }
    }

    /** Callback for ChildSessionStateMachine to notify IkeSessionStateMachine. */
    @VisibleForTesting
    class ChildSessionSmCallback implements ChildSessionStateMachine.IChildSessionSmCallback {
        @Override
        public void onChildSaCreated(int remoteSpi, ChildSessionStateMachine childSession) {
            mRemoteSpiToChildSessionMap.put(remoteSpi, childSession);
        }

        @Override
        public void onChildSaDeleted(int remoteSpi) {
            mRemoteSpiToChildSessionMap.remove(remoteSpi);
        }

        @Override
        public void scheduleRetryLocalRequest(ChildLocalRequest childRequest) {
            scheduleRetry(childRequest);
        }

        @Override
        public void onOutboundPayloadsReady(
                @ExchangeType int exchangeType,
                boolean isResp,
                List<IkePayload> payloadList,
                ChildSessionStateMachine childSession) {
            sendMessage(
                    CMD_OUTBOUND_CHILD_PAYLOADS_READY,
                    new ChildOutboundData(exchangeType, isResp, payloadList, childSession));
        }

        @Override
        public void onProcedureFinished(ChildSessionStateMachine childSession) {
            if (getHandler() == null) {
                // If the state machine has quit (because IKE Session is being closed), do not send
                // any message.
                return;
            }

            sendMessage(CMD_CHILD_PROCEDURE_FINISHED, childSession);
        }

        @Override
        public void onChildSessionClosed(ChildSessionCallback userCallbacks) {
            synchronized (mChildCbToSessions) {
                mChildCbToSessions.remove(userCallbacks);
            }
        }

        @Override
        public void onFatalIkeSessionError(boolean needsNotifyRemote) {
            // TODO: If needsNotifyRemote is true, send a Delete IKE request and then kill the IKE
            // Session. Otherwise, directly kill the IKE Session.
        }
    }

    /** Top level state for handling uncaught exceptions for all subclasses. */
    abstract class ExceptionHandler extends ExceptionHandlerBase {
        @Override
        protected void cleanUpAndQuit(RuntimeException e) {
            // Clean up all SaRecords.
            closeAllSaRecords(false /*expectSaClosed*/);

            executeUserCallback(
                    () -> {
                        mIkeSessionCallback.onClosedWithException(new IkeInternalException(e));
                    });
            logWtf("Unexpected exception in " + getCurrentState().getName(), e);
            quitNow();
        }

        @Override
        protected String getCmdString(int cmd) {
            return CMD_TO_STR.get(cmd);
        }
    }

    /** Called when this StateMachine quits. */
    @Override
    protected void onQuitting() {
        // Clean up all SaRecords.
        closeAllSaRecords(true /*expectSaClosed*/);

        synchronized (mChildCbToSessions) {
            for (ChildSessionStateMachine child : mChildCbToSessions.values()) {
                // Fire asynchronous call for Child Sessions to do cleanup and remove itself
                // from the map.
                child.killSession();
            }
        }

        // Release IPsec SPIs if IKE Session is terminated before receiving the IKE AUTH response
        // that contains the first child SA proposal
        CreateChildSaHelper.releaseSpiResources(mFirstChildReqList);

        if (mIkeNattKeepalive != null) {
            mIkeNattKeepalive.stop();
        }

        if (mIkeSocket != null) {
            mIkeSocket.releaseReference(this);
        }

        if (mNetworkCallback != null) {
            mConnectivityManager.unregisterNetworkCallback(mNetworkCallback);
            mNetworkCallback = null;
        }

        sIkeAlarmReceiver.unregisterIkeSession(mIkeSessionId);

        synchronized (IKE_SESSION_LOCK) {
            Set<IkeSessionStateMachine> ikeSet = sContextToIkeSmMap.get(mContext);
            ikeSet.remove(this);
            if (ikeSet.isEmpty()) {
                mContext.unregisterReceiver(sIkeAlarmReceiver);
                sContextToIkeSmMap.remove(mContext);
            }
            // TODO: Remove the stored ikeSessionCallback
        }

        mIke3gppExtensionExchange.close();

        mBusyWakeLock.release();
        mScheduler.releaseAllLocalRequestWakeLocks();
    }

    private void closeAllSaRecords(boolean expectSaClosed) {
        closeIkeSaRecord(mCurrentIkeSaRecord, expectSaClosed);
        closeIkeSaRecord(mLocalInitNewIkeSaRecord, expectSaClosed);
        closeIkeSaRecord(mRemoteInitNewIkeSaRecord, expectSaClosed);

        mCurrentIkeSaRecord = null;
        mLocalInitNewIkeSaRecord = null;
        mRemoteInitNewIkeSaRecord = null;
    }

    private void closeIkeSaRecord(IkeSaRecord ikeSaRecord, boolean expectSaClosed) {
        if (ikeSaRecord == null) return;

        removeIkeSaRecord(ikeSaRecord);
        ikeSaRecord.close();

        if (!expectSaClosed) return;

        logWtf(
                "IkeSaRecord with local SPI: "
                        + ikeSaRecord.getLocalSpi()
                        + " is not correctly closed.");
    }

    private void handleIkeFatalError(Exception error) {
        IkeException ikeException =
                error instanceof IkeException
                        ? (IkeException) error
                        : new IkeInternalException(error);

        // Clean up all SaRecords.
        closeAllSaRecords(false /*expectSaClosed*/);
        executeUserCallback(
                () -> {
                    mIkeSessionCallback.onClosedWithException(ikeException);
                });
        loge("IKE Session fatal error in " + getCurrentState().getName(), ikeException);

        quitNow();
    }

    /** Parent state used to delete IKE sessions */
    class KillIkeSessionParent extends ExceptionHandler {
        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_KILL_SESSION:
                    closeAllSaRecords(false /*expectSaClosed*/);
                    executeUserCallback(
                            () -> {
                                mIkeSessionCallback.onClosed();
                            });
                    quitNow();
                    return HANDLED;
                default:
                    return NOT_HANDLED;
            }
        }
    }

    /** Initial state of IkeSessionStateMachine. */
    class Initial extends ExceptionHandler {
        @Override
        public void enterState() {
            try {
                resolveAndSetAvailableRemoteAddresses();

                setRemoteAddress();

                boolean isIpv4 = mRemoteAddress instanceof Inet4Address;
                mIkeSocket =
                        getIkeSocket(
                                isIpv4, mIkeSessionParams.hasIkeOption(IKE_OPTION_FORCE_PORT_4500));
                mLocalPort = mIkeSocket.getLocalPort();

                mLocalAddress =
                        mIkeLocalAddressGenerator.generateLocalAddress(
                                mNetwork, isIpv4, mRemoteAddress, mIkeSocket.getIkeServerPort());

                if (mIkeSocket instanceof IkeUdpEncapSocket) {
                    mIkeNattKeepalive = buildAndStartNattKeepalive();
                }
            } catch (ErrnoException | IOException | ResourceUnavailableException e) {
                handleIkeFatalError(e);
            }
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_LOCAL_REQUEST_CREATE_IKE:
                    transitionTo(mCreateIkeLocalIkeInit);
                    return HANDLED;
                case CMD_FORCE_TRANSITION:
                    transitionTo((State) message.obj);
                    return HANDLED;
                default:
                    return NOT_HANDLED;
            }
        }
    }

    /**
     * Set the remote address for the peer.
     *
     * <p>Prefers IPv6 addresses if:
     *
     * <ul>
     *   <li>an IPv6 address is known for the peer, and
     *   <li>the current underlying Network has a global (non-link local) IPv6 address available
     * </ul>
     *
     * Otherwise, an IPv4 address will be used.
     */
    private void setRemoteAddress() {
        LinkProperties linkProperties = mConnectivityManager.getLinkProperties(mNetwork);
        if (!mRemoteAddressesV6.isEmpty() && linkProperties.hasGlobalIpv6Address()) {
            // TODO(b/175348096): randomly choose from available addresses
            mRemoteAddress = mRemoteAddressesV6.get(0);
        } else {
            if (mRemoteAddressesV4.isEmpty()) {
                throw new IllegalArgumentException("No valid IPv4 or IPv6 addresses for peer");
            }

            // TODO(b/175348096): randomly choose from available addresses
            mRemoteAddress = mRemoteAddressesV4.get(0);
        }
    }

    /**
     * Idle represents a state when there is no ongoing IKE exchange affecting established IKE SA.
     */
    class Idle extends LocalRequestQueuer {
        private PendingIntent mDpdIntent;

        // TODO (b/152236790): Add wakelock for awaiting LocalRequests and ongoing procedures.

        @Override
        public void enterState() {
            if (!mScheduler.readyForNextProcedure()) {
                mBusyWakeLock.release();
            }

            if (mDpdIntent == null) {
                long remoteIkeSpi = mCurrentIkeSaRecord.getRemoteSpi();
                mDpdIntent =
                        buildIkeAlarmIntent(
                                mContext,
                                ACTION_DPD,
                                getIntentIdentifier(remoteIkeSpi),
                                getIntentIkeSmMsg(CMD_LOCAL_REQUEST_DPD, remoteIkeSpi));
            }
            long dpdDelayMs = TimeUnit.SECONDS.toMillis(mIkeSessionParams.getDpdDelaySeconds());

            // Initiating DPD is a way to detect the aliveness of the remote server and also a
            // way to assert the aliveness of IKE library. Considering this, the alarm to trigger
            // DPD needs to go off even when device is in doze mode to decrease the chance the
            // remote server thinks IKE library is dead. Also, since DPD initiation is
            // time-critical, we need to use "setExact" to avoid the batching alarm delay which can
            // be at most 75% for the alarm timeout (@see AlarmManagerService#maxTriggerTime).
            // Please check AlarmManager#setExactAndAllowWhileIdle for more details.
            mAlarmManager.setExactAndAllowWhileIdle(
                    AlarmManager.ELAPSED_REALTIME_WAKEUP,
                    SystemClock.elapsedRealtime() + dpdDelayMs,
                    mDpdIntent);
            logd("DPD Alarm scheduled with DPD delay: " + dpdDelayMs + "ms");
        }

        @Override
        protected void exitState() {
            // #exitState is guaranteed to be invoked when quit() or quitNow() is called
            mAlarmManager.cancel(mDpdIntent);
            logd("DPD Alarm canceled");

            mBusyWakeLock.acquire();
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_IKE_PACKET:
                    deferMessage(message);
                    transitionTo(mReceiving);
                    return HANDLED;

                case CMD_ALARM_FIRED:
                    handleFiredAlarm(message);
                    return HANDLED;

                case CMD_FORCE_TRANSITION: // Testing command
                    transitionTo((State) message.obj);
                    return HANDLED;

                case CMD_EXECUTE_LOCAL_REQ:
                    executeLocalRequest((LocalRequest) message.obj, message);
                    return HANDLED;

                case CMD_KILL_SESSION:
                    // Notify the remote that the IKE Session is being deleted. This notification is
                    // sent as a best-effort, so don't worry about retransmitting.
                    sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));

                    // Let KillIkeSessionParent handle the rest of the cleanup.
                    return NOT_HANDLED;

                case CMD_SET_NETWORK:
                    onUnderlyingNetworkUpdated((Network) message.obj);
                    return HANDLED;

                default:
                    // Queue local requests, and trigger next procedure
                    if (isLocalRequest(message.what)) {
                        handleLocalRequest(message.what, (LocalRequest) message.obj);

                        // Synchronously calls through to the scheduler callback, which will
                        // post the CMD_EXECUTE_LOCAL_REQ to the front of the queue, ensuring
                        // it is always the next request processed.
                        mScheduler.readyForNextProcedure();
                        return HANDLED;
                    }
                    return NOT_HANDLED;
            }
        }

        private void executeLocalRequest(LocalRequest req, Message message) {
            req.releaseWakeLock();

            if (!isRequestForCurrentSa(req)) {
                logd("Request is for a deleted SA. Ignore it.");
                mScheduler.readyForNextProcedure();
                return;
            }

            switch (req.procedureType) {
                case CMD_LOCAL_REQUEST_REKEY_IKE:
                    transitionTo(mRekeyIkeLocalCreate);
                    break;
                case CMD_LOCAL_REQUEST_DELETE_IKE:
                    transitionTo(mDeleteIkeLocalDelete);
                    break;
                case CMD_LOCAL_REQUEST_DPD:
                    transitionTo(mDpdIkeLocalInfo);
                    break;
                case CMD_LOCAL_REQUEST_CREATE_CHILD: // fallthrough
                case CMD_LOCAL_REQUEST_REKEY_CHILD: // fallthrough
                case CMD_LOCAL_REQUEST_REKEY_CHILD_MOBIKE: // fallthrough
                case CMD_LOCAL_REQUEST_DELETE_CHILD:
                    deferMessage(message);
                    transitionTo(mChildProcedureOngoing);
                    break;
                case CMD_LOCAL_REQUEST_MOBIKE:
                    transitionTo(mMobikeLocalInfo);
                    break;
                default:
                    cleanUpAndQuit(
                            new IllegalStateException(
                                    "Invalid local request procedure type: " + req.procedureType));
            }
        }

        // When in Idle state, this IkeSessionStateMachine and all its ChildSessionStateMachines
        // only have one alive IKE/Child SA respectively. Returns true if this local request is for
        // the current IKE/Child SA, or false if the request is for a deleted SA.
        private boolean isRequestForCurrentSa(LocalRequest localRequest) {
            if (localRequest.isChildRequest()) {
                ChildLocalRequest req = (ChildLocalRequest) localRequest;
                if (req.remoteSpi == IkeLocalRequestScheduler.SPI_NOT_INCLUDED
                        || mRemoteSpiToChildSessionMap.get(req.remoteSpi) != null) {
                    return true;
                }
            } else {
                IkeLocalRequest req = (IkeLocalRequest) localRequest;
                if (req.remoteSpi == IkeLocalRequestScheduler.SPI_NOT_INCLUDED
                        || req.remoteSpi == mCurrentIkeSaRecord.getRemoteSpi()) {
                    return true;
                }
            }
            return false;
        }
    }

    private String getIntentIdentifier() {
        return TAG + "_" + mIkeSessionId;
    }

    private String getIntentIdentifier(long remoteIkeSpi) {
        return TAG + "_" + mIkeSessionId + "_" + remoteIkeSpi;
    }

    private Message getIntentIkeSmMsg(int localRequestType, long remoteIkeSpi) {
        Bundle spiBundle = new Bundle();
        spiBundle.putLong(BUNDLE_KEY_IKE_REMOTE_SPI, remoteIkeSpi);

        return obtainMessage(CMD_ALARM_FIRED, mIkeSessionId, localRequestType, spiBundle);
    }

    @VisibleForTesting
    SaLifetimeAlarmScheduler buildSaLifetimeAlarmScheduler(long remoteSpi) {
        PendingIntent deleteSaIntent =
                buildIkeAlarmIntent(
                        mContext,
                        ACTION_DELETE_IKE,
                        getIntentIdentifier(remoteSpi),
                        getIntentIkeSmMsg(CMD_LOCAL_REQUEST_DELETE_IKE, remoteSpi));
        PendingIntent rekeySaIntent =
                buildIkeAlarmIntent(
                        mContext,
                        ACTION_REKEY_IKE,
                        getIntentIdentifier(remoteSpi),
                        getIntentIkeSmMsg(CMD_LOCAL_REQUEST_REKEY_IKE, remoteSpi));

        return new SaLifetimeAlarmScheduler(
                mIkeSessionParams.getHardLifetimeMsInternal(),
                mIkeSessionParams.getSoftLifetimeMsInternal(),
                deleteSaIntent,
                rekeySaIntent,
                mAlarmManager);
    }

    // Package private. Accessible to ChildSessionStateMachine
    static PendingIntent buildIkeAlarmIntent(
            Context context, String intentAction, String intentId, Message ikeSmMsg) {
        Intent intent = new Intent(intentAction);
        intent.setIdentifier(intentId);
        intent.setPackage(context.getPackageName());

        Bundle bundle = new Bundle();
        bundle.putParcelable(IkeAlarmReceiver.PARCELABLE_NAME_IKE_SESSION_MSG, ikeSmMsg);
        intent.putExtras(bundle);

        return PendingIntent.getBroadcast(
                context, 0 /* requestCode; unused */, intent, PendingIntent.FLAG_IMMUTABLE);
    }

    /**
     * Gets IKE exchange subtype of a inbound IKE request message.
     *
     * <p>Knowing IKE exchange subtype of a inbound IKE request message helps IkeSessionStateMachine
     * to validate this request using the specific rule.
     *
     * <p>It is not allowed to obtain exchange subtype from a inbound response message for two
     * reasons. Firstly, the exchange subtype of a response message is the same with its
     * corresponding request message. Secondly, trying to get the exchange subtype from a response
     * message will easily fail when the response message contains only error notification payloads.
     *
     * @param ikeMessage inbound request IKE message to check.
     * @return IKE exchange subtype.
     */
    @IkeExchangeSubType
    private static int getIkeExchangeSubType(IkeMessage ikeMessage) {
        IkeHeader ikeHeader = ikeMessage.ikeHeader;
        if (ikeHeader.isResponseMsg) {
            throw new IllegalStateException("IKE Exchange subtype invalid for response messages.");
        }

        switch (ikeHeader.exchangeType) {
                // DPD omitted - should never be handled via handleRequestIkeMessage()
            case IkeHeader.EXCHANGE_TYPE_IKE_SA_INIT:
                return IKE_EXCHANGE_SUBTYPE_IKE_INIT;
            case IkeHeader.EXCHANGE_TYPE_IKE_AUTH:
                return IKE_EXCHANGE_SUBTYPE_IKE_AUTH;
            case IkeHeader.EXCHANGE_TYPE_CREATE_CHILD_SA:
                // It is guaranteed in the decoding process that SA Payload has at least one SA
                // Proposal. Since Rekey IKE and Create Child (both initial creation and rekey
                // creation) will cause a collision, although the RFC 7296 does not prohibit one SA
                // Payload to contain both IKE proposals and Child proposals, containing two types
                // does not make sense. IKE libary will reply according to the first SA Proposal
                // type and ignore the other type.
                IkeSaPayload saPayload =
                        ikeMessage.getPayloadForType(
                                IkePayload.PAYLOAD_TYPE_SA, IkeSaPayload.class);
                if (saPayload == null) {
                    return IKE_EXCHANGE_SUBTYPE_INVALID;
                }

                // If the received message has both SA(IKE) Payload and Notify-Rekey Payload, IKE
                // library will treat it as a Rekey IKE request and ignore the Notify-Rekey
                // Payload to provide better interoperability.
                if (saPayload.proposalList.get(0).protocolId == IkePayload.PROTOCOL_ID_IKE) {
                    return IKE_EXCHANGE_SUBTYPE_REKEY_IKE;
                }

                // If a Notify-Rekey Payload is found, this message is for rekeying a Child SA.
                List<IkeNotifyPayload> notifyPayloads =
                        ikeMessage.getPayloadListForType(
                                IkePayload.PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);

                // It is checked during decoding that there is at most one Rekey notification
                // payload.
                for (IkeNotifyPayload notifyPayload : notifyPayloads) {
                    if (notifyPayload.notifyType == IkeNotifyPayload.NOTIFY_TYPE_REKEY_SA) {
                        return IKE_EXCHANGE_SUBTYPE_REKEY_CHILD;
                    }
                }

                return IKE_EXCHANGE_SUBTYPE_CREATE_CHILD;
            case IkeHeader.EXCHANGE_TYPE_INFORMATIONAL:
                List<IkeDeletePayload> deletePayloads =
                        ikeMessage.getPayloadListForType(
                                IkePayload.PAYLOAD_TYPE_DELETE, IkeDeletePayload.class);

                // If no Delete payload was found, this request is a generic informational request.
                if (deletePayloads.isEmpty()) return IKE_EXCHANGE_SUBTYPE_GENERIC_INFO;

                // IKEv2 protocol does not clearly disallow to have both a Delete IKE payload and a
                // Delete Child payload in one IKE message. In this case, IKE library will only
                // respond to the Delete IKE payload.
                for (IkeDeletePayload deletePayload : deletePayloads) {
                    if (deletePayload.protocolId == IkePayload.PROTOCOL_ID_IKE) {
                        return IKE_EXCHANGE_SUBTYPE_DELETE_IKE;
                    }
                }
                return IKE_EXCHANGE_SUBTYPE_DELETE_CHILD;
            default:
                throw new IllegalStateException(
                        "Unrecognized exchange type in the validated IKE header: "
                                + ikeHeader.exchangeType);
        }
    }

    // Sends the provided IkeMessage using the current IKE SA record
    @VisibleForTesting
    void sendEncryptedIkeMessage(IkeMessage msg) {
        sendEncryptedIkeMessage(mCurrentIkeSaRecord, msg);
    }

    // Sends the provided IkeMessage using the provided IKE SA record
    @VisibleForTesting
    void sendEncryptedIkeMessage(IkeSaRecord ikeSaRecord, IkeMessage msg) {
        byte[][] packetList =
                msg.encryptAndEncode(
                        mIkeIntegrity,
                        mIkeCipher,
                        ikeSaRecord,
                        mSupportFragment,
                        DEFAULT_FRAGMENT_SIZE);
        for (byte[] packet : packetList) {
            mIkeSocket.sendIkePacket(packet, mRemoteAddress);
        }
        if (msg.ikeHeader.isResponseMsg) {
            ikeSaRecord.updateLastSentRespAllPackets(
                    Arrays.asList(packetList), msg.ikeHeader.messageId);
        }
    }

    // Builds and sends IKE-level error notification response on the provided IKE SA record
    @VisibleForTesting
    void buildAndSendErrorNotificationResponse(
            IkeSaRecord ikeSaRecord, int messageId, @ErrorType int errorType) {
        IkeNotifyPayload error = new IkeNotifyPayload(errorType);
        buildAndSendNotificationResponse(ikeSaRecord, messageId, error);
    }

    // Builds and sends error notification response on the provided IKE SA record
    @VisibleForTesting
    void buildAndSendNotificationResponse(
            IkeSaRecord ikeSaRecord, int messageId, IkeNotifyPayload notifyPayload) {
        IkeMessage msg =
                buildEncryptedNotificationMessage(
                        ikeSaRecord,
                        new IkeInformationalPayload[] {notifyPayload},
                        EXCHANGE_TYPE_INFORMATIONAL,
                        true /*isResponse*/,
                        messageId);

        sendEncryptedIkeMessage(ikeSaRecord, msg);
    }

    // Builds an Encrypted IKE Informational Message for the given IkeInformationalPayload using the
    // current IKE SA record.
    @VisibleForTesting
    IkeMessage buildEncryptedInformationalMessage(
            IkeInformationalPayload[] payloads, boolean isResponse, int messageId) {
        return buildEncryptedInformationalMessage(
                mCurrentIkeSaRecord, payloads, isResponse, messageId);
    }

    // Builds an Encrypted IKE Informational Message for the given IkeInformationalPayload using the
    // provided IKE SA record.
    @VisibleForTesting
    IkeMessage buildEncryptedInformationalMessage(
            IkeSaRecord saRecord,
            IkeInformationalPayload[] payloads,
            boolean isResponse,
            int messageId) {
        return buildEncryptedNotificationMessage(
                saRecord, payloads, IkeHeader.EXCHANGE_TYPE_INFORMATIONAL, isResponse, messageId);
    }

    // Builds an Encrypted IKE Message for the given IkeInformationalPayload using the provided IKE
    // SA record and exchange type.
    @VisibleForTesting
    IkeMessage buildEncryptedNotificationMessage(
            IkeSaRecord saRecord,
            IkeInformationalPayload[] payloads,
            @ExchangeType int exchangeType,
            boolean isResponse,
            int messageId) {
        IkeHeader header =
                new IkeHeader(
                        saRecord.getInitiatorSpi(),
                        saRecord.getResponderSpi(),
                        IkePayload.PAYLOAD_TYPE_SK,
                        exchangeType,
                        isResponse /*isResponseMsg*/,
                        saRecord.isLocalInit /*fromIkeInitiator*/,
                        messageId);

        return new IkeMessage(header, Arrays.asList(payloads));
    }

    private abstract class LocalRequestQueuer extends ExceptionHandler {
        /**
         * Reroutes all local requests to the scheduler
         *
         * @param requestVal The command value of the request
         * @param req The instance of the LocalRequest to be queued.
         */
        protected void handleLocalRequest(int requestVal, LocalRequest req) {
            switch (requestVal) {
                case CMD_LOCAL_REQUEST_DELETE_IKE: // Fallthrough
                case CMD_LOCAL_REQUEST_MOBIKE: // Fallthrough
                case CMD_LOCAL_REQUEST_REKEY_IKE: // Fallthrough
                case CMD_LOCAL_REQUEST_INFO: // Fallthrough
                case CMD_LOCAL_REQUEST_DPD:
                    mScheduler.addRequest(req);
                    return;

                case CMD_LOCAL_REQUEST_CREATE_CHILD: // Fallthrough
                case CMD_LOCAL_REQUEST_REKEY_CHILD: // Fallthrough
                case CMD_LOCAL_REQUEST_REKEY_CHILD_MOBIKE: // Fallthrough
                case CMD_LOCAL_REQUEST_DELETE_CHILD:
                    ChildLocalRequest childReq = (ChildLocalRequest) req;
                    if (childReq.procedureType != requestVal) {
                        cleanUpAndQuit(
                                new IllegalArgumentException(
                                        "ChildLocalRequest procedure type was invalid"));
                    }
                    mScheduler.addRequest(childReq);
                    return;

                default:
                    cleanUpAndQuit(
                            new IllegalStateException(
                                    "Unknown local request passed to handleLocalRequest"));
            }
        }

        /** Check if received signal is a local request. */
        protected boolean isLocalRequest(int msgWhat) {
            if ((msgWhat >= CMD_IKE_LOCAL_REQUEST_BASE
                            && msgWhat < CMD_IKE_LOCAL_REQUEST_BASE + CMD_CATEGORY_SIZE)
                    || (msgWhat >= CMD_CHILD_LOCAL_REQUEST_BASE
                            && msgWhat < CMD_CHILD_LOCAL_REQUEST_BASE + CMD_CATEGORY_SIZE)) {
                return true;
            }
            return false;
        }

        protected void handleFiredAlarm(Message message) {
            switch (message.arg2) {
                case CMD_SEND_KEEPALIVE:
                    // Software keepalive alarm is fired. Ignore the alarm whe NAT-T keepalive is no
                    // longer needed (e.g. migrating from IPv4 to IPv6)
                    if (mIkeNattKeepalive != null) {
                        mIkeNattKeepalive.onAlarmFired();
                    }
                    return;
                case CMD_LOCAL_REQUEST_DELETE_CHILD: // Hits hard lifetime; fall through
                case CMD_LOCAL_REQUEST_REKEY_CHILD: // Hits soft lifetime
                    int remoteChildSpi = ((Bundle) message.obj).getInt(BUNDLE_KEY_CHILD_REMOTE_SPI);
                    enqueueLocalRequestSynchronously(
                            mLocalRequestFactory.getChildLocalRequest(
                                    message.arg2, remoteChildSpi));
                    return;
                case CMD_LOCAL_REQUEST_DELETE_IKE: // Hits hard lifetime; fall through
                case CMD_LOCAL_REQUEST_REKEY_IKE: // Hits soft lifetime; fall through
                case CMD_LOCAL_REQUEST_DPD:
                    // IKE Session has not received any protectd IKE packet for the whole DPD delay
                    long remoteIkeSpi = ((Bundle) message.obj).getLong(BUNDLE_KEY_IKE_REMOTE_SPI);
                    enqueueLocalRequestSynchronously(
                            mLocalRequestFactory.getIkeLocalRequest(message.arg2, remoteIkeSpi));

                    // TODO(b/152442041): Cancel the scheduled DPD request if IKE Session starts any
                    // procedure before DPD get executed.
                    return;
                default:
                    logWtf("Invalid alarm action: " + message.arg2);
            }
        }

        private void enqueueLocalRequestSynchronously(LocalRequest request) {
            // Use dispatchMessage to synchronously handle this message so that the AlarmManager
            // WakeLock can keep protecting this message until it is enquequed in mScheduler. It is
            // safe because the alarmReceiver is called on the Ike HandlerThread, and the
            // IkeSessionStateMachine is not currently in a state transition.
            getHandler().dispatchMessage(obtainMessage(request.procedureType, request));
        }

        /** Builds a IKE Delete Request for the given IKE SA. */
        protected IkeMessage buildIkeDeleteReq(IkeSaRecord ikeSaRecord) {
            IkeInformationalPayload[] payloads =
                    new IkeInformationalPayload[] {new IkeDeletePayload()};
            return buildEncryptedInformationalMessage(
                    ikeSaRecord,
                    payloads,
                    false /* isResp */,
                    ikeSaRecord.getLocalRequestMessageId());
        }
    }

    /**
     * Base state defines common behaviours when receiving an IKE packet.
     *
     * <p>State that represents an ongoing IKE procedure MUST extend BusyState to handle received
     * IKE packet. Idle state will defer the received packet to a BusyState to process it.
     */
    private abstract class BusyState extends LocalRequestQueuer {
        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_IKE_PACKET:
                    handleReceivedIkePacket(message);
                    return HANDLED;
                case CMD_ALARM_FIRED:
                    handleFiredAlarm(message);
                    return HANDLED;
                case CMD_FORCE_TRANSITION:
                    transitionTo((State) message.obj);
                    return HANDLED;

                case CMD_EXECUTE_LOCAL_REQ:
                    logWtf("Invalid execute local request command in non-idle state");
                    return NOT_HANDLED;

                case CMD_RETRANSMIT:
                    triggerRetransmit();
                    return HANDLED;

                case CMD_SET_NETWORK:
                    onUnderlyingNetworkUpdated((Network) message.obj);
                    return HANDLED;

                default:
                    // Queue local requests, and trigger next procedure
                    if (isLocalRequest(message.what)) {
                        handleLocalRequest(message.what, (LocalRequest) message.obj);
                        return HANDLED;
                    }
                    return NOT_HANDLED;
            }
        }

        /**
         * Handler for retransmission timer firing
         *
         * <p>By default, the trigger is logged and dropped. States that have a retransmitter should
         * override this function, and proxy the call to Retransmitter.retransmit()
         */
        protected void triggerRetransmit() {
            logWtf("Retransmission trigger dropped in state: " + this.getClass().getSimpleName());
        }

        protected IkeSaRecord getIkeSaRecordForPacket(IkeHeader ikeHeader) {
            if (ikeHeader.fromIkeInitiator) {
                return mLocalSpiToIkeSaRecordMap.get(ikeHeader.ikeResponderSpi);
            } else {
                return mLocalSpiToIkeSaRecordMap.get(ikeHeader.ikeInitiatorSpi);
            }
        }

        protected void handleReceivedIkePacket(Message message) {
            // TODO: b/138411550 Notify subclasses when discarding a received packet. Receiving MUST
            // go back to Idle state in this case.

            String methodTag = "handleReceivedIkePacket: ";

            ReceivedIkePacket receivedIkePacket = (ReceivedIkePacket) message.obj;
            IkeHeader ikeHeader = receivedIkePacket.ikeHeader;
            byte[] ikePacketBytes = receivedIkePacket.ikePacketBytes;
            IkeSaRecord ikeSaRecord = getIkeSaRecordForPacket(ikeHeader);

            String msgDirection = ikeHeader.isResponseMsg ? "response" : "request";

            // Drop packets that we don't have an SA for:
            if (ikeSaRecord == null) {
                // TODO: Print a summary of the IKE message (perhaps the IKE header)
                cleanUpAndQuit(
                        new IllegalStateException(
                                "Received an IKE "
                                        + msgDirection
                                        + "but found no matching SA for it"));
                return;
            }

            logd(
                    methodTag
                            + "Received an "
                            + ikeHeader.getBasicInfoString()
                            + " on IKE SA with local SPI: "
                            + ikeSaRecord.getLocalSpi()
                            + ". Packet size: "
                            + ikePacketBytes.length);

            if (ikeHeader.isResponseMsg) {
                int expectedMsgId = ikeSaRecord.getLocalRequestMessageId();
                if (expectedMsgId - 1 == ikeHeader.messageId) {
                    logd(methodTag + "Received re-transmitted response. Discard it.");
                    return;
                }

                DecodeResult decodeResult =
                        IkeMessage.decode(
                                expectedMsgId,
                                mIkeIntegrity,
                                mIkeCipher,
                                ikeSaRecord,
                                ikeHeader,
                                ikePacketBytes,
                                ikeSaRecord.getCollectedFragments(true /*isResp*/));
                switch (decodeResult.status) {
                    case DECODE_STATUS_OK:
                        ikeSaRecord.incrementLocalRequestMessageId();
                        ikeSaRecord.resetCollectedFragments(true /*isResp*/);

                        DecodeResultOk resultOk = (DecodeResultOk) decodeResult;
                        if (isTempFailure(resultOk.ikeMessage)) {
                            handleTempFailure();
                        } else {
                            mTempFailHandler.reset();
                        }

                        handleResponseIkeMessage(resultOk.ikeMessage);
                        break;
                    case DECODE_STATUS_PARTIAL:
                        ikeSaRecord.updateCollectedFragments(
                                (DecodeResultPartial) decodeResult, true /*isResp*/);
                        break;
                    case DECODE_STATUS_PROTECTED_ERROR:
                        IkeException ikeException = ((DecodeResultError) decodeResult).ikeException;
                        logi(methodTag + "Protected error", ikeException);

                        ikeSaRecord.incrementLocalRequestMessageId();
                        ikeSaRecord.resetCollectedFragments(true /*isResp*/);

                        handleResponseGenericProcessError(
                                ikeSaRecord,
                                new InvalidSyntaxException(
                                        "Generic processing error in the received response",
                                        ikeException));
                        break;
                    case DECODE_STATUS_UNPROTECTED_ERROR:
                        logi(
                                methodTag
                                        + "Message authentication or decryption failed on received"
                                        + " response. Discard it",
                                ((DecodeResultError) decodeResult).ikeException);
                        break;
                    default:
                        cleanUpAndQuit(
                                new IllegalStateException(
                                        "Unrecognized decoding status: " + decodeResult.status));
                }

            } else {
                int expectedMsgId = ikeSaRecord.getRemoteRequestMessageId();
                if (expectedMsgId - 1 == ikeHeader.messageId) {

                    if (ikeSaRecord.isRetransmittedRequest(ikePacketBytes)) {
                        if (ikeSaRecord.getLastSentRespMsgId() == ikeHeader.messageId) {
                            logd(
                                    "Received re-transmitted request "
                                            + ikeHeader.messageId
                                            + " Retransmitting response");
                            for (byte[] packet : ikeSaRecord.getLastSentRespAllPackets()) {
                                mIkeSocket.sendIkePacket(packet, mRemoteAddress);
                            }
                        } else {
                            logd(
                                    "Received re-transmitted request "
                                            + ikeHeader.messageId
                                            + " Original request is still being processed");
                        }

                        // TODO:Support resetting remote rekey delete timer.
                    } else {
                        logi(methodTag + "Received response with invalid message ID. Discard it.");
                    }
                } else {
                    DecodeResult decodeResult =
                            IkeMessage.decode(
                                    expectedMsgId,
                                    mIkeIntegrity,
                                    mIkeCipher,
                                    ikeSaRecord,
                                    ikeHeader,
                                    ikePacketBytes,
                                    ikeSaRecord.getCollectedFragments(false /*isResp*/));
                    switch (decodeResult.status) {
                        case DECODE_STATUS_OK:
                            ikeSaRecord.incrementRemoteRequestMessageId();
                            ikeSaRecord.resetCollectedFragments(false /*isResp*/);

                            DecodeResultOk resultOk = (DecodeResultOk) decodeResult;
                            IkeMessage ikeMessage = resultOk.ikeMessage;
                            ikeSaRecord.updateLastReceivedReqFirstPacket(resultOk.firstPacket);

                            // Handle DPD here.
                            if (ikeMessage.isDpdRequest()) {
                                logd(methodTag + "Received DPD request");
                                IkeMessage dpdResponse =
                                        buildEncryptedInformationalMessage(
                                                ikeSaRecord,
                                                new IkeInformationalPayload[] {},
                                                true,
                                                ikeHeader.messageId);
                                sendEncryptedIkeMessage(ikeSaRecord, dpdResponse);
                                break;
                            }

                            int ikeExchangeSubType = getIkeExchangeSubType(ikeMessage);
                            logd(
                                    methodTag
                                            + "Request exchange subtype: "
                                            + EXCHANGE_SUBTYPE_TO_STRING.get(ikeExchangeSubType));

                            if (ikeExchangeSubType == IKE_EXCHANGE_SUBTYPE_INVALID
                                    || ikeExchangeSubType == IKE_EXCHANGE_SUBTYPE_IKE_INIT
                                    || ikeExchangeSubType == IKE_EXCHANGE_SUBTYPE_IKE_AUTH) {

                                // Reply with INVALID_SYNTAX and close IKE Session.
                                buildAndSendErrorNotificationResponse(
                                        mCurrentIkeSaRecord,
                                        ikeHeader.messageId,
                                        ERROR_TYPE_INVALID_SYNTAX);
                                handleIkeFatalError(
                                        new InvalidSyntaxException(
                                                "Cannot handle message with invalid or unexpected"
                                                        + " IkeExchangeSubType: "
                                                        + ikeExchangeSubType));
                                return;
                            }
                            handleRequestIkeMessage(ikeMessage, ikeExchangeSubType, message);
                            break;
                        case DECODE_STATUS_PARTIAL:
                            ikeSaRecord.updateCollectedFragments(
                                    (DecodeResultPartial) decodeResult, false /*isResp*/);
                            break;
                        case DECODE_STATUS_PROTECTED_ERROR:
                            DecodeResultProtectedError resultError =
                                    (DecodeResultProtectedError) decodeResult;

                            IkeException ikeException = resultError.ikeException;
                            logi(methodTag + "Protected error", resultError.ikeException);

                            ikeSaRecord.incrementRemoteRequestMessageId();
                            ikeSaRecord.resetCollectedFragments(false /*isResp*/);

                            ikeSaRecord.updateLastReceivedReqFirstPacket(resultError.firstPacket);

                            // IkeException MUST be already wrapped into an IkeProtocolException
                            handleRequestGenericProcessError(
                                    ikeSaRecord,
                                    ikeHeader.messageId,
                                    (IkeProtocolException) ikeException);
                            break;
                        case DECODE_STATUS_UNPROTECTED_ERROR:
                            logi(
                                    methodTag
                                            + "Message authentication or decryption failed on"
                                            + " received request. Discard it",
                                    ((DecodeResultError) decodeResult).ikeException);
                            break;
                        default:
                            cleanUpAndQuit(
                                    new IllegalStateException(
                                            "Unrecognized decoding status: "
                                                    + decodeResult.status));
                    }
                }
            }
        }

        private boolean isTempFailure(IkeMessage message) {
            List<IkeNotifyPayload> notifyPayloads =
                    message.getPayloadListForType(PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);

            for (IkeNotifyPayload notify : notifyPayloads) {
                if (notify.notifyType == ERROR_TYPE_TEMPORARY_FAILURE) {
                    return true;
                }
            }
            return false;
        }

        protected void handleTempFailure() {
            // Log and close IKE Session due to unexpected TEMPORARY_FAILURE. This error should
            // only occur during CREATE_CHILD_SA exchange.
            handleIkeFatalError(
                    new InvalidSyntaxException("Received unexpected TEMPORARY_FAILURE"));

            // States that accept a TEMPORARY MUST override this method to schedule a retry.
        }

        protected void handleGenericInfoRequest(IkeMessage ikeMessage) {
            try {
                List<IkeInformationalPayload> infoPayloadList = new ArrayList<>();
                for (IkePayload payload : ikeMessage.ikePayloadList) {
                    switch (payload.payloadType) {
                        case PAYLOAD_TYPE_CP:
                            // TODO(b/150327849): Respond with config payload responses.
                            break;
                        case PAYLOAD_TYPE_NOTIFY:
                            IkeNotifyPayload notify = (IkeNotifyPayload) payload;
                            if (notify.notifyType == NOTIFY_TYPE_COOKIE2) {
                                infoPayloadList.add(
                                        IkeNotifyPayload.handleCookie2AndGenerateCopy(notify));
                            }

                            // No action for other notifications
                            break;
                        default:
                            logw(
                                    "Received unexpected payload in an INFORMATIONAL request."
                                            + " Payload type: "
                                            + payload.payloadType);
                    }
                }

                IkeMessage infoResp =
                        buildEncryptedInformationalMessage(
                                infoPayloadList.toArray(
                                        new IkeInformationalPayload[infoPayloadList.size()]),
                                true /* isResponse */,
                                ikeMessage.ikeHeader.messageId);
                sendEncryptedIkeMessage(infoResp);
            } catch (InvalidSyntaxException e) {
                buildAndSendErrorNotificationResponse(
                        mCurrentIkeSaRecord,
                        ikeMessage.ikeHeader.messageId,
                        ERROR_TYPE_INVALID_SYNTAX);
                handleIkeFatalError(e);
                return;
            }
        }

        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            // Subclasses MUST override it if they care
            cleanUpAndQuit(
                    new IllegalStateException(
                            "Do not support handling an encrypted request: " + ikeExchangeSubType));
        }

        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            // Subclasses MUST override it if they care
            cleanUpAndQuit(
                    new IllegalStateException("Do not support handling an encrypted response"));
        }

        /**
         * Method for handling generic processing error of a request.
         *
         * <p>A generic processing error is usally syntax error, unsupported critical payload error
         * and major version error. IKE SA that should reply with corresponding error notifications
         */
        protected void handleRequestGenericProcessError(
                IkeSaRecord ikeSaRecord, int messageId, IkeProtocolException exception) {
            IkeNotifyPayload errNotify = exception.buildNotifyPayload();
            sendEncryptedIkeMessage(
                    ikeSaRecord,
                    buildEncryptedInformationalMessage(
                            ikeSaRecord,
                            new IkeInformationalPayload[] {errNotify},
                            true /*isResponse*/,
                            messageId));

            // Receiver of INVALID_SYNTAX error notification should delete the IKE SA
            if (exception.getErrorType() == ERROR_TYPE_INVALID_SYNTAX) {
                handleIkeFatalError(exception);
            }
        }

        /**
         * Method for handling generic processing error of a response.
         *
         * <p>Detailed error is wrapped in the InvalidSyntaxException, which is usally syntax error,
         * unsupported critical payload error and major version error. IKE SA that receives a
         * response with these errors should be closed.
         */
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            // Subclasses MUST override it if they care
            cleanUpAndQuit(
                    new IllegalStateException(
                            "Do not support handling generic processing error of encrypted"
                                    + " response"));
        }

        /**
         * Method for handling and extracting 3GPP-specific payloads from the IKE response payloads.
         *
         * <p>Returns the extracted 3GPP payloads after they have been handled. Only non
         * error-notify payloads are returned.
         */
        protected List<IkePayload> handle3gppRespAndExtractNonError3gppPayloads(
                int exchangeSubtype, List<IkePayload> respPayloads) throws InvalidSyntaxException {
            List<IkePayload> ike3gppPayloads =
                    mIke3gppExtensionExchange.extract3gppResponsePayloads(
                            exchangeSubtype, respPayloads);

            mIke3gppExtensionExchange.handle3gppResponsePayloads(exchangeSubtype, ike3gppPayloads);

            List<IkePayload> ike3gppErrorNotifyPayloads = new ArrayList<>();
            for (IkePayload payload : ike3gppPayloads) {
                if (payload instanceof IkeNotifyPayload) {
                    IkeNotifyPayload notifyPayload = (IkeNotifyPayload) payload;
                    if (notifyPayload.isErrorNotify()) {
                        ike3gppErrorNotifyPayloads.add(payload);
                    }
                }
            }
            ike3gppPayloads.removeAll(ike3gppErrorNotifyPayloads);

            return ike3gppPayloads;
        }
    }

    /**
     * Retransmitter represents a RAII class to send the initial request, and retransmit as needed.
     *
     * <p>The Retransmitter class will automatically start transmission upon creation.
     */
    @VisibleForTesting
    class EncryptedRetransmitter extends Retransmitter {
        private final IkeSaRecord mIkeSaRecord;

        @VisibleForTesting
        EncryptedRetransmitter(IkeMessage msg) {
            this(mCurrentIkeSaRecord, msg);
        }

        private EncryptedRetransmitter(IkeSaRecord ikeSaRecord, IkeMessage msg) {
            super(getHandler(), msg, mIkeSessionParams.getRetransmissionTimeoutsMillis());

            mIkeSaRecord = ikeSaRecord;

            retransmit();
        }

        @Override
        public void send(IkeMessage msg) {
            sendEncryptedIkeMessage(mIkeSaRecord, msg);
        }

        @Override
        public void handleRetransmissionFailure() {
            handleIkeFatalError(new IOException("Retransmitting failure"));
        }
    }

    /**
     * DeleteResponderBase represents all states after IKE_INIT and IKE_AUTH.
     *
     * <p>All post-init states share common functionality of being able to respond to IKE_DELETE
     * requests.
     */
    private abstract class DeleteResponderBase extends BusyState {
        /** Builds a IKE Delete Response for the given IKE SA and request. */
        protected IkeMessage buildIkeDeleteResp(IkeMessage req, IkeSaRecord ikeSaRecord) {
            IkeInformationalPayload[] payloads = new IkeInformationalPayload[] {};
            return buildEncryptedInformationalMessage(
                    ikeSaRecord, payloads, true /* isResp */, req.ikeHeader.messageId);
        }

        /**
         * Validates that the delete request is acceptable.
         *
         * <p>The request message must be guaranteed by previous checks to be of SUBTYPE_DELETE_IKE,
         * and therefore contains an IkeDeletePayload. This is checked in getIkeExchangeSubType.
         */
        protected void validateIkeDeleteReq(IkeMessage req, IkeSaRecord expectedRecord)
                throws InvalidSyntaxException {
            if (expectedRecord != getIkeSaRecordForPacket(req.ikeHeader)) {
                throw new InvalidSyntaxException("Delete request received in wrong SA");
            }
        }

        /**
         * Helper method for responding to a session deletion request
         *
         * <p>Note that this method expects that the session is keyed on the current IKE SA session,
         * and closing the IKE SA indicates that the remote wishes to end the session as a whole. As
         * such, this should not be used in rekey cases where there is any ambiguity as to which IKE
         * SA the session is reliant upon.
         *
         * <p>Note that this method will also quit the state machine.
         *
         * @param ikeMessage The received session deletion request
         */
        protected void handleDeleteSessionRequest(IkeMessage ikeMessage) {
            try {
                validateIkeDeleteReq(ikeMessage, mCurrentIkeSaRecord);
                IkeMessage resp = buildIkeDeleteResp(ikeMessage, mCurrentIkeSaRecord);

                executeUserCallback(
                        () -> {
                            mIkeSessionCallback.onClosed();
                        });

                sendEncryptedIkeMessage(mCurrentIkeSaRecord, resp);

                removeIkeSaRecord(mCurrentIkeSaRecord);
                mCurrentIkeSaRecord.close();
                mCurrentIkeSaRecord = null;

                quitNow();
            } catch (InvalidSyntaxException e) {
                // Got deletion of a non-Current IKE SA. Program error.
                cleanUpAndQuit(new IllegalStateException(e));
            }
        }
    }

    /**
     * DeleteBase abstracts deletion handling for all states initiating a delete exchange
     *
     * <p>All subclasses of this state share common functionality that a deletion request is sent,
     * and the response is received.
     */
    private abstract class DeleteBase extends DeleteResponderBase {
        protected void validateIkeDeleteResp(IkeMessage resp, IkeSaRecord expectedSaRecord)
                throws InvalidSyntaxException {
            if (expectedSaRecord != getIkeSaRecordForPacket(resp.ikeHeader)) {
                throw new IllegalStateException("Response received on incorrect SA");
            }

            if (resp.ikeHeader.exchangeType != IkeHeader.EXCHANGE_TYPE_INFORMATIONAL) {
                throw new InvalidSyntaxException(
                        "Invalid exchange type; expected INFORMATIONAL, but got: "
                                + resp.ikeHeader.exchangeType);
            }

            if (!resp.ikePayloadList.isEmpty()) {
                throw new InvalidSyntaxException(
                        "Unexpected payloads - IKE Delete response should be empty.");
            }
        }
    }

    /**
     * Receiving represents a state when idle IkeSessionStateMachine receives an incoming packet.
     *
     * <p>If this incoming packet is fully handled by Receiving state and does not trigger any
     * further state transition or deletion of whole IKE Session, IkeSessionStateMachine MUST
     * transition back to Idle.
     */
    class Receiving extends RekeyIkeHandlerBase {
        private boolean mProcedureFinished = true;

        @Override
        public void enterState() {
            mProcedureFinished = true;
        }

        @Override
        protected void handleReceivedIkePacket(Message message) {
            super.handleReceivedIkePacket(message);

            // If the received packet does not trigger a state transition or the packet causes this
            // state machine to quit, transition back to Idle State. In the second case, state
            // machine will first go back to Idle and then quit.
            if (mProcedureFinished) transitionTo(mIdle);
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_REKEY_IKE:
                    // Errors in this exchange with no specific protocol error code will all be
                    // classified to use NO_PROPOSAL_CHOSEN. The reason that we don't use
                    // NO_ADDITIONAL_SAS is because it indicates "responder is unwilling to accept
                    // any more Child SAs on this IKE SA.", according to RFC 7296. Sending this
                    // error may mislead the remote peer.
                    try {
                        validateIkeRekeyReq(ikeMessage);

                        // TODO: Add support for limited re-negotiation of parameters

                        // Build a rekey response payload with our previously selected proposal,
                        // against which we will validate the received proposals.
                        IkeSaPayload reqSaPayload =
                                ikeMessage.getPayloadForType(
                                        IkePayload.PAYLOAD_TYPE_SA, IkeSaPayload.class);
                        byte respProposalNumber =
                                reqSaPayload.getNegotiatedProposalNumber(mSaProposal);

                        List<IkePayload> payloadList =
                                CreateIkeSaHelper.getRekeyIkeSaResponsePayloads(
                                        respProposalNumber,
                                        mSaProposal,
                                        mIkeSpiGenerator,
                                        mLocalAddress,
                                        mRandomFactory);

                        // Build IKE header
                        IkeHeader ikeHeader =
                                new IkeHeader(
                                        mCurrentIkeSaRecord.getInitiatorSpi(),
                                        mCurrentIkeSaRecord.getResponderSpi(),
                                        IkePayload.PAYLOAD_TYPE_SK,
                                        IkeHeader.EXCHANGE_TYPE_CREATE_CHILD_SA,
                                        true /*isResponseMsg*/,
                                        mCurrentIkeSaRecord.isLocalInit,
                                        ikeMessage.ikeHeader.messageId);

                        IkeMessage responseIkeMessage = new IkeMessage(ikeHeader, payloadList);

                        // Build new SA first to ensure that we can find a valid proposal.
                        mRemoteInitNewIkeSaRecord =
                                validateAndBuildIkeSa(
                                        ikeMessage, responseIkeMessage, false /*isLocalInit*/);

                        sendEncryptedIkeMessage(responseIkeMessage);

                        transitionTo(mRekeyIkeRemoteDelete);
                        mProcedureFinished = false;
                    } catch (IkeProtocolException e) {
                        handleRekeyCreationFailure(ikeMessage.ikeHeader.messageId, e);
                    } catch (GeneralSecurityException e) {
                        handleRekeyCreationFailure(
                                ikeMessage.ikeHeader.messageId,
                                new NoValidProposalChosenException(
                                        "Error in building new IKE SA", e));
                    } catch (IOException e) {
                        handleRekeyCreationFailure(
                                ikeMessage.ikeHeader.messageId,
                                new NoValidProposalChosenException(
                                        "IKE SPI allocation collided - they reused an SPI.", e));
                    }
                    return;
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    handleDeleteSessionRequest(ikeMessage);
                    return;
                case IKE_EXCHANGE_SUBTYPE_CREATE_CHILD: // Fall through
                case IKE_EXCHANGE_SUBTYPE_DELETE_CHILD: // Fall through
                case IKE_EXCHANGE_SUBTYPE_REKEY_CHILD:
                    deferMessage(
                            obtainMessage(
                                    CMD_RECEIVE_REQUEST_FOR_CHILD,
                                    ikeExchangeSubType,
                                    0 /*placeHolder*/,
                                    ikeMessage));
                    transitionTo(mChildProcedureOngoing);
                    mProcedureFinished = false;
                    return;
                case IKE_EXCHANGE_SUBTYPE_GENERIC_INFO:
                    handleGenericInfoRequest(ikeMessage);
                    return;
                default:
            }
        }

        private void handleRekeyCreationFailure(int messageId, IkeProtocolException e) {
            loge("Received invalid Rekey IKE request. Reject with error notification", e);

            buildAndSendNotificationResponse(
                    mCurrentIkeSaRecord, messageId, e.buildNotifyPayload());
        }
    }

    /**
     * This class represents a state when there is at least one ongoing Child procedure
     * (Create/Rekey/Delete Child)
     *
     * <p>For a locally initiated Child procedure, this state is responsible for notifying Child
     * Session to initiate the exchange, building outbound request IkeMessage with Child Session
     * provided payload list and redirecting the inbound response to Child Session for validation.
     *
     * <p>For a remotely initiated Child procedure, this state is responsible for redirecting the
     * inbound request to Child Session(s) and building outbound response IkeMessage with Child
     * Session provided payload list. Exchange collision on a Child Session will be resolved inside
     * the Child Session.
     *
     * <p>For a remotely initiated IKE procedure, this state will only accept a Delete IKE request
     * and reject other types with TEMPORARY_FAILURE, since it causes conflict with the ongoing
     * Child procedure.
     *
     * <p>For most inbound request/response, this state will first pick out and handle IKE related
     * payloads and then send the rest of the payloads to Child Session for further validation. It
     * is the Child Session's responsibility to check required payloads (and verify the exchange
     * type) according to its procedure type. Only when receiving an inbound delete Child request,
     * as the only case where multiple Child Sessions will be affected by one IkeMessage, this state
     * will only send Delete Payload(s) to Child Session.
     */
    class ChildProcedureOngoing extends DeleteBase {
        // It is possible that mChildInLocalProcedure is also in mChildInRemoteProcedures when both
        // sides initiated exchange for the same Child Session.
        private ChildSessionStateMachine mChildInLocalProcedure;
        private Set<ChildSessionStateMachine> mChildInRemoteProcedures;

        private ChildLocalRequest mLocalRequestOngoing;

        private int mLastInboundRequestMsgId;
        private List<IkePayload> mOutboundRespPayloads;
        private Set<ChildSessionStateMachine> mAwaitingChildResponse;

        private EncryptedRetransmitter mRetransmitter;

        @Override
        public void enterState() {
            mChildInLocalProcedure = null;
            mChildInRemoteProcedures = new HashSet<>();

            mLocalRequestOngoing = null;

            mLastInboundRequestMsgId = 0;
            mOutboundRespPayloads = new LinkedList<>();
            mAwaitingChildResponse = new HashSet<>();
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_REQUEST_FOR_CHILD:
                    // Handle remote request (and do state transition)
                    handleRequestIkeMessage(
                            (IkeMessage) message.obj,
                            message.arg1 /*ikeExchangeSubType*/,
                            null /*ReceivedIkePacket*/);
                    return HANDLED;
                case CMD_OUTBOUND_CHILD_PAYLOADS_READY:
                    ChildOutboundData outboundData = (ChildOutboundData) message.obj;
                    int exchangeType = outboundData.exchangeType;
                    List<IkePayload> outboundPayloads = outboundData.payloadList;

                    if (outboundData.isResp) {
                        handleOutboundResponse(
                                exchangeType, outboundPayloads, outboundData.childSession);
                    } else {
                        handleOutboundRequest(exchangeType, outboundPayloads);
                    }

                    return HANDLED;
                case CMD_CHILD_PROCEDURE_FINISHED:
                    ChildSessionStateMachine childSession = (ChildSessionStateMachine) message.obj;

                    if (mChildInLocalProcedure == childSession) {
                        mChildInLocalProcedure = null;
                        mLocalRequestOngoing = null;
                    }
                    mChildInRemoteProcedures.remove(childSession);

                    transitionToIdleIfAllProceduresDone();
                    return HANDLED;
                case CMD_HANDLE_FIRST_CHILD_NEGOTIATION:
                    FirstChildNegotiationData childData = (FirstChildNegotiationData) message.obj;

                    mChildInLocalProcedure = getChildSession(childData.childSessionCallback);
                    if (mChildInLocalProcedure == null) {
                        cleanUpAndQuit(new IllegalStateException("First child not found."));
                        return HANDLED;
                    }

                    mChildInLocalProcedure.handleFirstChildExchange(
                            childData.reqPayloads,
                            childData.respPayloads,
                            mLocalAddress,
                            mRemoteAddress,
                            getEncapSocketOrNull(),
                            mIkePrf,
                            mSaProposal.getDhGroupTransforms()[0].id, // negotiated DH
                            mCurrentIkeSaRecord.getSkD());
                    return HANDLED;
                case CMD_EXECUTE_LOCAL_REQ:
                    executeLocalRequest((ChildLocalRequest) message.obj);
                    return HANDLED;
                case CMD_KILL_SESSION:
                    // If mChildInLocalProcedure is null, there are no unfinished locally initiated
                    // procedures. It is safe to notify the remote that the session is being
                    // deleted.
                    if (mChildInLocalProcedure == null) {
                        // The delete notification is sent as a best-effort, so don't worry about
                        // retransmitting.
                        sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
                    }

                    // Let KillIkeSessionParent handle the rest of the cleanup.
                    return NOT_HANDLED;
                default:
                    return super.processStateMessage(message);
            }
        }

        @Override
        protected void handleTempFailure() {
            // The ChildSessionStateMachine will be responsible for rescheduling the rejected
            // request.
            mTempFailHandler.handleTempFailure();
        }

        private void transitionToIdleIfAllProceduresDone() {
            if (mChildInLocalProcedure == null && mChildInRemoteProcedures.isEmpty()) {
                transitionTo(mIdle);
            }
        }

        private ChildSessionStateMachine getChildSession(ChildLocalRequest req) {
            if (req.childSessionCallback == null) {
                return mRemoteSpiToChildSessionMap.get(req.remoteSpi);
            }
            return getChildSession(req.childSessionCallback);
        }

        private ChildSessionStateMachine getChildSession(ChildSessionCallback callback) {
            synchronized (mChildCbToSessions) {
                return mChildCbToSessions.get(callback);
            }
        }

        // Returns the UDP-Encapsulation socket to the newly created ChildSessionStateMachine if
        // a NAT is detected or if NAT-T AND MOBIKE are enabled by both parties. It allows the
        // ChildSessionStateMachine to build IPsec transforms that can send and receive IPsec
        // traffic through a NAT.
        private UdpEncapsulationSocket getEncapSocketOrNull() {
            if (!(mIkeSocket instanceof IkeUdpEncapSocket)) {
                return null;
            }
            return ((IkeUdpEncapSocket) mIkeSocket).getUdpEncapsulationSocket();
        }

        private void executeLocalRequest(ChildLocalRequest req) {
            req.releaseWakeLock();
            mChildInLocalProcedure = getChildSession(req);
            mLocalRequestOngoing = req;

            if (mChildInLocalProcedure == null) {
                // This request has been validated to have a recognized target Child Session when
                // it was sent to IKE Session at the begginnig. Failing to find this Child Session
                // now means the Child creation has failed.
                logd(
                        "Child state machine not found for local request: "
                                + req.procedureType
                                + " Creation of Child Session may have been failed.");

                transitionToIdleIfAllProceduresDone();
                return;
            }
            switch (req.procedureType) {
                case CMD_LOCAL_REQUEST_CREATE_CHILD:
                    mChildInLocalProcedure.createChildSession(
                            mLocalAddress,
                            mRemoteAddress,
                            getEncapSocketOrNull(),
                            mIkePrf,
                            mSaProposal.getDhGroupTransforms()[0].id, // negotiated DH
                            mCurrentIkeSaRecord.getSkD());
                    break;
                case CMD_LOCAL_REQUEST_REKEY_CHILD:
                    mChildInLocalProcedure.rekeyChildSession();
                    break;
                case CMD_LOCAL_REQUEST_REKEY_CHILD_MOBIKE:
                    mChildInLocalProcedure.rekeyChildSessionForMobike(
                            mLocalAddress, mRemoteAddress, getEncapSocketOrNull());
                    break;
                case CMD_LOCAL_REQUEST_DELETE_CHILD:
                    mChildInLocalProcedure.deleteChildSession();
                    break;
                default:
                    cleanUpAndQuit(
                            new IllegalStateException(
                                    "Invalid Child procedure type: " + req.procedureType));
                    break;
            }
        }

        /**
         * This method is called when this state receives an inbound request or when mReceiving
         * received an inbound Child request and deferred it to this state.
         */
        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            // TODO: Grab a remote lock and hand payloads to the Child Session

            mLastInboundRequestMsgId = ikeMessage.ikeHeader.messageId;
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_CREATE_CHILD:
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_NO_ADDITIONAL_SAS);
                    break;
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    // Send response and quit state machine
                    handleDeleteSessionRequest(ikeMessage);

                    // Return immediately to avoid transitioning to mIdle
                    return;
                case IKE_EXCHANGE_SUBTYPE_DELETE_CHILD:
                    handleInboundDeleteChildRequest(ikeMessage);
                    break;
                case IKE_EXCHANGE_SUBTYPE_REKEY_IKE:
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
                    break;
                case IKE_EXCHANGE_SUBTYPE_REKEY_CHILD:
                    handleInboundRekeyChildRequest(ikeMessage);
                    break;
                case IKE_EXCHANGE_SUBTYPE_GENERIC_INFO:
                    handleGenericInfoRequest(ikeMessage);
                    break;
                default:
                    cleanUpAndQuit(
                            new IllegalStateException(
                                    "Invalid IKE exchange subtype: " + ikeExchangeSubType));
                    return;
            }
            transitionToIdleIfAllProceduresDone();
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            mRetransmitter.stopRetransmitting();

            List<IkePayload> handledPayloads = new LinkedList<>();

            for (IkePayload payload : ikeMessage.ikePayloadList) {
                switch (payload.payloadType) {
                    case PAYLOAD_TYPE_NOTIFY:
                        // TODO: Handle fatal IKE error notification and IKE status notification.
                        break;
                    case PAYLOAD_TYPE_VENDOR:
                        // TODO: Handle Vendor ID Payload
                        handledPayloads.add(payload);
                        break;
                    case PAYLOAD_TYPE_CP:
                        // TODO: Handle IKE related configuration attributes and pass the payload to
                        // Child to further handle internal IP address attributes.
                        break;
                    default:
                        break;
                }
            }

            List<IkePayload> payloads = new LinkedList<>();
            payloads.addAll(ikeMessage.ikePayloadList);
            payloads.removeAll(handledPayloads);

            mChildInLocalProcedure.receiveResponse(ikeMessage.ikeHeader.exchangeType, payloads);
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            mRetransmitter.stopRetransmitting();

            sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
            handleIkeFatalError(ikeException);
        }

        private void handleInboundDeleteChildRequest(IkeMessage ikeMessage) {
            // It is guaranteed in #getIkeExchangeSubType that at least one Delete Child Payload
            // exists.

            HashMap<ChildSessionStateMachine, List<IkePayload>> childToDelPayloadsMap =
                    new HashMap<>();
            Set<Integer> spiHandled = new HashSet<>();

            for (IkePayload payload : ikeMessage.ikePayloadList) {
                switch (payload.payloadType) {
                    case PAYLOAD_TYPE_VENDOR:
                        // TODO: Investigate if Vendor ID Payload can be in an INFORMATIONAL
                        // message.
                        break;
                    case PAYLOAD_TYPE_NOTIFY:
                        logw(
                                "Unexpected or unknown notification: "
                                        + ((IkeNotifyPayload) payload).notifyType);
                        break;
                    case PAYLOAD_TYPE_DELETE:
                        IkeDeletePayload delPayload = (IkeDeletePayload) payload;

                        for (int spi : delPayload.spisToDelete) {
                            ChildSessionStateMachine child = mRemoteSpiToChildSessionMap.get(spi);
                            if (child == null) {
                                // TODO: Investigate how other implementations handle that.
                                logw("Child SA not found with received SPI: " + spi);
                            } else if (!spiHandled.add(spi)) {
                                logw("Received repeated Child SPI: " + spi);
                            } else {
                                // Store Delete Payload with its target ChildSession
                                if (!childToDelPayloadsMap.containsKey(child)) {
                                    childToDelPayloadsMap.put(child, new LinkedList<>());
                                }
                                List<IkePayload> delPayloads = childToDelPayloadsMap.get(child);

                                // Avoid storing repeated Delete Payload
                                if (!delPayloads.contains(delPayload)) delPayloads.add(delPayload);
                            }
                        }

                        break;
                    case PAYLOAD_TYPE_CP:
                        // TODO: Handle it
                        break;
                    default:
                        logw("Unexpected payload types found: " + payload.payloadType);
                }
            }

            // If no Child SA is found, only reply with IKE related payloads or an empty
            // message
            if (childToDelPayloadsMap.isEmpty()) {
                logd("No Child SA is found for this request.");
                sendEncryptedIkeMessage(
                        buildEncryptedInformationalMessage(
                                new IkeInformationalPayload[0],
                                true /*isResp*/,
                                ikeMessage.ikeHeader.messageId));
                return;
            }

            // Send Delete Payloads to Child Sessions
            for (ChildSessionStateMachine child : childToDelPayloadsMap.keySet()) {
                child.receiveRequest(
                        IKE_EXCHANGE_SUBTYPE_DELETE_CHILD,
                        EXCHANGE_TYPE_INFORMATIONAL,
                        childToDelPayloadsMap.get(child));
                mAwaitingChildResponse.add(child);
                mChildInRemoteProcedures.add(child);
            }
        }

        private void handleInboundRekeyChildRequest(IkeMessage ikeMessage) {
            // It is guaranteed in #getIkeExchangeSubType that at least one Notify-Rekey Child
            // Payload exists.
            List<IkePayload> handledPayloads = new LinkedList<>();
            ChildSessionStateMachine targetChild = null;
            Set<Integer> unrecognizedSpis = new HashSet<>();

            for (IkePayload payload : ikeMessage.ikePayloadList) {
                switch (payload.payloadType) {
                    case PAYLOAD_TYPE_VENDOR:
                        // TODO: Handle it.
                        handledPayloads.add(payload);
                        break;
                    case PAYLOAD_TYPE_NOTIFY:
                        IkeNotifyPayload notifyPayload = (IkeNotifyPayload) payload;
                        if (NOTIFY_TYPE_REKEY_SA != notifyPayload.notifyType) break;

                        int childSpi = notifyPayload.spi;
                        ChildSessionStateMachine child = mRemoteSpiToChildSessionMap.get(childSpi);

                        if (child == null) {
                            // Remember unrecognized SPIs and reply error notification if no
                            // recognized SPI found.
                            unrecognizedSpis.add(childSpi);
                            logw("Child SA not found with received SPI: " + childSpi);
                        } else if (targetChild == null) {
                            // Each message should have only one Notify-Rekey Payload. If there are
                            // multiple of them, we only process the first valid one and ignore
                            // others.
                            targetChild = mRemoteSpiToChildSessionMap.get(childSpi);
                        } else {
                            logw("More than one Notify-Rekey Payload found with SPI: " + childSpi);
                            handledPayloads.add(notifyPayload);
                        }
                        break;
                    case PAYLOAD_TYPE_CP:
                        // TODO: Handle IKE related configuration attributes and pass the payload to
                        // Child to further handle internal IP address attributes.
                        break;
                    default:
                        break;
                }
            }

            // Reject request with error notification.
            if (targetChild == null) {
                IkeInformationalPayload[] errorPayloads =
                        new IkeInformationalPayload[unrecognizedSpis.size()];
                int i = 0;
                for (Integer spi : unrecognizedSpis) {
                    errorPayloads[i++] =
                            new IkeNotifyPayload(
                                    IkePayload.PROTOCOL_ID_ESP,
                                    spi,
                                    ERROR_TYPE_CHILD_SA_NOT_FOUND,
                                    new byte[0]);
                }

                IkeMessage msg =
                        buildEncryptedNotificationMessage(
                                mCurrentIkeSaRecord,
                                errorPayloads,
                                EXCHANGE_TYPE_INFORMATIONAL,
                                true /*isResponse*/,
                                ikeMessage.ikeHeader.messageId);

                sendEncryptedIkeMessage(mCurrentIkeSaRecord, msg);
                return;
            }

            // Normal path
            List<IkePayload> payloads = new LinkedList<>();
            payloads.addAll(ikeMessage.ikePayloadList);
            payloads.removeAll(handledPayloads);

            mAwaitingChildResponse.add(targetChild);
            mChildInRemoteProcedures.add(targetChild);

            targetChild.receiveRequest(
                    IKE_EXCHANGE_SUBTYPE_REKEY_CHILD, ikeMessage.ikeHeader.exchangeType, payloads);
        }

        private void handleOutboundRequest(int exchangeType, List<IkePayload> outboundPayloads) {
            IkeHeader ikeHeader =
                    new IkeHeader(
                            mCurrentIkeSaRecord.getInitiatorSpi(),
                            mCurrentIkeSaRecord.getResponderSpi(),
                            IkePayload.PAYLOAD_TYPE_SK,
                            exchangeType,
                            false /*isResp*/,
                            mCurrentIkeSaRecord.isLocalInit,
                            mCurrentIkeSaRecord.getLocalRequestMessageId());
            IkeMessage ikeMessage = new IkeMessage(ikeHeader, outboundPayloads);

            mRetransmitter = new EncryptedRetransmitter(ikeMessage);
        }

        private void handleOutboundResponse(
                int exchangeType,
                List<IkePayload> outboundPayloads,
                ChildSessionStateMachine childSession) {
            // For each request IKE passed to Child, Child will send back to IKE a response. Even
            // if the Child Session is under simultaneous deletion, it will send back an empty
            // payload list.
            mOutboundRespPayloads.addAll(outboundPayloads);
            mAwaitingChildResponse.remove(childSession);

            // When the server tries to delete multiple Child Sessions in one IKE exchange,
            // mAwaitingChildResponse may not be empty. It means that there are Child Sessions
            // have not sent IKE Session the delete responses. In this case IKE Session needs to
            // return and keep waiting for all the Child responses in this state.
            if (!mAwaitingChildResponse.isEmpty()) return;

            IkeHeader ikeHeader =
                    new IkeHeader(
                            mCurrentIkeSaRecord.getInitiatorSpi(),
                            mCurrentIkeSaRecord.getResponderSpi(),
                            IkePayload.PAYLOAD_TYPE_SK,
                            exchangeType,
                            true /*isResp*/,
                            mCurrentIkeSaRecord.isLocalInit,
                            mLastInboundRequestMsgId);
            IkeMessage ikeMessage = new IkeMessage(ikeHeader, mOutboundRespPayloads);
            sendEncryptedIkeMessage(ikeMessage);

            // Clear mOutboundRespPayloads so that in a two-exchange process (e.g. Rekey Child), the
            // response of the first exchange won't be added to the response of the second exchange.
            mOutboundRespPayloads.clear();
        }
    }

    /** CreateIkeLocalIkeInit represents state when IKE library initiates IKE_INIT exchange. */
    @VisibleForTesting
    public class CreateIkeLocalIkeInit extends BusyState {
        private IkeSecurityParameterIndex mLocalIkeSpiResource;
        private IkeSecurityParameterIndex mRemoteIkeSpiResource;
        private Retransmitter mRetransmitter;

        // TODO: Support negotiating IKE fragmentation

        @Override
        public void enterState() {
            try {
                sendRequest(buildIkeInitReq());
            } catch (IOException e) {
                // Fail to assign IKE SPI
                handleIkeFatalError(e);
            }
        }

        private void sendRequest(IkeMessage request) {
            // Register local SPI to receive the IKE INIT response.
            mIkeSocket.registerIke(request.ikeHeader.ikeInitiatorSpi, IkeSessionStateMachine.this);

            mIkeInitRequestBytes = request.encode();
            mIkeInitNoncePayload =
                    request.getPayloadForType(IkePayload.PAYLOAD_TYPE_NONCE, IkeNoncePayload.class);

            if (mRetransmitter != null) {
                mRetransmitter.stopRetransmitting();
            }
            mRetransmitter = new UnencryptedRetransmitter(request);
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_IKE_PACKET:
                    handleReceivedIkePacket(message);
                    return HANDLED;

                case CMD_SET_NETWORK:
                    // Shouldn't be receiving this command before MOBIKE is active - determined with
                    // last IKE_AUTH response
                    logWtf("Received SET_NETWORK cmd in " + getCurrentState().getName());
                    return NOT_HANDLED;

                default:
                    return super.processStateMessage(message);
            }
        }

        protected void handleReceivedIkePacket(Message message) {
            String methodTag = "handleReceivedIkePacket: ";

            ReceivedIkePacket receivedIkePacket = (ReceivedIkePacket) message.obj;
            IkeHeader ikeHeader = receivedIkePacket.ikeHeader;
            byte[] ikePacketBytes = receivedIkePacket.ikePacketBytes;

            logd(
                    methodTag
                            + "Received an "
                            + ikeHeader.getBasicInfoString()
                            + ". Packet size: "
                            + ikePacketBytes.length);

            if (ikeHeader.isResponseMsg) {
                DecodeResult decodeResult = IkeMessage.decode(0, ikeHeader, ikePacketBytes);

                switch (decodeResult.status) {
                    case DECODE_STATUS_OK:
                        handleResponseIkeMessage(((DecodeResultOk) decodeResult).ikeMessage);
                        mIkeInitResponseBytes = ikePacketBytes;

                        // SA negotiation failed
                        if (mCurrentIkeSaRecord == null) break;

                        mCurrentIkeSaRecord.incrementLocalRequestMessageId();
                        break;
                    case DECODE_STATUS_PARTIAL:
                        // Fall through. We don't support IKE fragmentation here. We should never
                        // get this status.
                    case DECODE_STATUS_PROTECTED_ERROR:
                        // IKE INIT response is not protected. So we should never get this status
                        cleanUpAndQuit(
                                new IllegalStateException(
                                        "Unexpected decoding status: " + decodeResult.status));
                        break;
                    case DECODE_STATUS_UNPROTECTED_ERROR:
                        logi(
                                "Discard unencrypted response with syntax error",
                                ((DecodeResultError) decodeResult).ikeException);
                        break;
                    default:
                        cleanUpAndQuit(
                                new IllegalStateException(
                                        "Invalid decoding status: " + decodeResult.status));
                }

            } else {
                // TODO: Also prettyprint IKE header in the log.
                logi("Received a request while waiting for IKE_INIT response. Discard it.");
            }
        }

        /** Returns the Notify-Cookie payload, or null if it does not exist */
        private IkeNotifyPayload getNotifyCookie(IkeMessage ikeMessage) {
            List<IkeNotifyPayload> notifyPayloads =
                    ikeMessage.getPayloadListForType(PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);
            for (IkeNotifyPayload notify : notifyPayloads) {
                if (notify.notifyType == NOTIFY_TYPE_COOKIE) {
                    return notify;
                }
            }
            return null;
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            // IKE_SA_INIT exchange and IKE SA setup succeed
            boolean ikeInitSuccess = false;

            // IKE INIT is not finished. IKE_SA_INIT request was re-sent with Notify-Cookie,
            // and the same INIT SPI and other payloads.
            boolean ikeInitRetriedWithCookie = false;

            try {
                int exchangeType = ikeMessage.ikeHeader.exchangeType;
                if (exchangeType != IkeHeader.EXCHANGE_TYPE_IKE_SA_INIT) {
                    throw new InvalidSyntaxException(
                            "Expected EXCHANGE_TYPE_IKE_SA_INIT but received: " + exchangeType);
                }

                // Retry IKE INIT if there is Notify-Cookie
                IkeNotifyPayload inCookiePayload = getNotifyCookie(ikeMessage);
                if (inCookiePayload != null) {
                    IkeNotifyPayload outCookiePayload =
                            IkeNotifyPayload.handleCookieAndGenerateCopy(inCookiePayload);
                    IkeMessage initReq =
                            buildReqWithCookie(mRetransmitter.getMessage(), outCookiePayload);

                    sendRequest(initReq);
                    ikeInitRetriedWithCookie = true;
                    return;
                }

                // Negotiate IKE SA
                validateIkeInitResp(mRetransmitter.getMessage(), ikeMessage);

                mCurrentIkeSaRecord =
                        IkeSaRecord.makeFirstIkeSaRecord(
                                mRetransmitter.getMessage(),
                                ikeMessage,
                                mLocalIkeSpiResource,
                                mRemoteIkeSpiResource,
                                mIkePrf,
                                mIkeIntegrity == null ? 0 : mIkeIntegrity.getKeyLength(),
                                mIkeCipher.getKeyLength(),
                                buildSaLifetimeAlarmScheduler(mRemoteIkeSpiResource.getSpi()));

                addIkeSaRecord(mCurrentIkeSaRecord);
                ikeInitSuccess = true;

                transitionTo(mCreateIkeLocalIkeAuth);
            } catch (IkeProtocolException | GeneralSecurityException | IOException e) {
                if (e instanceof InvalidKeException) {
                    InvalidKeException keException = (InvalidKeException) e;

                    int requestedDhGroup = keException.getDhGroup();
                    boolean doAllProposalsHaveDhGroup = true;
                    for (IkeSaProposal proposal : mIkeSessionParams.getSaProposalsInternal()) {
                        doAllProposalsHaveDhGroup &=
                                proposal.getDhGroups().contains(requestedDhGroup);
                    }

                    // If DH group is not acceptable for all proposals, fail. The caller explicitly
                    // did not want that combination, and the IKE library must honor it.
                    if (doAllProposalsHaveDhGroup) {
                        mPeerSelectedDhGroup = requestedDhGroup;

                        // Remove state set during request creation
                        mIkeSocket.unregisterIke(
                                mRetransmitter.getMessage().ikeHeader.ikeInitiatorSpi);
                        mIkeInitRequestBytes = null;
                        mIkeInitNoncePayload = null;

                        transitionTo(mInitial);
                        openSession();

                        return;
                    }
                }

                handleIkeFatalError(e);
            } finally {
                if (!ikeInitSuccess && !ikeInitRetriedWithCookie) {
                    if (mLocalIkeSpiResource != null) {
                        mLocalIkeSpiResource.close();
                        mLocalIkeSpiResource = null;
                    }
                    if (mRemoteIkeSpiResource != null) {
                        mRemoteIkeSpiResource.close();
                        mRemoteIkeSpiResource = null;
                    }
                }
            }
        }

        private IkeMessage buildIkeInitReq() throws IOException {
            // Generate IKE SPI
            mLocalIkeSpiResource = mIkeSpiGenerator.allocateSpi(mLocalAddress);

            long initSpi = mLocalIkeSpiResource.getSpi();
            long respSpi = 0;

            // It is validated in IkeSessionParams.Builder to ensure IkeSessionParams has at least
            // one IkeSaProposal and all SaProposals are valid for IKE SA negotiation.
            IkeSaProposal[] saProposals = mIkeSessionParams.getSaProposalsInternal();
            List<IkePayload> payloadList =
                    CreateIkeSaHelper.getIkeInitSaRequestPayloads(
                            saProposals,
                            mPeerSelectedDhGroup,
                            initSpi,
                            respSpi,
                            mLocalAddress,
                            mRemoteAddress,
                            mLocalPort,
                            mIkeSocket.getIkeServerPort(),
                            mRandomFactory,
                            mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE));
            payloadList.add(
                    new IkeNotifyPayload(
                            IkeNotifyPayload.NOTIFY_TYPE_IKEV2_FRAGMENTATION_SUPPORTED));

            ByteBuffer signatureHashAlgoTypes =
                    ByteBuffer.allocate(
                            IkeAuthDigitalSignPayload.ALL_SIGNATURE_ALGO_TYPES.length * 2);
            for (short type : IkeAuthDigitalSignPayload.ALL_SIGNATURE_ALGO_TYPES) {
                signatureHashAlgoTypes.putShort(type);
            }
            payloadList.add(
                    new IkeNotifyPayload(
                            IkeNotifyPayload.NOTIFY_TYPE_SIGNATURE_HASH_ALGORITHMS,
                            signatureHashAlgoTypes.array()));

            // TODO: Add Notification Payloads according to user configurations.

            // Build IKE header
            IkeHeader ikeHeader =
                    new IkeHeader(
                            initSpi,
                            respSpi,
                            IkePayload.PAYLOAD_TYPE_SA,
                            IkeHeader.EXCHANGE_TYPE_IKE_SA_INIT,
                            false /*isResponseMsg*/,
                            true /*fromIkeInitiator*/,
                            0 /*messageId*/);

            return new IkeMessage(ikeHeader, payloadList);
        }

        /**
         * Builds an IKE INIT request that has the same payloads and SPI with the original request,
         * and with the new Notify-Cookie Payload as the first payload.
         */
        private IkeMessage buildReqWithCookie(
                IkeMessage originalReq, IkeNotifyPayload cookieNotify) {
            List<IkePayload> payloads = new ArrayList<>();

            // Notify-Cookie MUST be the first payload.
            payloads.add(cookieNotify);

            for (IkePayload payload : originalReq.ikePayloadList) {
                // Keep all previous payloads except COOKIEs
                if (payload instanceof IkeNotifyPayload
                        && ((IkeNotifyPayload) payload).notifyType == NOTIFY_TYPE_COOKIE) {
                    continue;
                }
                payloads.add(payload);
            }

            IkeHeader originalHeader = originalReq.ikeHeader;
            IkeHeader header =
                    new IkeHeader(
                            originalHeader.ikeInitiatorSpi,
                            originalHeader.ikeResponderSpi,
                            PAYLOAD_TYPE_NOTIFY,
                            IkeHeader.EXCHANGE_TYPE_IKE_SA_INIT,
                            false /* isResponseMsg */,
                            true /* fromIkeInitiator */,
                            0 /* messageId */);
            return new IkeMessage(header, payloads);
        }

        private void validateIkeInitResp(IkeMessage reqMsg, IkeMessage respMsg)
                throws IkeProtocolException, IOException {
            IkeHeader respIkeHeader = respMsg.ikeHeader;
            mRemoteIkeSpiResource =
                    mIkeSpiGenerator.allocateSpi(mRemoteAddress, respIkeHeader.ikeResponderSpi);

            IkeSaPayload respSaPayload = null;
            IkeKePayload respKePayload = null;

            /**
             * There MAY be multiple NAT_DETECTION_SOURCE_IP payloads in a message if the sender
             * does not know which of several network attachments will be used to send the packet.
             */
            List<IkeNotifyPayload> natSourcePayloads = new LinkedList<>();
            IkeNotifyPayload natDestPayload = null;

            boolean hasNoncePayload = false;

            for (IkePayload payload : respMsg.ikePayloadList) {
                switch (payload.payloadType) {
                    case IkePayload.PAYLOAD_TYPE_SA:
                        respSaPayload = (IkeSaPayload) payload;
                        break;
                    case IkePayload.PAYLOAD_TYPE_KE:
                        respKePayload = (IkeKePayload) payload;
                        break;
                    case IkePayload.PAYLOAD_TYPE_CERT_REQUEST:
                        // Certificates unconditionally sent (only) for Digital Signature Auth
                        break;
                    case IkePayload.PAYLOAD_TYPE_NONCE:
                        hasNoncePayload = true;
                        mIkeRespNoncePayload = (IkeNoncePayload) payload;
                        break;
                    case IkePayload.PAYLOAD_TYPE_VENDOR:
                        mRemoteVendorIds.add(((IkeVendorPayload) payload).vendorId);
                        break;
                    case IkePayload.PAYLOAD_TYPE_NOTIFY:
                        IkeNotifyPayload notifyPayload = (IkeNotifyPayload) payload;

                        if (notifyPayload.isErrorNotify()) {
                            throw notifyPayload.validateAndBuildIkeException();
                        }

                        switch (notifyPayload.notifyType) {
                            case NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP:
                                natSourcePayloads.add(notifyPayload);
                                break;
                            case NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP:
                                if (natDestPayload != null) {
                                    throw new InvalidSyntaxException(
                                            "More than one"
                                                    + " NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP"
                                                    + " found");
                                }
                                natDestPayload = notifyPayload;
                                break;
                            case NOTIFY_TYPE_IKEV2_FRAGMENTATION_SUPPORTED:
                                mSupportFragment = true;
                                mEnabledExtensions.add(EXTENSION_TYPE_FRAGMENTATION);
                                break;
                            case NOTIFY_TYPE_SIGNATURE_HASH_ALGORITHMS:
                                mPeerSignatureHashAlgorithms =
                                        IkeAuthDigitalSignPayload
                                                .getSignatureHashAlgorithmsFromIkeNotifyPayload(
                                                        notifyPayload);
                                break;
                            default:
                                // Unknown and unexpected status notifications are ignored as per
                                // RFC7296.
                                logw(
                                        "Received unknown or unexpected status notifications with"
                                                + " notify type: "
                                                + notifyPayload.notifyType);
                        }

                        break;
                    default:
                        logw(
                                "Received unexpected payload in IKE INIT response. Payload type: "
                                        + payload.payloadType);
                }
            }

            if (respSaPayload == null
                    || respKePayload == null
                    || !hasNoncePayload) {
                throw new InvalidSyntaxException("SA, KE, or Nonce payload missing.");
            }

            IkeSaPayload reqSaPayload =
                    reqMsg.getPayloadForType(IkePayload.PAYLOAD_TYPE_SA, IkeSaPayload.class);
            mSaProposal =
                    IkeSaPayload.getVerifiedNegotiatedIkeProposalPair(
                                    reqSaPayload, respSaPayload, mIkeSpiGenerator, mRemoteAddress)
                            .second
                            .saProposal;

            // Build IKE crypto tools using mSaProposal. It is ensured that mSaProposal is valid and
            // has exactly one Transform for each Transform type. Only exception is when
            // combined-mode cipher is used, there will be either no integrity algorithm or an
            // INTEGRITY_ALGORITHM_NONE type algorithm.
            mIkeCipher = IkeCipher.create(mSaProposal.getEncryptionTransforms()[0]);
            if (!mIkeCipher.isAead()) {
                mIkeIntegrity = IkeMacIntegrity.create(mSaProposal.getIntegrityTransforms()[0]);
            }
            mIkePrf = IkeMacPrf.create(mSaProposal.getPrfTransforms()[0]);

            IkeKePayload reqKePayload =
                    reqMsg.getPayloadForType(IkePayload.PAYLOAD_TYPE_KE, IkeKePayload.class);
            if (reqKePayload.dhGroup != respKePayload.dhGroup
                    && respKePayload.dhGroup != mPeerSelectedDhGroup) {
                throw new InvalidSyntaxException("Received KE payload with mismatched DH group.");
            }

            if (reqMsg.hasNotifyPayload(NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP)) {
                handleNatDetection(respMsg, natSourcePayloads, natDestPayload);
            }
        }

        private void handleNatDetection(
                IkeMessage respMsg,
                List<IkeNotifyPayload> natSourcePayloads,
                IkeNotifyPayload natDestPayload)
                throws InvalidSyntaxException, IOException {
            mHasCheckedNattSupport = true;

            if (!didPeerIncludeNattDetectionPayloads(natSourcePayloads, natDestPayload)) {
                mSupportNatTraversal = false;
                return;
            }

            mSupportNatTraversal = true;

            // NAT detection
            long initIkeSpi = respMsg.ikeHeader.ikeInitiatorSpi;
            long respIkeSpi = respMsg.ikeHeader.ikeResponderSpi;

            updateLocalAndRemoteNatDetected(
                    initIkeSpi, respIkeSpi, natSourcePayloads, natDestPayload);

            if (mLocalNatDetected || mRemoteNatDetected) {
                logd("Switching to send to remote port 4500 if it's not already");
                boolean isIpv4 = mRemoteAddress instanceof Inet4Address;

                try {
                    IkeSocket newSocket = getIkeSocket(isIpv4, true /* useEncapPort */);
                    if (newSocket == mIkeSocket) {
                        // Attempting to switch to current socket - ignore.
                        return;
                    }
                    // TODO(b/186900683): use getAndSwitchToIkeSocket here instead
                    switchToIkeSocket(initIkeSpi, newSocket);
                    mLocalPort = mIkeSocket.getLocalPort();

                    if (isIpv4) {
                        mIkeNattKeepalive = buildAndStartNattKeepalive();
                    }
                } catch (ErrnoException | IOException | ResourceUnavailableException e) {
                    handleIkeFatalError(e);
                }
            }
        }

        /** Switch to a new IKE socket due to NAT detection */
        private void switchToIkeSocket(long localSpi, IkeSocket newSocket) {
            migrateSpiToIkeSocket(localSpi, mIkeSocket, newSocket);
            mIkeSocket.releaseReference(IkeSessionStateMachine.this);
            mIkeSocket = newSocket;
        }

        @Override
        public void exitState() {
            super.exitState();

            if (mRetransmitter != null) {
                mRetransmitter.stopRetransmitting();
            }
        }

        private class UnencryptedRetransmitter extends Retransmitter {
            private UnencryptedRetransmitter(IkeMessage msg) {
                super(getHandler(), msg, mIkeSessionParams.getRetransmissionTimeoutsMillis());

                retransmit();
            }

            @Override
            public void send(IkeMessage msg) {
                // Sends unencrypted
                mIkeSocket.sendIkePacket(msg.encode(), mRemoteAddress);
            }

            @Override
            public void handleRetransmissionFailure() {
                handleIkeFatalError(new IOException("Retransmitting IKE INIT request failure"));
            }
        }
    }

    /**
     * Returns if the peer included NAT-T detection payloads
     *
     * @throws InvalidSyntaxException if an invalid combination of NAT-T detection payloads are
     *     received.
     */
    private boolean didPeerIncludeNattDetectionPayloads(
            List<IkeNotifyPayload> natSourcePayloads, IkeNotifyPayload natDestPayload)
            throws InvalidSyntaxException {
        if (!natSourcePayloads.isEmpty() && natDestPayload != null) {
            return true;
        } else if (natSourcePayloads.isEmpty() && natDestPayload == null) {
            return false;
        } else {
            throw new InvalidSyntaxException(
                    "Missing source or destination NAT detection notification");
        }
    }

    /**
     * Updates whether the local or remote peer are behind NATs. Assumes that mRemoteAddress is an
     * IPv4 address.
     */
    private void updateLocalAndRemoteNatDetected(
            long initIkeSpi,
            long respIkeSpi,
            List<IkeNotifyPayload> natSourcePayloads,
            IkeNotifyPayload natDestPayload) {
        // Check if local node is behind NAT
        byte[] expectedLocalNatData =
                IkeNotifyPayload.generateNatDetectionData(
                        initIkeSpi, respIkeSpi, mLocalAddress, mLocalPort);
        mLocalNatDetected = !Arrays.equals(expectedLocalNatData, natDestPayload.notifyData);

        // Check if the remote node is behind NAT
        byte[] expectedRemoteNatData =
                IkeNotifyPayload.generateNatDetectionData(
                        initIkeSpi, respIkeSpi, mRemoteAddress, mIkeSocket.getIkeServerPort());
        mRemoteNatDetected = true;
        for (IkeNotifyPayload natPayload : natSourcePayloads) {
            // If none of the received hash matches the expected value, the remote node is
            // behind NAT.
            if (Arrays.equals(expectedRemoteNatData, natPayload.notifyData)) {
                mRemoteNatDetected = false;
            }
        }
    }

    /** Starts NAT-T keepalive for current IkeUdpEncapSocket */
    private IkeNattKeepalive buildAndStartNattKeepalive() throws IOException {
        if (!(mIkeSocket instanceof IkeUdpEncapSocket)) {
            throw new IllegalStateException(
                    "Cannot start NAT-T keepalive when IKE Session is not using UDP Encap socket");
        }

        PendingIntent keepaliveIntent =
                buildIkeAlarmIntent(
                        mContext,
                        ACTION_KEEPALIVE,
                        getIntentIdentifier(),
                        obtainMessage(CMD_ALARM_FIRED, mIkeSessionId, CMD_SEND_KEEPALIVE));

        IkeNattKeepalive keepalive =
                new IkeNattKeepalive(
                        mContext,
                        mConnectivityManager,
                        mIkeSessionParams.getNattKeepAliveDelaySeconds(),
                        (Inet4Address) mLocalAddress,
                        (Inet4Address) mRemoteAddress,
                        ((IkeUdpEncapSocket) mIkeSocket).getUdpEncapsulationSocket(),
                        mIkeSocket.getIkeSocketConfig().getNetwork(),
                        keepaliveIntent);
        keepalive.start();
        return keepalive;
    }

    /**
     * CreateIkeLocalIkeAuthBase represents the common state and functionality required to perform
     * IKE AUTH exchanges in both the EAP and non-EAP flows.
     */
    abstract class CreateIkeLocalIkeAuthBase extends DeleteBase {
        protected Retransmitter mRetransmitter;

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        // TODO: b/139482382 If receiving a remote request while waiting for the last IKE AUTH
        // response, defer it to next state.

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            IkeSaRecord ikeSaRecord = getIkeSaRecordForPacket(ikeMessage.ikeHeader);

            // Null out last received packet, so the next state (that handles the actual request)
            // does not treat the message as a retransmission.
            ikeSaRecord.updateLastReceivedReqFirstPacket(null);

            // Send to next state; we can't handle this yet.
            deferMessage(message);
        }

        protected IkeMessage buildIkeAuthReqMessage(List<IkePayload> payloadList) {
            // Build IKE header
            IkeHeader ikeHeader =
                    new IkeHeader(
                            mCurrentIkeSaRecord.getInitiatorSpi(),
                            mCurrentIkeSaRecord.getResponderSpi(),
                            IkePayload.PAYLOAD_TYPE_SK,
                            IkeHeader.EXCHANGE_TYPE_IKE_AUTH,
                            false /*isResponseMsg*/,
                            true /*fromIkeInitiator*/,
                            mCurrentIkeSaRecord.getLocalRequestMessageId());

            return new IkeMessage(ikeHeader, payloadList);
        }

        protected void authenticatePsk(
                byte[] psk, IkeAuthPayload authPayload, IkeIdPayload respIdPayload)
                throws AuthenticationFailedException {
            if (authPayload.authMethod != IkeAuthPayload.AUTH_METHOD_PRE_SHARED_KEY) {
                throw new AuthenticationFailedException(
                        "Expected the remote/server to use PSK-based authentication but"
                                + " they used: "
                                + authPayload.authMethod);
            }

            IkeAuthPskPayload pskPayload = (IkeAuthPskPayload) authPayload;
            pskPayload.verifyInboundSignature(
                    psk,
                    mIkeInitResponseBytes,
                    mCurrentIkeSaRecord.nonceInitiator,
                    respIdPayload.getEncodedPayloadBody(),
                    mIkePrf,
                    mCurrentIkeSaRecord.getSkPr());
        }

        protected List<IkePayload> extractChildPayloadsFromMessage(IkeMessage ikeMessage)
                throws InvalidSyntaxException {
            List<IkePayload> list = new LinkedList<>();
            for (IkePayload payload : ikeMessage.ikePayloadList) {
                switch (payload.payloadType) {
                    case PAYLOAD_TYPE_SA: // fall through
                    case PAYLOAD_TYPE_TS_INITIATOR: // fall through
                    case PAYLOAD_TYPE_TS_RESPONDER: // fall through
                    case PAYLOAD_TYPE_CP:
                        list.add(payload);
                        break;
                    case PAYLOAD_TYPE_NOTIFY:
                        if (((IkeNotifyPayload) payload).isNewChildSaNotify()) {
                            list.add(payload);
                        }
                        break;
                    default:
                        // Ignore payloads unrelated with Child negotiation
                }
            }

            // Payload validation is done in ChildSessionStateMachine
            return list;
        }

        protected void performFirstChildNegotiation(
                List<IkePayload> childReqList, List<IkePayload> childRespList) {
            childReqList.add(mIkeInitNoncePayload);
            childRespList.add(mIkeRespNoncePayload);

            deferMessage(
                    obtainMessage(
                            CMD_HANDLE_FIRST_CHILD_NEGOTIATION,
                            new FirstChildNegotiationData(
                                    mFirstChildSessionParams,
                                    mFirstChildCallbacks,
                                    childReqList,
                                    childRespList)));

            transitionTo(mChildProcedureOngoing);
        }

        // TODO: b/177434707 Calling IkeSessionConnectionInfo constructor is safe because it does
        // not depend on any platform API added after SDK R. Handle this case in a mainline standard
        // way when b/177434707 is fixed.
        @SuppressLint("NewApi")
        protected IkeSessionConfiguration buildIkeSessionConfiguration(IkeMessage ikeMessage) {
            IkeConfigPayload configPayload =
                    ikeMessage.getPayloadForType(
                            IkePayload.PAYLOAD_TYPE_CP, IkeConfigPayload.class);
            if (configPayload == null) {
                logi("No config payload in ikeMessage.");
            } else if (configPayload.configType != CONFIG_TYPE_REPLY) {
                logi("Unexpected config payload. Config Type: " + configPayload.configType);
                configPayload = null;
            }

            IkeSessionConnectionInfo ikeConnInfo =
                    new IkeSessionConnectionInfo(mLocalAddress, mRemoteAddress, mNetwork);

            return new IkeSessionConfiguration(
                    ikeConnInfo, configPayload, mRemoteVendorIds, mEnabledExtensions);
        }

        protected void notifyIkeSessionSetup(IkeMessage msg) {
            IkeSessionConfiguration ikeSessionConfig = buildIkeSessionConfiguration(msg);
            executeUserCallback(
                    () -> {
                        mIkeSessionCallback.onOpened(ikeSessionConfig);
                    });
        }

        protected void handleNotifyInLastAuthResp(
                IkeNotifyPayload notifyPayload, IkeAuthPayload authPayload) throws IkeException {
            if (notifyPayload.isErrorNotify()) {
                if (notifyPayload.isNewChildSaNotify() && authPayload != null) {
                    // If error is for creating Child and Auth payload is included, try
                    // to do authentication first and let ChildSessionStateMachine
                    // handle the error later.
                    return;
                } else {
                    throw notifyPayload.validateAndBuildIkeException();
                }
            } else if (notifyPayload.isNewChildSaNotify()) {
                // If payload is not an error but is for the new Child, it's reasonable
                // to receive here. Let the ChildSessionStateMachine handle it.
                return;
            } else if (mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)
                    && notifyPayload.notifyType == NOTIFY_TYPE_MOBIKE_SUPPORTED) {
                mSupportMobike = true;
                mEnabledExtensions.add(EXTENSION_TYPE_MOBIKE);
                return;
            } else {
                // Unknown and unexpected status notifications are ignored as per
                // RFC7296.
                logw(
                        "Received unknown or unexpected status notifications with"
                                + " notify type: "
                                + notifyPayload.notifyType);
            }
        }

        protected void setUpMobilityHandling() throws IkeException {
            try {
                if (mIkeSessionParams.getConfiguredNetwork() != null) {
                    // Caller configured a specific Network - track it
                    // ConnectivityManager does not provide a callback for tracking a specific
                    // Network. In order to do so, create a NetworkRequest without any
                    // capabilities so it will match all Networks. The NetworkCallback will then
                    // filter for the correct (caller-specified) Network.
                    NetworkRequest request =
                            new NetworkRequest.Builder().clearCapabilities().build();
                    mNetworkCallback =
                            new IkeSpecificNetworkCallback(
                                    IkeSessionStateMachine.this, mNetwork, mLocalAddress);
                    mConnectivityManager.registerNetworkCallback(
                            request, mNetworkCallback, getHandler());
                } else {
                    // Caller did not configure a specific Network - track the default
                    mNetworkCallback =
                            new IkeDefaultNetworkCallback(
                                    IkeSessionStateMachine.this, mNetwork, mLocalAddress);
                    mConnectivityManager.registerDefaultNetworkCallback(
                            mNetworkCallback, getHandler());
                }
            } catch (RuntimeException e) {
                // Error occurred while registering the NetworkCallback
                throw new IkeInternalException("Error while registering NetworkCallback", e);
            }

            // Switch to port 4500 if NAT-T is supported (whether or not mobility is done via MOBIKE
            // or Rekey Child). This way, there is no need to change the ports later if a NAT
            // is detected on the new path.
            if (mHasCheckedNattSupport
                    && mSupportNatTraversal
                    && mIkeSocket.getIkeServerPort() != IkeSocket.SERVER_PORT_UDP_ENCAPSULATED) {
                getAndSwitchToIkeSocket(
                        mIkeSocket instanceof IkeUdp4Socket, true /* useEncapPort */);
            }
        }
    }

    /**
     * CreateIkeLocalIkeAuth represents state when IKE library initiates IKE_AUTH exchange.
     *
     * <p>If using EAP, CreateIkeLocalIkeAuth will transition to CreateIkeLocalIkeAuthInEap state
     * after validating the IKE AUTH response.
     */
    class CreateIkeLocalIkeAuth extends CreateIkeLocalIkeAuthBase {
        private boolean mUseEap;

        @Override
        public void enterState() {
            try {
                super.enterState();
                mRetransmitter = new EncryptedRetransmitter(buildIkeAuthReq());
                mUseEap =
                        (IkeSessionParams.IKE_AUTH_METHOD_EAP
                                == mIkeSessionParams.getLocalAuthConfig().mAuthMethod);
            } catch (SpiUnavailableException | ResourceUnavailableException e) {
                // Handle IPsec SPI assigning failure.
                handleIkeFatalError(e);
            }
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_SET_NETWORK:
                    // Shouldn't be receiving this command before MOBIKE is active - determined with
                    // last IKE_AUTH response
                    logWtf("Received SET_NETWORK cmd in " + getCurrentState().getName());
                    return NOT_HANDLED;

                default:
                    return super.processStateMessage(message);
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                int exchangeType = ikeMessage.ikeHeader.exchangeType;
                if (exchangeType != IkeHeader.EXCHANGE_TYPE_IKE_AUTH) {
                    throw new InvalidSyntaxException(
                            "Expected EXCHANGE_TYPE_IKE_AUTH but received: " + exchangeType);
                }

                validateIkeAuthResp(ikeMessage);

                List<IkePayload> childReqList =
                        extractChildPayloadsFromMessage(mRetransmitter.getMessage());
                if (mUseEap) {
                    // childReqList needed after EAP completed, so persist to IkeSessionStateMachine
                    // state.
                    mFirstChildReqList = childReqList;

                    IkeEapPayload ikeEapPayload =
                            ikeMessage.getPayloadForType(
                                    IkePayload.PAYLOAD_TYPE_EAP, IkeEapPayload.class);
                    if (ikeEapPayload == null) {
                        throw new AuthenticationFailedException("Missing EAP payload");
                    }
                    deferMessage(obtainMessage(CMD_EAP_START_EAP_AUTH, ikeEapPayload));
                    transitionTo(mCreateIkeLocalIkeAuthInEap);
                } else {
                    if (mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)) {
                        setUpMobilityHandling();
                    }
                    notifyIkeSessionSetup(ikeMessage);

                    performFirstChildNegotiation(
                            childReqList, extractChildPayloadsFromMessage(ikeMessage));
                }
            } catch (IkeException e) {
                if (!mUseEap) {
                    // Notify the remote because they may have set up the IKE SA.
                    sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
                }
                handleIkeFatalError(e);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            mRetransmitter.stopRetransmitting();

            if (!mUseEap) {
                // Notify the remote because they may have set up the IKE SA.
                sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
            }
            handleIkeFatalError(ikeException);
        }

        private IkeMessage buildIkeAuthReq()
                throws SpiUnavailableException, ResourceUnavailableException {
            List<IkePayload> payloadList = new LinkedList<>();

            // Build Identification payloads
            mInitIdPayload =
                    new IkeIdPayload(
                            true /*isInitiator*/, mIkeSessionParams.getLocalIdentification());
            IkeIdPayload respIdPayload =
                    new IkeIdPayload(
                            false /*isInitiator*/, mIkeSessionParams.getRemoteIdentification());
            payloadList.add(mInitIdPayload);
            payloadList.add(respIdPayload);

            if (mIkeSessionParams.hasIkeOption(IKE_OPTION_EAP_ONLY_AUTH)) {
                payloadList.add(new IkeNotifyPayload(NOTIFY_TYPE_EAP_ONLY_AUTHENTICATION));
            }
            if (mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)) {
                payloadList.add(new IkeNotifyPayload(NOTIFY_TYPE_MOBIKE_SUPPORTED));
            }

            // Build Authentication payload
            IkeAuthConfig authConfig = mIkeSessionParams.getLocalAuthConfig();
            switch (authConfig.mAuthMethod) {
                case IkeSessionParams.IKE_AUTH_METHOD_PSK:
                    IkeAuthPskPayload pskPayload =
                            new IkeAuthPskPayload(
                                    ((IkeAuthPskConfig) authConfig).mPsk,
                                    mIkeInitRequestBytes,
                                    mCurrentIkeSaRecord.nonceResponder,
                                    mInitIdPayload.getEncodedPayloadBody(),
                                    mIkePrf,
                                    mCurrentIkeSaRecord.getSkPi());
                    payloadList.add(pskPayload);
                    break;
                case IkeSessionParams.IKE_AUTH_METHOD_PUB_KEY_SIGNATURE:
                    IkeAuthDigitalSignLocalConfig localAuthConfig =
                            (IkeAuthDigitalSignLocalConfig) mIkeSessionParams.getLocalAuthConfig();

                    // Add certificates to list
                    payloadList.add(
                            new IkeCertX509CertPayload(localAuthConfig.getClientEndCertificate()));
                    for (X509Certificate intermediateCert : localAuthConfig.mIntermediateCerts) {
                        payloadList.add(new IkeCertX509CertPayload(intermediateCert));
                    }

                    IkeAuthDigitalSignPayload digitalSignaturePayload =
                            new IkeAuthDigitalSignPayload(
                                    IkeAuthDigitalSignPayload.SIGNATURE_ALGO_RSA_SHA2_512,
                                    localAuthConfig.mPrivateKey,
                                    mIkeInitRequestBytes,
                                    mCurrentIkeSaRecord.nonceResponder,
                                    mInitIdPayload.getEncodedPayloadBody(),
                                    mIkePrf,
                                    mCurrentIkeSaRecord.getSkPi());
                    payloadList.add(digitalSignaturePayload);

                    break;
                case IkeSessionParams.IKE_AUTH_METHOD_EAP:
                    // Do not include AUTH payload when using EAP.
                    break;
                default:
                    cleanUpAndQuit(
                            new IllegalArgumentException(
                                    "Unrecognized authentication method: "
                                            + authConfig.mAuthMethod));
            }

            payloadList.addAll(
                    CreateChildSaHelper.getInitChildCreateReqPayloads(
                            mRandomFactory,
                            mIpSecSpiGenerator,
                            mLocalAddress,
                            mFirstChildSessionParams,
                            true /*isFirstChildSa*/));

            final List<ConfigAttribute> configAttributes = new ArrayList<>();
            configAttributes.addAll(
                    Arrays.asList(
                            CreateChildSaHelper.getConfigAttributes(mFirstChildSessionParams)));
            configAttributes.addAll(
                    Arrays.asList(mIkeSessionParams.getConfigurationAttributesInternal()));
            // Always request app version
            configAttributes.add(new IkeConfigPayload.ConfigAttributeAppVersion());
            payloadList.add(new IkeConfigPayload(false /*isReply*/, configAttributes));

            // Add 3GPP-specific payloads for this exchange subtype
            payloadList.addAll(
                    mIke3gppExtensionExchange.getRequestPayloads(IKE_EXCHANGE_SUBTYPE_IKE_AUTH));

            return buildIkeAuthReqMessage(payloadList);
        }

        private void validateIkeAuthResp(IkeMessage authResp) throws IkeException {
            // Validate IKE Authentication
            IkeAuthPayload authPayload = null;
            List<IkeCertPayload> certPayloads = new LinkedList<>();

            // Process 3GPP-specific payloads before verifying IKE_AUTH to ensure that the
            // caller is informed of them.
            List<IkePayload> ike3gppPayloads =
                    handle3gppRespAndExtractNonError3gppPayloads(
                            IKE_EXCHANGE_SUBTYPE_IKE_AUTH, authResp.ikePayloadList);

            List<IkePayload> payloadsWithout3gpp = new ArrayList<>(authResp.ikePayloadList);
            payloadsWithout3gpp.removeAll(ike3gppPayloads);

            for (IkePayload payload : payloadsWithout3gpp) {
                switch (payload.payloadType) {
                    case IkePayload.PAYLOAD_TYPE_ID_RESPONDER:
                        mRespIdPayload = (IkeIdPayload) payload;
                        if (!mIkeSessionParams.hasIkeOption(
                                        IkeSessionParams.IKE_OPTION_ACCEPT_ANY_REMOTE_ID)
                                && !mIkeSessionParams
                                        .getRemoteIdentification()
                                        .equals(mRespIdPayload.ikeId)) {
                            throw new AuthenticationFailedException(
                                    "Unrecognized Responder Identification.");
                        }
                        break;
                    case IkePayload.PAYLOAD_TYPE_AUTH:
                        authPayload = (IkeAuthPayload) payload;
                        break;
                    case IkePayload.PAYLOAD_TYPE_CERT:
                        certPayloads.add((IkeCertPayload) payload);
                        break;
                    case IkePayload.PAYLOAD_TYPE_NOTIFY:
                        handleNotifyInLastAuthResp(
                                (IkeNotifyPayload) payload,
                                authResp.getPayloadForType(
                                        PAYLOAD_TYPE_AUTH, IkeAuthPayload.class));
                        break;
                    case PAYLOAD_TYPE_SA: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_CP: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_TS_INITIATOR: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_TS_RESPONDER: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_EAP: // Will be handled separately
                        break;
                    default:
                        logw(
                                "Received unexpected payload in IKE AUTH response. Payload"
                                        + " type: "
                                        + payload.payloadType);
                }
            }

            // Verify existence of payloads
            if (authPayload == null && mIkeSessionParams.hasIkeOption(IKE_OPTION_EAP_ONLY_AUTH)) {
                // If EAP-only option is selected, the responder will not send auth payload if it
                // accepts EAP-only authentication. Currently only EAP-only safe methods are
                // proposed to the responder if IKE_OPTION_EAP_ONLY_AUTH option is set. So there is
                // no need to check if the responder selected an EAP-only safe method
                return;
            }

            // Authenticate the remote peer.
            if (authPayload != null && mRespIdPayload != null) {
                authenticate(authPayload, mRespIdPayload, certPayloads);
                return;
            }

            throw new AuthenticationFailedException("ID-Responder or Auth payload is missing.");
        }

        private void authenticate(
                IkeAuthPayload authPayload,
                IkeIdPayload respIdPayload,
                List<IkeCertPayload> certPayloads)
                throws AuthenticationFailedException {
            switch (mIkeSessionParams.getRemoteAuthConfig().mAuthMethod) {
                case IkeSessionParams.IKE_AUTH_METHOD_PSK:
                    authenticatePsk(
                            ((IkeAuthPskConfig) mIkeSessionParams.getRemoteAuthConfig()).mPsk,
                            authPayload,
                            respIdPayload);
                    break;
                case IkeSessionParams.IKE_AUTH_METHOD_PUB_KEY_SIGNATURE:
                    authenticateDigitalSignature(
                            certPayloads,
                            ((IkeAuthDigitalSignRemoteConfig)
                                            mIkeSessionParams.getRemoteAuthConfig())
                                    .mTrustAnchor,
                            authPayload,
                            respIdPayload);
                    break;
                default:
                    cleanUpAndQuit(
                            new IllegalArgumentException(
                                    "Unrecognized auth method: " + authPayload.authMethod));
            }
        }

        private void authenticateDigitalSignature(
                List<IkeCertPayload> certPayloads,
                TrustAnchor trustAnchor,
                IkeAuthPayload authPayload,
                IkeIdPayload respIdPayload)
                throws AuthenticationFailedException {
            if (authPayload.authMethod != IkeAuthPayload.AUTH_METHOD_RSA_DIGITAL_SIGN
                    && authPayload.authMethod != IkeAuthPayload.AUTH_METHOD_GENERIC_DIGITAL_SIGN) {
                throw new AuthenticationFailedException(
                        "Expected the remote/server to use digital-signature-based authentication"
                                + " but they used: "
                                + authPayload.authMethod);
            }

            X509Certificate endCert = null;
            List<X509Certificate> certList = new LinkedList<>();

            // TODO: b/122676944 Extract CRL from IkeCrlPayload when we support IkeCrlPayload
            for (IkeCertPayload certPayload : certPayloads) {
                X509Certificate cert = ((IkeCertX509CertPayload) certPayload).certificate;

                // The first certificate MUST be the end entity certificate.
                if (endCert == null) endCert = cert;
                certList.add(cert);
            }

            if (endCert == null) {
                throw new AuthenticationFailedException(
                        "The remote/server failed to provide a end certificate");
            }

            respIdPayload.validateEndCertIdOrThrow(endCert);

            Set<TrustAnchor> trustAnchorSet =
                    trustAnchor == null ? null : Collections.singleton(trustAnchor);

            IkeCertPayload.validateCertificates(
                    endCert, certList, null /*crlList*/, trustAnchorSet);

            IkeAuthDigitalSignPayload signPayload = (IkeAuthDigitalSignPayload) authPayload;
            signPayload.verifyInboundSignature(
                    endCert,
                    mIkeInitResponseBytes,
                    mCurrentIkeSaRecord.nonceInitiator,
                    respIdPayload.getEncodedPayloadBody(),
                    mIkePrf,
                    mCurrentIkeSaRecord.getSkPr());
        }

        @Override
        public void exitState() {
            mRetransmitter.stopRetransmitting();
        }
    }

    /**
     * CreateIkeLocalIkeAuthInEap represents the state when the IKE library authenticates the client
     * with an EAP session.
     */
    class CreateIkeLocalIkeAuthInEap extends CreateIkeLocalIkeAuthBase {
        private EapAuthenticator mEapAuthenticator;

        @Override
        public void enterState() {
            IkeSessionParams.IkeAuthEapConfig ikeAuthEapConfig =
                    (IkeSessionParams.IkeAuthEapConfig) mIkeSessionParams.getLocalAuthConfig();

            // TODO(b/148689509): Pass in deterministic random when test mode is enabled
            mEapAuthenticator =
                    mEapAuthenticatorFactory.newEapAuthenticator(
                            getHandler().getLooper(),
                            new IkeEapCallback(),
                            mContext,
                            ikeAuthEapConfig.mEapConfig,
                            mRandomFactory);
        }

        @Override
        public boolean processStateMessage(Message msg) {
            switch (msg.what) {
                case CMD_EAP_START_EAP_AUTH:
                    IkeEapPayload ikeEapPayload = (IkeEapPayload) msg.obj;
                    mEapAuthenticator.processEapMessage(ikeEapPayload.eapMessage);

                    return HANDLED;
                case CMD_EAP_OUTBOUND_MSG_READY:
                    byte[] eapMsgBytes = (byte[]) msg.obj;
                    IkeEapPayload eapPayload = new IkeEapPayload(eapMsgBytes);

                    // Setup new retransmitter with EAP response
                    mRetransmitter =
                            new EncryptedRetransmitter(
                                    buildIkeAuthReqMessage(Arrays.asList(eapPayload)));

                    return HANDLED;
                case CMD_EAP_ERRORED:
                    handleIkeFatalError(new AuthenticationFailedException((Throwable) msg.obj));
                    return HANDLED;
                case CMD_EAP_FAILED:
                    AuthenticationFailedException exception =
                            new AuthenticationFailedException("EAP Authentication Failed");

                    handleIkeFatalError(exception);
                    return HANDLED;
                case CMD_EAP_FINISH_EAP_AUTH:
                    deferMessage(msg);
                    transitionTo(mCreateIkeLocalIkeAuthPostEap);

                    return HANDLED;
                case CMD_SET_NETWORK:
                    // Shouldn't be receiving this command before MOBIKE is active - determined with
                    // last IKE_AUTH response
                    logWtf("Received SET_NETWORK cmd in " + getCurrentState().getName());
                    return NOT_HANDLED;
                default:
                    return super.processStateMessage(msg);
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                mRetransmitter.stopRetransmitting();

                int exchangeType = ikeMessage.ikeHeader.exchangeType;
                if (exchangeType != IkeHeader.EXCHANGE_TYPE_IKE_AUTH) {
                    throw new InvalidSyntaxException(
                            "Expected EXCHANGE_TYPE_IKE_AUTH but received: " + exchangeType);
                }

                // Process 3GPP-specific payloads before verifying IKE_AUTH to ensure that the
                // caller is informed of them.
                List<IkePayload> ike3gppPayloads =
                        handle3gppRespAndExtractNonError3gppPayloads(
                                IKE_EXCHANGE_SUBTYPE_IKE_AUTH, ikeMessage.ikePayloadList);

                List<IkePayload> payloadsWithout3gpp = new ArrayList<>(ikeMessage.ikePayloadList);
                payloadsWithout3gpp.removeAll(ike3gppPayloads);

                IkeEapPayload eapPayload = null;
                for (IkePayload payload : payloadsWithout3gpp) {
                    switch (payload.payloadType) {
                        case IkePayload.PAYLOAD_TYPE_EAP:
                            eapPayload = (IkeEapPayload) payload;
                            break;
                        case IkePayload.PAYLOAD_TYPE_NOTIFY:
                            IkeNotifyPayload notifyPayload = (IkeNotifyPayload) payload;
                            if (notifyPayload.isErrorNotify()) {
                                throw notifyPayload.validateAndBuildIkeException();
                            } else {
                                // Unknown and unexpected status notifications are ignored as per
                                // RFC7296.
                                logw(
                                        "Received unknown or unexpected status notifications with"
                                                + " notify type: "
                                                + notifyPayload.notifyType);
                            }
                            break;
                        default:
                            logw(
                                    "Received unexpected payload in IKE AUTH response. Payload"
                                            + " type: "
                                            + payload.payloadType);
                    }
                }

                if (eapPayload == null) {
                    throw new AuthenticationFailedException("EAP Payload is missing.");
                }

                mEapAuthenticator.processEapMessage(eapPayload.eapMessage);
            } catch (IkeProtocolException exception) {
                handleIkeFatalError(exception);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            mRetransmitter.stopRetransmitting();
            handleIkeFatalError(ikeException);
        }

        private class IkeEapCallback implements IEapCallback {
            @Override
            public void onSuccess(byte[] msk, byte[] emsk) {
                // Extended MSK not used in IKEv2, drop.
                sendMessage(CMD_EAP_FINISH_EAP_AUTH, msk);
            }

            @Override
            public void onFail() {
                sendMessage(CMD_EAP_FAILED);
            }

            @Override
            public void onResponse(byte[] eapMsg) {
                sendMessage(CMD_EAP_OUTBOUND_MSG_READY, eapMsg);
            }

            @Override
            public void onError(Throwable cause) {
                sendMessage(CMD_EAP_ERRORED, cause);
            }
        }
    }

    /**
     * CreateIkeLocalIkeAuthPostEap represents the state when the IKE library is performing the
     * post-EAP PSK-base authentication run.
     */
    class CreateIkeLocalIkeAuthPostEap extends CreateIkeLocalIkeAuthBase {
        private byte[] mEapMsk = new byte[0];

        @Override
        public boolean processStateMessage(Message msg) {
            switch (msg.what) {
                case CMD_EAP_FINISH_EAP_AUTH:
                    mEapMsk = (byte[]) msg.obj;

                    IkeAuthPskPayload pskPayload =
                            new IkeAuthPskPayload(
                                    mEapMsk,
                                    mIkeInitRequestBytes,
                                    mCurrentIkeSaRecord.nonceResponder,
                                    mInitIdPayload.getEncodedPayloadBody(),
                                    mIkePrf,
                                    mCurrentIkeSaRecord.getSkPi());
                    IkeMessage postEapAuthMsg = buildIkeAuthReqMessage(Arrays.asList(pskPayload));
                    mRetransmitter = new EncryptedRetransmitter(postEapAuthMsg);

                    return HANDLED;
                case CMD_SET_NETWORK:
                    // Shouldn't be receiving this command before MOBIKE is active - determined with
                    // last IKE_AUTH response
                    logWtf("Received SET_NETWORK cmd in " + getCurrentState().getName());
                    return NOT_HANDLED;
                default:
                    return super.processStateMessage(msg);
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                int exchangeType = ikeMessage.ikeHeader.exchangeType;
                if (exchangeType != IkeHeader.EXCHANGE_TYPE_IKE_AUTH) {
                    throw new InvalidSyntaxException(
                            "Expected EXCHANGE_TYPE_IKE_AUTH but received: " + exchangeType);
                }

                validateIkeAuthRespPostEap(ikeMessage);

                if (mIkeSessionParams.hasIkeOption(IKE_OPTION_MOBIKE)) {
                    setUpMobilityHandling();
                }
                notifyIkeSessionSetup(ikeMessage);

                performFirstChildNegotiation(
                        mFirstChildReqList, extractChildPayloadsFromMessage(ikeMessage));
            } catch (IkeException e) {
                // Notify the remote because they may have set up the IKE SA.
                sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
                handleIkeFatalError(e);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            mRetransmitter.stopRetransmitting();
            // Notify the remote because they may have set up the IKE SA.
            sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
            handleIkeFatalError(ikeException);
        }

        private void validateIkeAuthRespPostEap(IkeMessage authResp) throws IkeException {
            IkeAuthPayload authPayload = null;

            // Process 3GPP-specific payloads before verifying IKE_AUTH to ensure that the
            // caller is informed of them.
            List<IkePayload> ike3gppPayloads =
                    handle3gppRespAndExtractNonError3gppPayloads(
                            IKE_EXCHANGE_SUBTYPE_IKE_AUTH, authResp.ikePayloadList);

            List<IkePayload> payloadsWithout3gpp = new ArrayList<>(authResp.ikePayloadList);
            payloadsWithout3gpp.removeAll(ike3gppPayloads);

            for (IkePayload payload : payloadsWithout3gpp) {
                switch (payload.payloadType) {
                    case IkePayload.PAYLOAD_TYPE_AUTH:
                        authPayload = (IkeAuthPayload) payload;
                        break;
                    case IkePayload.PAYLOAD_TYPE_NOTIFY:
                        handleNotifyInLastAuthResp(
                                (IkeNotifyPayload) payload,
                                authResp.getPayloadForType(
                                        PAYLOAD_TYPE_AUTH, IkeAuthPayload.class));
                        break;
                    case PAYLOAD_TYPE_SA: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_CP: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_TS_INITIATOR: // Will be handled separately; fall through
                    case PAYLOAD_TYPE_TS_RESPONDER: // Will be handled separately; fall through
                        break;
                    default:
                        logw(
                                "Received unexpected payload in IKE AUTH response. Payload"
                                        + " type: "
                                        + payload.payloadType);
                }
            }

            // Verify existence of payloads
            if (authPayload == null) {
                throw new AuthenticationFailedException("Post-EAP Auth payload missing.");
            }

            authenticatePsk(mEapMsk, authPayload, mRespIdPayload);
        }

        @Override
        public void exitState() {
            mRetransmitter.stopRetransmitting();
        }
    }

    private abstract class RekeyIkeHandlerBase extends DeleteBase {
        private void validateIkeRekeyCommon(IkeMessage ikeMessage) throws InvalidSyntaxException {
            boolean hasSaPayload = false;
            boolean hasKePayload = false;
            boolean hasNoncePayload = false;
            for (IkePayload payload : ikeMessage.ikePayloadList) {
                switch (payload.payloadType) {
                    case IkePayload.PAYLOAD_TYPE_SA:
                        hasSaPayload = true;
                        break;
                    case IkePayload.PAYLOAD_TYPE_KE:
                        hasKePayload = true;
                        break;
                    case IkePayload.PAYLOAD_TYPE_NONCE:
                        hasNoncePayload = true;
                        break;
                    case IkePayload.PAYLOAD_TYPE_VENDOR:
                        // Vendor payloads allowed, but not verified
                        break;
                    case IkePayload.PAYLOAD_TYPE_NOTIFY:
                        // Notification payloads allowed, but left to handler methods to process.
                        break;
                    default:
                        logw(
                                "Received unexpected payload in IKE REKEY request. Payload type: "
                                        + payload.payloadType);
                }
            }

            if (!hasSaPayload || !hasKePayload || !hasNoncePayload) {
                throw new InvalidSyntaxException("SA, KE or Nonce payload missing.");
            }
        }

        @VisibleForTesting
        void validateIkeRekeyReq(IkeMessage ikeMessage) throws InvalidSyntaxException {
            // Skip validation of exchange type since it has been done during decoding request.

            List<IkeNotifyPayload> notificationPayloads =
                    ikeMessage.getPayloadListForType(
                            IkePayload.PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);
            for (IkeNotifyPayload notifyPayload : notificationPayloads) {
                if (notifyPayload.isErrorNotify()) {
                    logw("Error notifications invalid in request: " + notifyPayload.notifyType);
                }
            }

            validateIkeRekeyCommon(ikeMessage);
        }

        @VisibleForTesting
        void validateIkeRekeyResp(IkeMessage reqMsg, IkeMessage respMsg)
                throws InvalidSyntaxException {
            int exchangeType = respMsg.ikeHeader.exchangeType;
            if (exchangeType != IkeHeader.EXCHANGE_TYPE_CREATE_CHILD_SA
                    && exchangeType != IkeHeader.EXCHANGE_TYPE_INFORMATIONAL) {
                throw new InvalidSyntaxException(
                        "Expected Rekey response (CREATE_CHILD_SA or INFORMATIONAL) but received: "
                                + exchangeType);
            }

            List<IkeNotifyPayload> notificationPayloads =
                    respMsg.getPayloadListForType(
                            IkePayload.PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);
            for (IkeNotifyPayload notifyPayload : notificationPayloads) {
                if (notifyPayload.isErrorNotify()) {
                    // Error notifications found. Stop validation for SA negotiation.
                    return;
                }
            }

            validateIkeRekeyCommon(respMsg);

            // Verify DH groups matching
            IkeKePayload reqKePayload =
                    reqMsg.getPayloadForType(IkePayload.PAYLOAD_TYPE_KE, IkeKePayload.class);
            IkeKePayload respKePayload =
                    respMsg.getPayloadForType(IkePayload.PAYLOAD_TYPE_KE, IkeKePayload.class);
            if (reqKePayload.dhGroup != respKePayload.dhGroup) {
                throw new InvalidSyntaxException("Received KE payload with mismatched DH group.");
            }
        }

        // It doesn't make sense to include multiple error notify payloads in one response. If it
        // happens, IKE Session will only handle the most severe one.
        protected boolean handleErrorNotifyIfExists(IkeMessage respMsg, boolean isSimulRekey) {
            IkeNotifyPayload invalidSyntaxNotifyPayload = null;
            IkeNotifyPayload tempFailureNotifyPayload = null;
            IkeNotifyPayload firstErrorNotifyPayload = null;

            List<IkeNotifyPayload> notificationPayloads =
                    respMsg.getPayloadListForType(
                            IkePayload.PAYLOAD_TYPE_NOTIFY, IkeNotifyPayload.class);
            for (IkeNotifyPayload notifyPayload : notificationPayloads) {
                if (!notifyPayload.isErrorNotify()) continue;

                if (firstErrorNotifyPayload == null) firstErrorNotifyPayload = notifyPayload;

                if (ERROR_TYPE_INVALID_SYNTAX == notifyPayload.notifyType) {
                    invalidSyntaxNotifyPayload = notifyPayload;
                } else if (ERROR_TYPE_TEMPORARY_FAILURE == notifyPayload.notifyType) {
                    tempFailureNotifyPayload = notifyPayload;
                }
            }

            // No error Notify Payload included in this response.
            if (firstErrorNotifyPayload == null) return NOT_HANDLED;

            // Handle Invalid Syntax if it exists
            if (invalidSyntaxNotifyPayload != null) {
                try {
                    IkeProtocolException exception =
                            invalidSyntaxNotifyPayload.validateAndBuildIkeException();
                    handleIkeFatalError(exception);
                } catch (InvalidSyntaxException e) {
                    // Error notify payload has invalid syntax
                    handleIkeFatalError(e);
                }
                return HANDLED;
            }

            if (tempFailureNotifyPayload != null) {
                // Handle Temporary Failure if exists
                loge("Received TEMPORARY_FAILURE for rekey IKE. Already handled during decoding.");
            } else {
                // Handle other errors
                loge(
                        "Received error notification: "
                                + firstErrorNotifyPayload.notifyType
                                + " for rekey IKE. Schedule a retry");
                if (!isSimulRekey) {
                    mCurrentIkeSaRecord.rescheduleRekey(RETRY_INTERVAL_MS);
                }
            }

            if (isSimulRekey) {
                transitionTo(mRekeyIkeRemoteDelete);
            } else {
                transitionTo(mIdle);
            }
            return HANDLED;
        }

        protected IkeSaRecord validateAndBuildIkeSa(
                IkeMessage reqMsg, IkeMessage respMessage, boolean isLocalInit)
                throws IkeProtocolException, GeneralSecurityException, IOException {
            InetAddress initAddr = isLocalInit ? mLocalAddress : mRemoteAddress;
            InetAddress respAddr = isLocalInit ? mRemoteAddress : mLocalAddress;

            Pair<IkeProposal, IkeProposal> negotiatedProposals = null;
            try {
                IkeSaPayload reqSaPayload =
                        reqMsg.getPayloadForType(IkePayload.PAYLOAD_TYPE_SA, IkeSaPayload.class);
                IkeSaPayload respSaPayload =
                        respMessage.getPayloadForType(
                                IkePayload.PAYLOAD_TYPE_SA, IkeSaPayload.class);

                // Throw exception or return valid negotiated proposal with allocated SPIs
                negotiatedProposals =
                        IkeSaPayload.getVerifiedNegotiatedIkeProposalPair(
                                reqSaPayload, respSaPayload, mIkeSpiGenerator, mRemoteAddress);
                IkeProposal reqProposal = negotiatedProposals.first;
                IkeProposal respProposal = negotiatedProposals.second;

                IkeMacPrf newPrf;
                IkeCipher newCipher;
                IkeMacIntegrity newIntegrity = null;

                newCipher = IkeCipher.create(respProposal.saProposal.getEncryptionTransforms()[0]);
                if (!newCipher.isAead()) {
                    newIntegrity =
                            IkeMacIntegrity.create(
                                    respProposal.saProposal.getIntegrityTransforms()[0]);
                }
                newPrf = IkeMacPrf.create(respProposal.saProposal.getPrfTransforms()[0]);

                // Build new SaRecord
                long remoteSpi =
                        isLocalInit
                                ? respProposal.getIkeSpiResource().getSpi()
                                : reqProposal.getIkeSpiResource().getSpi();
                IkeSaRecord newSaRecord =
                        IkeSaRecord.makeRekeyedIkeSaRecord(
                                mCurrentIkeSaRecord,
                                mIkePrf,
                                reqMsg,
                                respMessage,
                                reqProposal.getIkeSpiResource(),
                                respProposal.getIkeSpiResource(),
                                newPrf,
                                newIntegrity == null ? 0 : newIntegrity.getKeyLength(),
                                newCipher.getKeyLength(),
                                isLocalInit,
                                buildSaLifetimeAlarmScheduler(remoteSpi));
                addIkeSaRecord(newSaRecord);

                mIkeCipher = newCipher;
                mIkePrf = newPrf;
                mIkeIntegrity = newIntegrity;

                return newSaRecord;
            } catch (IkeProtocolException | GeneralSecurityException | IOException e) {
                if (negotiatedProposals != null) {
                    negotiatedProposals.first.getIkeSpiResource().close();
                    negotiatedProposals.second.getIkeSpiResource().close();
                }
                throw e;
            }
        }
    }

    /** RekeyIkeLocalCreate represents state when IKE library initiates Rekey IKE exchange. */
    class RekeyIkeLocalCreate extends RekeyIkeHandlerBase {
        protected Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            try {
                mRetransmitter = new EncryptedRetransmitter(buildIkeRekeyReq());
            } catch (IOException e) {
                loge("Fail to assign IKE SPI for rekey. Schedule a retry.", e);
                mCurrentIkeSaRecord.rescheduleRekey(RETRY_INTERVAL_MS);
                transitionTo(mIdle);
            }
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        protected void handleTempFailure() {
            mTempFailHandler.handleTempFailure();
            mCurrentIkeSaRecord.rescheduleRekey(RETRY_INTERVAL_MS);
        }

        /**
         * Builds a IKE Rekey request, reusing the current proposal
         *
         * <p>As per RFC 7296, rekey messages are of format: { HDR { SK { SA, Ni, KEi } } }
         *
         * <p>This method currently reuses agreed upon proposal.
         */
        private IkeMessage buildIkeRekeyReq() throws IOException {
            // TODO: Evaluate if we need to support different proposals for rekeys
            IkeSaProposal[] saProposals = new IkeSaProposal[] {mSaProposal};

            // No need to allocate SPIs; they will be allocated as part of the
            // getRekeyIkeSaRequestPayloads
            List<IkePayload> payloadList =
                    CreateIkeSaHelper.getRekeyIkeSaRequestPayloads(
                            saProposals, mIkeSpiGenerator, mLocalAddress, mRandomFactory);

            // Build IKE header
            IkeHeader ikeHeader =
                    new IkeHeader(
                            mCurrentIkeSaRecord.getInitiatorSpi(),
                            mCurrentIkeSaRecord.getResponderSpi(),
                            IkePayload.PAYLOAD_TYPE_SK,
                            IkeHeader.EXCHANGE_TYPE_CREATE_CHILD_SA,
                            false /*isResponseMsg*/,
                            mCurrentIkeSaRecord.isLocalInit,
                            mCurrentIkeSaRecord.getLocalRequestMessageId());

            return new IkeMessage(ikeHeader, payloadList);
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    handleDeleteSessionRequest(ikeMessage);
                    break;
                case IKE_EXCHANGE_SUBTYPE_GENERIC_INFO:
                    handleGenericInfoRequest(ikeMessage);
                    break;
                default:
                    // TODO: Implement simultaneous rekey
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                // Validate syntax
                validateIkeRekeyResp(mRetransmitter.getMessage(), ikeMessage);

                // Handle error notifications if they exist
                if (handleErrorNotifyIfExists(ikeMessage, false /*isSimulRekey*/) == NOT_HANDLED) {
                    // No error notifications included. Negotiate new SA
                    mLocalInitNewIkeSaRecord =
                            validateAndBuildIkeSa(
                                    mRetransmitter.getMessage(), ikeMessage, true /*isLocalInit*/);
                    transitionTo(mRekeyIkeLocalDelete);
                }

                // Stop retransmissions
                mRetransmitter.stopRetransmitting();
            } catch (IkeProtocolException e) {
                if (e instanceof InvalidSyntaxException) {
                    handleProcessRespOrSaCreationFailureAndQuit(e);
                } else {
                    handleProcessRespOrSaCreationFailureAndQuit(
                            new InvalidSyntaxException(
                                    "Error in processing IKE Rekey-Create response", e));
                }

            } catch (GeneralSecurityException | IOException e) {
                handleProcessRespOrSaCreationFailureAndQuit(
                        new IkeInternalException("Error in creating a new IKE SA during rekey", e));
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException ikeException) {
            handleProcessRespOrSaCreationFailureAndQuit(ikeException);
        }

        private void handleProcessRespOrSaCreationFailureAndQuit(IkeException exception) {
            // We don't retry rekey if failure was caused by invalid response or SA creation error.
            // Reason is there is no way to notify the remote side the old SA is still alive but the
            // new one has failed.

            mRetransmitter.stopRetransmitting();

            sendEncryptedIkeMessage(buildIkeDeleteReq(mCurrentIkeSaRecord));
            handleIkeFatalError(exception);
        }
    }

    /**
     * SimulRekeyIkeLocalCreate represents the state where IKE library has replied to rekey request
     * sent from the remote and is waiting for a rekey response for a locally initiated rekey
     * request.
     *
     * <p>SimulRekeyIkeLocalCreate extends RekeyIkeLocalCreate so that it can call super class to
     * validate incoming rekey response against locally initiated rekey request.
     */
    class SimulRekeyIkeLocalCreate extends RekeyIkeLocalCreate {
        @Override
        public void enterState() {
            mRetransmitter = new EncryptedRetransmitter(null);
            // TODO: Populate super.mRetransmitter from state initialization data
            // Do not send request.
        }

        public IkeMessage buildRequest() {
            throw new UnsupportedOperationException(
                    "Do not support sending request in " + getCurrentState().getName());
        }

        @Override
        public void exitState() {
            // Do nothing.
        }

        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_IKE_PACKET:
                    ReceivedIkePacket receivedIkePacket = (ReceivedIkePacket) message.obj;
                    IkeHeader ikeHeader = receivedIkePacket.ikeHeader;

                    if (mRemoteInitNewIkeSaRecord == getIkeSaRecordForPacket(ikeHeader)) {
                        deferMessage(message);
                    } else {
                        handleReceivedIkePacket(message);
                    }
                    return HANDLED;

                default:
                    return super.processStateMessage(message);
            }
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    deferMessage(message);
                    return;
                default:
                    // TODO: Add more cases for other types of request.
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                validateIkeRekeyResp(mRetransmitter.getMessage(), ikeMessage);

                // TODO: Check and handle error notifications before SA negotiation

                mLocalInitNewIkeSaRecord =
                        validateAndBuildIkeSa(
                                mRetransmitter.getMessage(), ikeMessage, true /*isLocalInit*/);
                transitionTo(mSimulRekeyIkeLocalDeleteRemoteDelete);
            } catch (IkeProtocolException e) {
                // TODO: Handle processing errors.
            } catch (GeneralSecurityException e) {
                // TODO: Fatal - kill session.
            } catch (IOException e) {
                // TODO: SPI allocation collided - delete new IKE SA, retry rekey.
            }
        }
    }

    /** RekeyIkeDeleteBase represents common behaviours of deleting stage during rekeying IKE SA. */
    private abstract class RekeyIkeDeleteBase extends DeleteBase {
        @Override
        public boolean processStateMessage(Message message) {
            switch (message.what) {
                case CMD_RECEIVE_IKE_PACKET:
                    ReceivedIkePacket receivedIkePacket = (ReceivedIkePacket) message.obj;
                    IkeHeader ikeHeader = receivedIkePacket.ikeHeader;

                    // Verify that this message is correctly authenticated and encrypted:
                    IkeSaRecord ikeSaRecord = getIkeSaRecordForPacket(ikeHeader);
                    boolean isMessageOnNewSa = false;
                    if (ikeSaRecord != null && mIkeSaRecordSurviving == ikeSaRecord) {
                        DecodeResult decodeResult =
                                IkeMessage.decode(
                                        ikeHeader.isResponseMsg
                                                ? ikeSaRecord.getLocalRequestMessageId()
                                                : ikeSaRecord.getRemoteRequestMessageId(),
                                        mIkeIntegrity,
                                        mIkeCipher,
                                        ikeSaRecord,
                                        ikeHeader,
                                        receivedIkePacket.ikePacketBytes,
                                        ikeSaRecord.getCollectedFragments(ikeHeader.isResponseMsg));
                        isMessageOnNewSa =
                                (decodeResult.status == DECODE_STATUS_PROTECTED_ERROR)
                                        || (decodeResult.status == DECODE_STATUS_OK)
                                        || (decodeResult.status == DECODE_STATUS_PARTIAL);
                    }

                    // Authenticated request received on the new/surviving SA; treat it as
                    // an acknowledgement that the remote has successfully rekeyed.
                    if (isMessageOnNewSa) {
                        State nextState = mIdle;

                        // This is the first IkeMessage seen on the new SA. It cannot be a response.
                        // Likewise, if it a request, it must not be a retransmission. Verify msgId.
                        // If either condition happens, consider rekey a success, but immediately
                        // kill the session.
                        if (ikeHeader.isResponseMsg
                                || ikeSaRecord.getRemoteRequestMessageId() - ikeHeader.messageId
                                        != 0) {
                            nextState = mDeleteIkeLocalDelete;
                        } else {
                            deferMessage(message);
                        }

                        // Locally close old (and losing) IKE SAs. As a result of not waiting for
                        // delete responses, the old SA can be left in a state where the stored ID
                        // is no longer correct. However, this finishRekey() call will remove that
                        // SA, so it doesn't matter.
                        finishRekey();
                        transitionTo(nextState);
                    } else {
                        handleReceivedIkePacket(message);
                    }

                    return HANDLED;
                default:
                    return super.processStateMessage(message);
                    // TODO: Add more cases for other packet types.
            }
        }

        // Rekey timer for old (and losing) SAs will be cancelled as part of the closing of the SA.
        protected void finishRekey() {
            mCurrentIkeSaRecord = mIkeSaRecordSurviving;
            mLocalInitNewIkeSaRecord = null;
            mRemoteInitNewIkeSaRecord = null;

            mIkeSaRecordSurviving = null;

            if (mIkeSaRecordAwaitingLocalDel != null) {
                removeIkeSaRecord(mIkeSaRecordAwaitingLocalDel);
                mIkeSaRecordAwaitingLocalDel.close();
                mIkeSaRecordAwaitingLocalDel = null;
            }

            if (mIkeSaRecordAwaitingRemoteDel != null) {
                removeIkeSaRecord(mIkeSaRecordAwaitingRemoteDel);
                mIkeSaRecordAwaitingRemoteDel.close();
                mIkeSaRecordAwaitingRemoteDel = null;
            }

            synchronized (mChildCbToSessions) {
                for (ChildSessionStateMachine child : mChildCbToSessions.values()) {
                    child.setSkD(mCurrentIkeSaRecord.getSkD());
                }
            }

            // TODO: Update prf of all child sessions
        }
    }

    /**
     * SimulRekeyIkeLocalDeleteRemoteDelete represents the deleting stage during simultaneous
     * rekeying when IKE library is waiting for both a Delete request and a Delete response.
     */
    class SimulRekeyIkeLocalDeleteRemoteDelete extends RekeyIkeDeleteBase {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            // Detemine surviving IKE SA. According to RFC 7296: "The new IKE SA containing the
            // lowest nonce SHOULD be deleted by the node that created it, and the other surviving
            // new IKE SA MUST inherit all the Child SAs."
            if (mLocalInitNewIkeSaRecord.compareTo(mRemoteInitNewIkeSaRecord) > 0) {
                mIkeSaRecordSurviving = mLocalInitNewIkeSaRecord;
                mIkeSaRecordAwaitingLocalDel = mCurrentIkeSaRecord;
                mIkeSaRecordAwaitingRemoteDel = mRemoteInitNewIkeSaRecord;
            } else {
                mIkeSaRecordSurviving = mRemoteInitNewIkeSaRecord;
                mIkeSaRecordAwaitingLocalDel = mLocalInitNewIkeSaRecord;
                mIkeSaRecordAwaitingRemoteDel = mCurrentIkeSaRecord;
            }
            mRetransmitter =
                    new EncryptedRetransmitter(
                            mIkeSaRecordAwaitingLocalDel,
                            buildIkeDeleteReq(mIkeSaRecordAwaitingLocalDel));
            // TODO: Set timer awaiting for delete request.
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            IkeSaRecord ikeSaRecordForPacket = getIkeSaRecordForPacket(ikeMessage.ikeHeader);
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    try {
                        validateIkeDeleteReq(ikeMessage, mIkeSaRecordAwaitingRemoteDel);
                        IkeMessage respMsg =
                                buildIkeDeleteResp(ikeMessage, mIkeSaRecordAwaitingRemoteDel);
                        removeIkeSaRecord(mIkeSaRecordAwaitingRemoteDel);
                        // TODO: Encode and send response and close
                        // mIkeSaRecordAwaitingRemoteDel.
                        // TODO: Stop timer awating delete request.
                        transitionTo(mSimulRekeyIkeLocalDelete);
                    } catch (InvalidSyntaxException e) {
                        logd("Validation failed for delete request", e);
                        // TODO: Shutdown - fatal error
                    }
                    return;
                default:
                    // TODO: Reply with TEMPORARY_FAILURE
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                validateIkeDeleteResp(ikeMessage, mIkeSaRecordAwaitingLocalDel);
                finishDeleteIkeSaAwaitingLocalDel();
            } catch (InvalidSyntaxException e) {
                loge("Invalid syntax on IKE Delete response. Shutting down anyways", e);
                finishDeleteIkeSaAwaitingLocalDel();
            } catch (IllegalStateException e) {
                // Response received on incorrect SA
                cleanUpAndQuit(e);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException exception) {
            if (mIkeSaRecordAwaitingLocalDel == ikeSaRecord) {
                loge("Invalid syntax on IKE Delete response. Shutting down anyways", exception);
                finishDeleteIkeSaAwaitingLocalDel();
            } else {
                cleanUpAndQuit(
                        new IllegalStateException("Delete response received on incorrect SA"));
            }
        }

        private void finishDeleteIkeSaAwaitingLocalDel() {
            mRetransmitter.stopRetransmitting();

            removeIkeSaRecord(mIkeSaRecordAwaitingLocalDel);
            mIkeSaRecordAwaitingLocalDel.close();
            mIkeSaRecordAwaitingLocalDel = null;

            transitionTo(mSimulRekeyIkeRemoteDelete);
        }

        @Override
        public void exitState() {
            finishRekey();
            mRetransmitter.stopRetransmitting();
            // TODO: Stop awaiting delete request timer.
        }
    }

    /**
     * SimulRekeyIkeLocalDelete represents the state when IKE library is waiting for a Delete
     * response during simultaneous rekeying.
     */
    class SimulRekeyIkeLocalDelete extends RekeyIkeDeleteBase {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            mRetransmitter = new EncryptedRetransmitter(mIkeSaRecordAwaitingLocalDel, null);
            // TODO: Populate mRetransmitter from state initialization data.
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            // Always return a TEMPORARY_FAILURE. In no case should we accept a message on an SA
            // that is going away. All messages on the new SA is caught in RekeyIkeDeleteBase
            buildAndSendErrorNotificationResponse(
                    mIkeSaRecordAwaitingLocalDel,
                    ikeMessage.ikeHeader.messageId,
                    ERROR_TYPE_TEMPORARY_FAILURE);
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                validateIkeDeleteResp(ikeMessage, mIkeSaRecordAwaitingLocalDel);
                finishRekey();
                transitionTo(mIdle);
            } catch (InvalidSyntaxException e) {
                loge(
                        "Invalid syntax on IKE Delete response. Shutting down old IKE SA and"
                                + " finishing rekey",
                        e);
                finishRekey();
                transitionTo(mIdle);
            } catch (IllegalStateException e) {
                // Response received on incorrect SA
                cleanUpAndQuit(e);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException exception) {
            if (mIkeSaRecordAwaitingLocalDel == ikeSaRecord) {
                loge(
                        "Invalid syntax on IKE Delete response. Shutting down old IKE SA and"
                                + " finishing rekey",
                        exception);
                finishRekey();
                transitionTo(mIdle);
            } else {
                cleanUpAndQuit(
                        new IllegalStateException("Delete response received on incorrect SA"));
            }
        }
    }

    /**
     * SimulRekeyIkeRemoteDelete represents the state that waiting for a Delete request during
     * simultaneous rekeying.
     */
    class SimulRekeyIkeRemoteDelete extends RekeyIkeDeleteBase {
        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            // At this point, the incoming request can ONLY be on mIkeSaRecordAwaitingRemoteDel - if
            // it was on the surviving SA, it is deferred and the rekey is finished. It is likewise
            // impossible to have this on the local-deleted SA, since the delete has already been
            // acknowledged in the SimulRekeyIkeLocalDeleteRemoteDelete state.
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    try {
                        validateIkeDeleteReq(ikeMessage, mIkeSaRecordAwaitingRemoteDel);

                        IkeMessage respMsg =
                                buildIkeDeleteResp(ikeMessage, mIkeSaRecordAwaitingRemoteDel);
                        sendEncryptedIkeMessage(mIkeSaRecordAwaitingRemoteDel, respMsg);

                        finishRekey();
                        transitionTo(mIdle);
                    } catch (InvalidSyntaxException e) {
                        // Program error.
                        cleanUpAndQuit(new IllegalStateException(e));
                    }
                    return;
                default:
                    buildAndSendErrorNotificationResponse(
                            mIkeSaRecordAwaitingRemoteDel,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
            }
        }
    }

    /**
     * RekeyIkeLocalDelete represents the deleting stage when IKE library is initiating a Rekey
     * procedure.
     *
     * <p>RekeyIkeLocalDelete and SimulRekeyIkeLocalDelete have same behaviours in
     * processStateMessage(). While RekeyIkeLocalDelete overrides enterState() and exitState()
     * methods for initiating and finishing the deleting stage for IKE rekeying.
     */
    class RekeyIkeLocalDelete extends SimulRekeyIkeLocalDelete {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            mIkeSaRecordSurviving = mLocalInitNewIkeSaRecord;
            mIkeSaRecordAwaitingLocalDel = mCurrentIkeSaRecord;
            mRetransmitter =
                    new EncryptedRetransmitter(
                            mIkeSaRecordAwaitingLocalDel,
                            buildIkeDeleteReq(mIkeSaRecordAwaitingLocalDel));
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        public void exitState() {
            mRetransmitter.stopRetransmitting();
        }
    }

    /**
     * RekeyIkeRemoteDelete represents the deleting stage when responding to a Rekey procedure.
     *
     * <p>RekeyIkeRemoteDelete and SimulRekeyIkeRemoteDelete have same behaviours in
     * processStateMessage(). While RekeyIkeLocalDelete overrides enterState() and exitState()
     * methods for waiting incoming delete request and for finishing the deleting stage for IKE
     * rekeying.
     */
    class RekeyIkeRemoteDelete extends SimulRekeyIkeRemoteDelete {
        @Override
        public void enterState() {
            mIkeSaRecordSurviving = mRemoteInitNewIkeSaRecord;
            mIkeSaRecordAwaitingRemoteDel = mCurrentIkeSaRecord;

            sendMessageDelayed(TIMEOUT_REKEY_REMOTE_DELETE, REKEY_DELETE_TIMEOUT_MS);
        }

        @Override
        public boolean processStateMessage(Message message) {
            // Intercept rekey delete timeout. Assume rekey succeeded since no retransmissions
            // were received.
            if (message.what == TIMEOUT_REKEY_REMOTE_DELETE) {
                finishRekey();
                transitionTo(mIdle);

                return HANDLED;
            } else {
                return super.processStateMessage(message);
            }
        }

        @Override
        public void exitState() {
            removeMessages(TIMEOUT_REKEY_REMOTE_DELETE);
        }
    }

    /** DeleteIkeLocalDelete initiates a deletion request of the current IKE Session. */
    class DeleteIkeLocalDelete extends DeleteBase {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            mRetransmitter = new EncryptedRetransmitter(buildIkeDeleteReq(mCurrentIkeSaRecord));
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    handleDeleteSessionRequest(ikeMessage);
                    return;
                default:
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            try {
                validateIkeDeleteResp(ikeMessage, mCurrentIkeSaRecord);
                executeUserCallback(
                        () -> {
                            mIkeSessionCallback.onClosed();
                        });

                removeIkeSaRecord(mCurrentIkeSaRecord);
                mCurrentIkeSaRecord.close();
                mCurrentIkeSaRecord = null;
                quitNow();
            } catch (InvalidSyntaxException e) {
                handleResponseGenericProcessError(mCurrentIkeSaRecord, e);
            }
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException exception) {
            loge("Invalid syntax on IKE Delete response. Shutting down anyways", exception);
            handleIkeFatalError(exception);
            quitNow();
        }

        @Override
        public void exitState() {
            mRetransmitter.stopRetransmitting();
        }
    }

    /** DpdIkeLocalInfo initiates a dead peer detection for IKE Session. */
    class DpdIkeLocalInfo extends DeleteBase {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            mRetransmitter =
                    new EncryptedRetransmitter(
                            buildEncryptedInformationalMessage(
                                    new IkeInformationalPayload[0],
                                    false /*isResp*/,
                                    mCurrentIkeSaRecord.getLocalRequestMessageId()));
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        protected void handleRequestIkeMessage(
                IkeMessage ikeMessage, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_GENERIC_INFO:
                    handleGenericInfoRequest(ikeMessage);
                    return;
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    // Reply and close IKE
                    handleDeleteSessionRequest(ikeMessage);
                    return;
                default:
                    // Reply and stay in current state
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            ikeMessage.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
                    return;
            }
        }

        @Override
        protected void handleResponseIkeMessage(IkeMessage ikeMessage) {
            // DPD response usually contains no payload. But since there is not any requirement of
            // it, payload validation will be skipped.
            if (ikeMessage.ikeHeader.exchangeType == IkeHeader.EXCHANGE_TYPE_INFORMATIONAL) {
                transitionTo(mIdle);
                return;
            }

            handleResponseGenericProcessError(
                    mCurrentIkeSaRecord,
                    new InvalidSyntaxException(
                            "Invalid exchange type; expected INFORMATIONAL, but got: "
                                    + ikeMessage.ikeHeader.exchangeType));
        }

        @Override
        protected void handleResponseGenericProcessError(
                IkeSaRecord ikeSaRecord, InvalidSyntaxException exception) {
            loge("Invalid syntax on IKE DPD response.", exception);
            handleIkeFatalError(exception);

            // #exitState will be called when StateMachine quits
            quitNow();
        }

        @Override
        public void exitState() {
            mRetransmitter.stopRetransmitting();
        }
    }

    /**
     * MobikeLocalInfo handles mobility event for the IKE Session.
     *
     * <p>When MOBIKE is supported by both sides, MobikeLocalInfo will initiate an
     * UPDATE_SA_ADDRESSES exchange for the IKE Session.
     */
    class MobikeLocalInfo extends DeleteBase {
        private Retransmitter mRetransmitter;

        @Override
        public void enterState() {
            if (!mSupportMobike) {
                logd("non-MOBIKE mobility event");
                migrateAllChildSAs();
                notifyConnectionInfoChanged();
                transitionTo(mIdle);
                return;
            }

            logd("MOBIKE mobility event");
            mRetransmitter = new EncryptedRetransmitter(buildUpdateSaAddressesReq());
        }

        private boolean needNatDetection() {
            if (mRemoteAddress instanceof Inet4Address) {
                // Add NAT_DETECTION payloads when it is unknown if server supports NAT-T or not, or
                // it is known that server supports NAT-T.
                return !mHasCheckedNattSupport || mSupportNatTraversal;
            } else {
                // Add NAT_DETECTION payloads only when a NAT has been detected previously. This is
                // mainly for updating the previous NAT detection result, so that if IKE Session
                // migrates from a v4 NAT environment to a v6 non-NAT environment, both sides can
                // switch to use non-encap ESP SA. This is especially beneficial for implementations
                // that do not support Ipv6 NAT-T.
                return mLocalNatDetected || mRemoteNatDetected;
            }
        }

        private IkeMessage buildUpdateSaAddressesReq() {
            // Generics required for addNatDetectionPayloadsToList that takes List<IkePayload> and
            // buildEncryptedInformationalMessage that takes InformationalPayload[].
            List<? super IkeInformationalPayload> payloadList = new ArrayList<>();
            payloadList.add(new IkeNotifyPayload(NOTIFY_TYPE_UPDATE_SA_ADDRESSES));

            if (needNatDetection()) {
                addNatDetectionPayloadsToList(
                        (List<IkePayload>) payloadList,
                        mLocalAddress,
                        mRemoteAddress,
                        mLocalPort,
                        mIkeSocket.getIkeServerPort(),
                        mCurrentIkeSaRecord.getInitiatorSpi(),
                        mCurrentIkeSaRecord.getResponderSpi());
            }

            return buildEncryptedInformationalMessage(
                    mCurrentIkeSaRecord,
                    payloadList.toArray(new IkeInformationalPayload[payloadList.size()]),
                    false /* isResp */,
                    mCurrentIkeSaRecord.getLocalRequestMessageId());
        }

        @Override
        protected void triggerRetransmit() {
            mRetransmitter.retransmit();
        }

        @Override
        public void exitState() {
            super.exitState();

            if (mRetransmitter != null) {
                mRetransmitter.stopRetransmitting();
            }
        }

        @Override
        public void handleRequestIkeMessage(
                IkeMessage msg, int ikeExchangeSubType, Message message) {
            switch (ikeExchangeSubType) {
                case IKE_EXCHANGE_SUBTYPE_DELETE_IKE:
                    handleDeleteSessionRequest(msg);
                    break;

                default:
                    // Send a temporary failure for all non-DELETE_IKE requests
                    buildAndSendErrorNotificationResponse(
                            mCurrentIkeSaRecord,
                            msg.ikeHeader.messageId,
                            ERROR_TYPE_TEMPORARY_FAILURE);
            }
        }

        @Override
        public void handleResponseIkeMessage(IkeMessage resp) {
            mRetransmitter.stopRetransmitting();

            try {
                validateResp(resp);

                migrateAllChildSAs();
                notifyConnectionInfoChanged();
                transitionTo(mIdle);
            } catch (IkeException | IOException e) {
                handleIkeFatalError(e);
            }
        }

        private void validateResp(IkeMessage resp) throws IkeException, IOException {
            if (resp.ikeHeader.exchangeType != IkeHeader.EXCHANGE_TYPE_INFORMATIONAL) {
                throw new InvalidSyntaxException(
                        "Invalid exchange type; expected INFORMATIONAL, but got: "
                                + resp.ikeHeader.exchangeType);
            }

            List<IkeNotifyPayload> natSourcePayloads = new ArrayList<>();
            IkeNotifyPayload natDestPayload = null;

            for (IkePayload payload : resp.ikePayloadList) {
                switch (payload.payloadType) {
                    case PAYLOAD_TYPE_NOTIFY:
                        IkeNotifyPayload notifyPayload = (IkeNotifyPayload) payload;
                        if (notifyPayload.isErrorNotify()) {
                            // TODO(b/): handle UNACCEPTABLE_ADDRESSES payload
                            throw notifyPayload.validateAndBuildIkeException();
                        }

                        switch (notifyPayload.notifyType) {
                            case NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP:
                                natSourcePayloads.add(notifyPayload);
                                break;
                            case NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP:
                                if (natDestPayload != null) {
                                    throw new InvalidSyntaxException(
                                            "More than one"
                                                    + " NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP"
                                                    + " found");
                                }
                                natDestPayload = notifyPayload;
                                break;
                            default:
                                // Unknown and unexpected status notifications are ignored as per
                                // RFC7296.
                                logw(
                                        "Received unknown or unexpected status notifications with"
                                                + " notify type: "
                                                + notifyPayload.notifyType);
                        }

                        break;
                    default:
                        logw("Unexpected payload types found: " + payload.payloadType);
                }
            }

            if (mRetransmitter.getMessage().hasNotifyPayload(NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP)) {
                handleNatDetection(resp, natSourcePayloads, natDestPayload);
            }
        }

        /** Handle NAT detection and switch socket if needed */
        private void handleNatDetection(
                IkeMessage resp,
                List<IkeNotifyPayload> natSourcePayloads,
                IkeNotifyPayload natDestPayload)
                throws IkeException {
            if (!didPeerIncludeNattDetectionPayloads(natSourcePayloads, natDestPayload)) {
                // If this is first time that IKE client sends NAT_DETECTION payloads, mark that the
                // server does not support NAT-T
                if (!mHasCheckedNattSupport) {
                    mHasCheckedNattSupport = true;
                    mSupportNatTraversal = false;
                }
                return;
            }

            // If this is first time that IKE client sends NAT_DETECTION payloads, mark that the
            // server supports NAT-T, and switch socket if a NAT is detected.
            if (!mHasCheckedNattSupport) {
                mHasCheckedNattSupport = true;
                mSupportNatTraversal = true;
            }

            updateLocalAndRemoteNatDetected(
                    resp.ikeHeader.ikeInitiatorSpi,
                    resp.ikeHeader.ikeResponderSpi,
                    natSourcePayloads,
                    natDestPayload);

            if (mLocalNatDetected || mRemoteNatDetected) {
                if (mRemoteAddress instanceof Inet6Address) {
                    throw new IkeInternalException(
                            new UnsupportedOperationException("An IPv6 NAT is detected."));
                }

                logd("Switching to send to remote port 4500 if it's not already");
                getAndSwitchToIkeSocket(
                        mRemoteAddress instanceof Inet4Address, true /* useEncapPort */);
            }
        }

        private void migrateAllChildSAs() {
            // TODO(b/172015298): migrate Child SAs directly if Kernel support

            // Schedule MOBIKE Rekeys for all Child Sessions
            for (int i = 0; i < mRemoteSpiToChildSessionMap.size(); i++) {
                int remoteChildSpi = mRemoteSpiToChildSessionMap.keyAt(i);
                sendMessage(
                        CMD_LOCAL_REQUEST_REKEY_CHILD_MOBIKE,
                        mLocalRequestFactory.getChildLocalRequest(
                                CMD_LOCAL_REQUEST_REKEY_CHILD_MOBIKE, remoteChildSpi));
            }
        }

        // TODO: b/177434707 Calling IkeSessionConnectionInfo constructor is safe because it does
        // not depend on any platform API added after SDK R. Handle this case in a mainline standard
        // way when b/177434707 is fixed.
        @SuppressLint("NewApi")
        private void notifyConnectionInfoChanged() {
            IkeSessionConnectionInfo connectionInfo =
                    new IkeSessionConnectionInfo(mLocalAddress, mRemoteAddress, mNetwork);
            executeUserCallback(
                    () -> mIkeSessionCallback.onIkeSessionConnectionInfoChanged(connectionInfo));
        }
    }

    private static void addNatDetectionPayloadsToList(
            List<IkePayload> payloadList,
            InetAddress localAddr,
            InetAddress remoteAddr,
            int localPort,
            int remotePort,
            long initIkeSpi,
            long respIkeSpi) {
        // Though RFC says Notify-NAT payload is "just after the Ni and Nr payloads (before
        // the optional CERTREQ payload)", it also says recipient MUST NOT reject " messages
        // in which the payloads were not in the "right" order" due to the lack of clarity
        // of the payload order.
        payloadList.add(
                new IkeNotifyPayload(
                        NOTIFY_TYPE_NAT_DETECTION_SOURCE_IP,
                        IkeNotifyPayload.generateNatDetectionData(
                                initIkeSpi, respIkeSpi, localAddr, localPort)));
        payloadList.add(
                new IkeNotifyPayload(
                        NOTIFY_TYPE_NAT_DETECTION_DESTINATION_IP,
                        IkeNotifyPayload.generateNatDetectionData(
                                initIkeSpi, respIkeSpi, remoteAddr, remotePort)));
    }

    /**
     * Helper class to generate IKE SA creation payloads, in both request and response directions.
     */
    private static class CreateIkeSaHelper {
        public static List<IkePayload> getIkeInitSaRequestPayloads(
                IkeSaProposal[] saProposals,
                int selectedDhGroup,
                long initIkeSpi,
                long respIkeSpi,
                InetAddress localAddr,
                InetAddress remoteAddr,
                int localPort,
                int remotePort,
                RandomnessFactory randomFactory,
                boolean isMobikeSupportEnabled)
                throws IOException {
            List<IkePayload> payloadList =
                    getCreateIkeSaPayloads(
                            selectedDhGroup,
                            IkeSaPayload.createInitialIkeSaPayload(saProposals),
                            randomFactory);

            if (remoteAddr instanceof Inet4Address) {
                // TODO(b/184869678): support NAT detection for all cases
                // UdpEncap for V6 is not supported in Android yet, so only send NAT Detection
                // payloads when using IPv4 addresses
                addNatDetectionPayloadsToList(
                        payloadList,
                        localAddr,
                        remoteAddr,
                        localPort,
                        remotePort,
                        initIkeSpi,
                        respIkeSpi);
            }

            return payloadList;
        }

        public static List<IkePayload> getRekeyIkeSaRequestPayloads(
                IkeSaProposal[] saProposals,
                IkeSpiGenerator ikeSpiGenerator,
                InetAddress localAddr,
                RandomnessFactory randomFactory)
                throws IOException {
            if (localAddr == null) {
                throw new IllegalArgumentException("Local address was null for rekey");
            }

            // Guaranteed to have at least one SA Proposal, since the IKE session was set up
            // properly.
            int selectedDhGroup = saProposals[0].getDhGroupTransforms()[0].id;

            return getCreateIkeSaPayloads(
                    selectedDhGroup,
                    IkeSaPayload.createRekeyIkeSaRequestPayload(
                            saProposals, ikeSpiGenerator, localAddr),
                    randomFactory);
        }

        public static List<IkePayload> getRekeyIkeSaResponsePayloads(
                byte respProposalNumber,
                IkeSaProposal saProposal,
                IkeSpiGenerator ikeSpiGenerator,
                InetAddress localAddr,
                RandomnessFactory randomFactory)
                throws IOException {
            if (localAddr == null) {
                throw new IllegalArgumentException("Local address was null for rekey");
            }

            int selectedDhGroup = saProposal.getDhGroupTransforms()[0].id;

            return getCreateIkeSaPayloads(
                    selectedDhGroup,
                    IkeSaPayload.createRekeyIkeSaResponsePayload(
                            respProposalNumber, saProposal, ikeSpiGenerator, localAddr),
                    randomFactory);
        }

        /**
         * Builds the initial or rekey IKE creation payloads.
         *
         * <p>Will return a non-empty list of IkePayloads, the first of which WILL be the SA payload
         */
        private static List<IkePayload> getCreateIkeSaPayloads(
                int selectedDhGroup, IkeSaPayload saPayload, RandomnessFactory randomFactory)
                throws IOException {
            if (saPayload.proposalList.size() == 0) {
                throw new IllegalArgumentException("Invalid SA proposal list - was empty");
            }

            List<IkePayload> payloadList = new ArrayList<>(3);

            // The old IKE spec RFC 4306 (section 2.5 and 2.6) requires the payload order in IKE
            // INIT to be SAi, KEi, Ni and allow responders to reject requests with wrong order.
            // Although starting from RFC 5996, the protocol removed the allowance for rejecting
            // messages in which the payloads were not in the "right" order, there are few responder
            // implementations are still following the old spec when handling IKE INIT request with
            // COOKIE payload. Thus IKE library should follow the payload order to be compatible
            // with older implementations.
            payloadList.add(saPayload);

            // SaPropoals.Builder guarantees that each SA proposal has at least one DH group.
            payloadList.add(IkeKePayload.createOutboundKePayload(selectedDhGroup, randomFactory));

            payloadList.add(new IkeNoncePayload(randomFactory));

            return payloadList;
        }
    }

    /**
     * Updates the underlying Network for this IKE Session to be the specified Network. This will
     * also update the local address and IkeSocket for the IKE Session.
     *
     * <p>MUST be called from the Handler Thread to avoid races.
     */
    @Override
    public void onUnderlyingNetworkUpdated(Network network) {
        Network oldNetwork = mNetwork;
        InetAddress oldLocalAddress = mLocalAddress;
        InetAddress oldRemoteAddress = mRemoteAddress;

        mNetwork = network;

        // If the network changes, perform a new DNS lookup to ensure that the correct remote
        // address is used. This ensures that DNS returns addresses for the correct address families
        // (important if using a v4/v6-only network). This also ensures that DNS64 is handled
        // correctly when switching between networks that may have different IPv6 prefixes.
        if (!mNetwork.equals(oldNetwork)) {
            try {
                resolveAndSetAvailableRemoteAddresses();
            } catch (IOException e) {
                handleIkeFatalError(e);
                return;
            }
        }

        setRemoteAddress();

        boolean isIpv4 = mRemoteAddress instanceof Inet4Address;

        // If it is known that the server supports NAT-T, use port 4500. Otherwise, use port 500.
        boolean nattSupported = mHasCheckedNattSupport && mSupportNatTraversal;
        int serverPort =
                nattSupported
                        ? IkeSocket.SERVER_PORT_UDP_ENCAPSULATED
                        : IkeSocket.SERVER_PORT_NON_UDP_ENCAPSULATED;

        try {
            mLocalAddress =
                    mIkeLocalAddressGenerator.generateLocalAddress(
                            mNetwork, isIpv4, mRemoteAddress, serverPort);

            if (mNetwork.equals(oldNetwork)
                    && mLocalAddress.equals(oldLocalAddress)
                    && mRemoteAddress.equals(oldRemoteAddress)) {
                logw(
                        "onUnderlyingNetworkUpdated: None of network, local or remote address has"
                                + " changed. No action needed here.");
                return;
            }

            // Only switch the IkeSocket if the underlying Network actually changes. This may not
            // always happen (ex: the underlying Network loses the current local address)
            if (!mNetwork.equals(oldNetwork)) {
                boolean useEncapPort =
                        mIkeSessionParams.hasIkeOption(IKE_OPTION_FORCE_PORT_4500) || nattSupported;
                getAndSwitchToIkeSocket(isIpv4, useEncapPort);
            }
        } catch (ErrnoException | IOException e) {
            handleIkeFatalError(e);
            return;
        }
        mNetworkCallback.setNetwork(mNetwork);
        mNetworkCallback.setAddress(mLocalAddress);

        try {
            mCurrentIkeSaRecord.migrate(mLocalAddress, mRemoteAddress);
            if (mLocalInitNewIkeSaRecord != null) {
                mLocalInitNewIkeSaRecord.migrate(mLocalAddress, mRemoteAddress);
            }
            if (mRemoteInitNewIkeSaRecord != null) {
                mRemoteInitNewIkeSaRecord.migrate(mLocalAddress, mRemoteAddress);
            }
        } catch (IOException e) {
            // Failed to migrate IKE SAs due to IKE SPI collision
            handleIkeFatalError(e);
            return;
        }

        // TODO(b/172013873): restart transmission timeouts on IKE SAs after changing networks
        sendMessage(
                CMD_LOCAL_REQUEST_MOBIKE,
                mLocalRequestFactory.getIkeLocalRequest(CMD_LOCAL_REQUEST_MOBIKE));
    }

    private void resolveAndSetAvailableRemoteAddresses() throws IOException {
        // TODO(b/149954916): Do DNS resolution asynchronously
        InetAddress[] allRemoteAddresses = null;
        final String hostname = mIkeSessionParams.getServerHostname();

        for (int attempts = 1;
                attempts <= MAX_DNS_RESOLUTION_ATTEMPTS && allRemoteAddresses == null;
                attempts++) {
            try {
                allRemoteAddresses = mNetwork.getAllByName(hostname);
            } catch (UnknownHostException e) {
                final boolean willRetry = attempts < MAX_DNS_RESOLUTION_ATTEMPTS;
                logd(
                        "Failed to look up host for attempt "
                                + attempts
                                + ": "
                                + hostname
                                + " retrying? "
                                + willRetry,
                        e);
            }
        }
        if (allRemoteAddresses == null) {
            throw new IOException(
                    "DNS resolution for "
                            + hostname
                            + " failed after "
                            + MAX_DNS_RESOLUTION_ATTEMPTS
                            + " attempts");
        }

        logd(
                "Resolved addresses for peer: "
                        + Arrays.toString(allRemoteAddresses)
                        + " to replace old addresses: v4="
                        + mRemoteAddressesV4
                        + " v6="
                        + mRemoteAddressesV6);

        mRemoteAddressesV4.clear();
        mRemoteAddressesV6.clear();
        for (InetAddress remoteAddress : allRemoteAddresses) {
            if (remoteAddress instanceof Inet4Address) {
                mRemoteAddressesV4.add((Inet4Address) remoteAddress);
            } else {
                mRemoteAddressesV6.add((Inet6Address) remoteAddress);
            }
        }
    }

    @Override
    public void onUnderlyingNetworkDied() {
        executeUserCallback(
                () -> mIkeSessionCallback.onError(new IkeNetworkLostException(mNetwork)));
    }
}
