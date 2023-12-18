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

package android.net.ipsec.ike;

import static android.system.OsConstants.AF_INET;
import static android.system.OsConstants.AF_INET6;

import static com.android.internal.net.ipsec.ike.utils.IkeCertUtils.certificateFromByteArray;
import static com.android.internal.net.ipsec.ike.utils.IkeCertUtils.privateKeyFromByteArray;

import android.annotation.IntDef;
import android.annotation.IntRange;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SuppressLint;
import android.annotation.SystemApi;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.eap.EapSessionConfig;
import android.net.ipsec.ike.ike3gpp.Ike3gppExtension;
import android.os.PersistableBundle;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload.ConfigAttribute;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload.ConfigAttributeIpv4Pcscf;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload.ConfigAttributeIpv6Pcscf;
import com.android.internal.net.ipsec.ike.message.IkeConfigPayload.IkeConfigAttribute;
import com.android.internal.net.ipsec.ike.message.IkePayload;
import com.android.modules.utils.build.SdkLevel;
import com.android.server.vcn.util.PersistableBundleUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.security.PrivateKey;
import java.security.cert.CertificateEncodingException;
import java.security.cert.TrustAnchor;
import java.security.cert.X509Certificate;
import java.security.interfaces.RSAKey;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

/**
 * IkeSessionParams contains all user provided configurations for negotiating an {@link IkeSession}.
 *
 * <p>Note that all negotiated configurations will be reused during rekey including SA Proposal and
 * lifetime.
 */
public final class IkeSessionParams {
    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({IKE_AUTH_METHOD_PSK, IKE_AUTH_METHOD_PUB_KEY_SIGNATURE, IKE_AUTH_METHOD_EAP})
    public @interface IkeAuthMethod {}

    // Constants to describe user configured authentication methods.
    /** @hide */
    public static final int IKE_AUTH_METHOD_PSK = 1;
    /** @hide */
    public static final int IKE_AUTH_METHOD_PUB_KEY_SIGNATURE = 2;
    /** @hide */
    public static final int IKE_AUTH_METHOD_EAP = 3;

    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({AUTH_DIRECTION_LOCAL, AUTH_DIRECTION_REMOTE, AUTH_DIRECTION_BOTH})
    public @interface AuthDirection {}

    // Constants to describe which side (local and/or remote) the authentication configuration will
    // be used.
    /** @hide */
    public static final int AUTH_DIRECTION_LOCAL = 1;
    /** @hide */
    public static final int AUTH_DIRECTION_REMOTE = 2;
    /** @hide */
    public static final int AUTH_DIRECTION_BOTH = 3;

    /** @hide */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({IKE_OPTION_ACCEPT_ANY_REMOTE_ID, IKE_OPTION_EAP_ONLY_AUTH, IKE_OPTION_MOBIKE})
    public @interface IkeOption {}

    /**
     * If set, the IKE library will accept any remote (server) identity, even if it does not match
     * the configured remote identity
     *
     * <p>See {@link Builder#setRemoteIdentification(IkeIdentification)}
     */
    public static final int IKE_OPTION_ACCEPT_ANY_REMOTE_ID = 0;
    /**
     * If set, and EAP has been configured as the authentication method, the IKE library will
     * request that the remote (also) use an EAP-only authentication flow.
     *
     * <p>@see {@link Builder#setAuthEap(X509Certificate, EapSessionConfig)}
     */
    public static final int IKE_OPTION_EAP_ONLY_AUTH = 1;
    /**
     * If set, the IKE library will be able to handle network and address changes.
     *
     * <p>The IKE library will first attempt to enable MOBIKE to handle the changes of underlying
     * network and addresses. If the server does not support MOBIKE, the IKE library will handle the
     * changes by rekeying all the underlying Child SAs.
     *
     * <p>If this option is set for an IKE Session, Transport-mode SAs will not be allowed in that
     * Session.
     *
     * <p>Checking if MOBIKE is supported by both the IKE library and the server in an IKE Session
     * is done via {@link IkeSessionConfiguration#isIkeExtensionEnabled(int)}.
     */
    public static final int IKE_OPTION_MOBIKE = 2;

    /**
     * Configures the IKE session to always send to port 4500.
     *
     * <p>If set, the IKE Session will be initiated and maintained exclusively using
     * destination port 4500, regardless of the presence of NAT. Otherwise, the IKE Session will
     * be initiated on destination port 500; then, if either a NAT is detected or both MOBIKE
     * and NAT-T are supported by the peer, it will proceed on port 4500.
     */
    public static final int IKE_OPTION_FORCE_PORT_4500 = 3;

    private static final int MIN_IKE_OPTION = IKE_OPTION_ACCEPT_ANY_REMOTE_ID;
    private static final int MAX_IKE_OPTION = IKE_OPTION_FORCE_PORT_4500;

    /** @hide */
    @VisibleForTesting static final int IKE_HARD_LIFETIME_SEC_MINIMUM = 300; // 5 minutes
    /** @hide */
    @VisibleForTesting static final int IKE_HARD_LIFETIME_SEC_MAXIMUM = 86400; // 24 hours
    /** @hide */
    @VisibleForTesting static final int IKE_HARD_LIFETIME_SEC_DEFAULT = 14400; // 4 hours

    /** @hide */
    @VisibleForTesting static final int IKE_SOFT_LIFETIME_SEC_MINIMUM = 120; // 2 minutes
    /** @hide */
    @VisibleForTesting static final int IKE_SOFT_LIFETIME_SEC_DEFAULT = 7200; // 2 hours

    /** @hide */
    @VisibleForTesting
    static final int IKE_LIFETIME_MARGIN_SEC_MINIMUM = (int) TimeUnit.MINUTES.toSeconds(1L);

    /** @hide */
    @VisibleForTesting static final int IKE_DPD_DELAY_SEC_MIN = 20;
    /** @hide */
    @VisibleForTesting static final int IKE_DPD_DELAY_SEC_MAX = 1800; // 30 minutes
    /** @hide */
    @VisibleForTesting static final int IKE_DPD_DELAY_SEC_DEFAULT = 120; // 2 minutes

    /** @hide */
    @VisibleForTesting static final int IKE_NATT_KEEPALIVE_DELAY_SEC_MIN = 10;
    /** @hide */
    @VisibleForTesting static final int IKE_NATT_KEEPALIVE_DELAY_SEC_MAX = 3600;
    /** @hide */
    @VisibleForTesting static final int IKE_NATT_KEEPALIVE_DELAY_SEC_DEFAULT = 10;

    /** @hide */
    @VisibleForTesting static final int DSCP_MIN = 0;
    /** @hide */
    @VisibleForTesting static final int DSCP_MAX = 63;
    /** @hide */
    @VisibleForTesting static final int DSCP_DEFAULT = 0;

    /** @hide */
    @VisibleForTesting static final int IKE_RETRANS_TIMEOUT_MS_MIN = 500;
    /** @hide */
    @VisibleForTesting
    static final int IKE_RETRANS_TIMEOUT_MS_MAX = (int) TimeUnit.MINUTES.toMillis(30L);
    /** @hide */
    @VisibleForTesting static final int IKE_RETRANS_MAX_ATTEMPTS_MAX = 10;
    /** @hide */
    @VisibleForTesting
    static final int[] IKE_RETRANS_TIMEOUT_MS_LIST_DEFAULT =
            new int[] {500, 1000, 2000, 4000, 8000};

    private static final String SERVER_HOST_NAME_KEY = "mServerHostname";
    private static final String SA_PROPOSALS_KEY = "mSaProposals";
    private static final String LOCAL_ID_KEY = "mLocalIdentification";
    private static final String REMOTE_ID_KEY = "mRemoteIdentification";
    private static final String LOCAL_AUTH_KEY = "mLocalAuthConfig";
    private static final String REMOTE_AUTH_KEY = "mRemoteAuthConfig";
    private static final String CONFIG_ATTRIBUTES_KEY = "mConfigRequests";
    private static final String RETRANS_TIMEOUTS_KEY = "mRetransTimeoutMsList";
    private static final String IKE_OPTIONS_KEY = "mIkeOptions";
    private static final String HARD_LIFETIME_SEC_KEY = "mHardLifetimeSec";
    private static final String SOFT_LIFETIME_SEC_KEY = "mSoftLifetimeSec";
    private static final String DPD_DELAY_SEC_KEY = "mDpdDelaySec";
    private static final String NATT_KEEPALIVE_DELAY_SEC_KEY = "mNattKeepaliveDelaySec";
    private static final String DSCP_KEY = "mDscp";
    private static final String IS_IKE_FRAGMENT_SUPPORTED_KEY = "mIsIkeFragmentationSupported";

    @NonNull private final String mServerHostname;

    // @see #getNetwork for reasons of changing the annotation from @NonNull to @Nullable in SDK S.
    // Do not include mDefaultOrConfiguredNetwork in #hashCode or #equal because when it represents
    // configured network, it always has the same value as mCallerConfiguredNetwork. When it
    // represents a default network it can only reflects the device status at the IkeSessionParams
    // creation time. Since the actually default network may change after IkeSessionParams is
    // constructed, depending on mDefaultOrConfiguredNetwork in #hashCode and #equal to decide
    // if this object equals to another object does not make sense.
    @Nullable private final Network mDefaultOrConfiguredNetwork;

    @Nullable private final Network mCallerConfiguredNetwork;

    @NonNull private final IkeSaProposal[] mSaProposals;

    @NonNull private final IkeIdentification mLocalIdentification;
    @NonNull private final IkeIdentification mRemoteIdentification;

    @NonNull private final IkeAuthConfig mLocalAuthConfig;
    @NonNull private final IkeAuthConfig mRemoteAuthConfig;

    @NonNull private final IkeConfigAttribute[] mConfigRequests;

    @NonNull private final int[] mRetransTimeoutMsList;

    @Nullable private final Ike3gppExtension mIke3gppExtension;

    private final long mIkeOptions;

    private final int mHardLifetimeSec;
    private final int mSoftLifetimeSec;

    private final int mDpdDelaySec;
    private final int mNattKeepaliveDelaySec;
    private final int mDscp;

    private final boolean mIsIkeFragmentationSupported;

    private IkeSessionParams(
            @NonNull String serverHostname,
            @NonNull Network defaultOrConfiguredNetwork,
            @NonNull Network callerConfiguredNetwork,
            @NonNull IkeSaProposal[] proposals,
            @NonNull IkeIdentification localIdentification,
            @NonNull IkeIdentification remoteIdentification,
            @NonNull IkeAuthConfig localAuthConfig,
            @NonNull IkeAuthConfig remoteAuthConfig,
            @NonNull IkeConfigAttribute[] configRequests,
            @NonNull int[] retransTimeoutMsList,
            @Nullable Ike3gppExtension ike3gppExtension,
            long ikeOptions,
            int hardLifetimeSec,
            int softLifetimeSec,
            int dpdDelaySec,
            int nattKeepaliveDelaySec,
            int dscp,
            boolean isIkeFragmentationSupported) {
        mServerHostname = serverHostname;
        mDefaultOrConfiguredNetwork = defaultOrConfiguredNetwork;
        mCallerConfiguredNetwork = callerConfiguredNetwork;

        mSaProposals = proposals;

        mLocalIdentification = localIdentification;
        mRemoteIdentification = remoteIdentification;

        mLocalAuthConfig = localAuthConfig;
        mRemoteAuthConfig = remoteAuthConfig;

        mConfigRequests = configRequests;

        mRetransTimeoutMsList = retransTimeoutMsList;

        mIke3gppExtension = ike3gppExtension;

        mIkeOptions = ikeOptions;

        mHardLifetimeSec = hardLifetimeSec;
        mSoftLifetimeSec = softLifetimeSec;

        mDpdDelaySec = dpdDelaySec;
        mNattKeepaliveDelaySec = nattKeepaliveDelaySec;
        mDscp = dscp;

        mIsIkeFragmentationSupported = isIkeFragmentationSupported;
    }

    private static void validateIkeOptionOrThrow(@IkeOption int ikeOption) {
        if (ikeOption < MIN_IKE_OPTION || ikeOption > MAX_IKE_OPTION) {
            throw new IllegalArgumentException("Invalid IKE Option: " + ikeOption);
        }
    }

    private static long getOptionBitValue(int ikeOption) {
        return 1 << ikeOption;
    }

    /**
     * Constructs this object by deserializing a PersistableBundle
     *
     * <p>Constructed IkeSessionParams is guaranteed to be valid, as checked by the
     * IkeSessionParams.Builder
     *
     * @hide
     */
    @NonNull
    public static IkeSessionParams fromPersistableBundle(@NonNull PersistableBundle in) {
        Objects.requireNonNull(in, "PersistableBundle is null");

        IkeSessionParams.Builder builder = new IkeSessionParams.Builder();

        builder.setServerHostname(in.getString(SERVER_HOST_NAME_KEY));

        PersistableBundle proposalBundle = in.getPersistableBundle(SA_PROPOSALS_KEY);
        Objects.requireNonNull(in, "SA Proposals is null");
        List<IkeSaProposal> saProposals =
                PersistableBundleUtils.toList(proposalBundle, IkeSaProposal::fromPersistableBundle);
        for (IkeSaProposal proposal : saProposals) {
            builder.addSaProposal(proposal);
        }

        builder.setLocalIdentification(
                IkeIdentification.fromPersistableBundle(in.getPersistableBundle(LOCAL_ID_KEY)));
        builder.setRemoteIdentification(
                IkeIdentification.fromPersistableBundle(in.getPersistableBundle(REMOTE_ID_KEY)));
        builder.setAuth(
                IkeAuthConfig.fromPersistableBundle(in.getPersistableBundle(LOCAL_AUTH_KEY)),
                IkeAuthConfig.fromPersistableBundle(in.getPersistableBundle(REMOTE_AUTH_KEY)));

        PersistableBundle configBundle = in.getPersistableBundle(CONFIG_ATTRIBUTES_KEY);
        Objects.requireNonNull(configBundle, "configBundle is null");
        List<ConfigAttribute> configList =
                PersistableBundleUtils.toList(configBundle, ConfigAttribute::fromPersistableBundle);
        for (ConfigAttribute configAttribute : configList) {
            builder.addConfigRequest((IkeConfigAttribute) configAttribute);
        }

        builder.setRetransmissionTimeoutsMillis(in.getIntArray(RETRANS_TIMEOUTS_KEY));

        long ikeOptions = in.getLong(IKE_OPTIONS_KEY);
        for (int option = MIN_IKE_OPTION; option <= MAX_IKE_OPTION; option++) {
            if (hasIkeOption(ikeOptions, option)) {
                builder.addIkeOption(option);
            } else {
                builder.removeIkeOption(option);
            }
        }

        builder.setLifetimeSeconds(
                in.getInt(HARD_LIFETIME_SEC_KEY), in.getInt(SOFT_LIFETIME_SEC_KEY));
        builder.setDpdDelaySeconds(in.getInt(DPD_DELAY_SEC_KEY));
        builder.setNattKeepAliveDelaySeconds(in.getInt(NATT_KEEPALIVE_DELAY_SEC_KEY));

        // Fragmentation policy is not configurable. IkeSessionParams will always be constructed to
        // support fragmentation.
        if (!in.getBoolean(IS_IKE_FRAGMENT_SUPPORTED_KEY)) {
            throw new IllegalArgumentException("Invalid fragmentation policy");
        }

        return builder.build();
    }
    /**
     * Serializes this object to a PersistableBundle
     *
     * @hide
     */
    @NonNull
    public PersistableBundle toPersistableBundle() {
        if (mCallerConfiguredNetwork != null || mIke3gppExtension != null) {
            throw new IllegalStateException(
                    "Cannot convert a IkeSessionParams with a caller configured network or with"
                            + " 3GPP extension enabled");
        }
        final PersistableBundle result = new PersistableBundle();

        result.putString(SERVER_HOST_NAME_KEY, mServerHostname);

        PersistableBundle saProposalBundle =
                PersistableBundleUtils.fromList(
                        Arrays.asList(mSaProposals), IkeSaProposal::toPersistableBundle);
        result.putPersistableBundle(SA_PROPOSALS_KEY, saProposalBundle);

        result.putPersistableBundle(LOCAL_ID_KEY, mLocalIdentification.toPersistableBundle());
        result.putPersistableBundle(REMOTE_ID_KEY, mRemoteIdentification.toPersistableBundle());
        result.putPersistableBundle(LOCAL_AUTH_KEY, mLocalAuthConfig.toPersistableBundle());
        result.putPersistableBundle(REMOTE_AUTH_KEY, mRemoteAuthConfig.toPersistableBundle());

        PersistableBundle configAttributeBundle =
                PersistableBundleUtils.fromList(
                        Arrays.asList(mConfigRequests), ConfigAttribute::toPersistableBundle);
        result.putPersistableBundle(CONFIG_ATTRIBUTES_KEY, configAttributeBundle);

        result.putIntArray(RETRANS_TIMEOUTS_KEY, mRetransTimeoutMsList);
        result.putLong(IKE_OPTIONS_KEY, mIkeOptions);
        result.putInt(HARD_LIFETIME_SEC_KEY, mHardLifetimeSec);
        result.putInt(SOFT_LIFETIME_SEC_KEY, mSoftLifetimeSec);
        result.putInt(DPD_DELAY_SEC_KEY, mDpdDelaySec);
        result.putInt(NATT_KEEPALIVE_DELAY_SEC_KEY, mNattKeepaliveDelaySec);
        result.putInt(DSCP_KEY, mDscp);
        result.putBoolean(IS_IKE_FRAGMENT_SUPPORTED_KEY, mIsIkeFragmentationSupported);

        return result;
    }

    /**
     * Retrieves the configured server hostname
     *
     * <p>The configured server hostname will be resolved during IKE Session creation.
     */
    @NonNull
    public String getServerHostname() {
        return mServerHostname;
    }

    /**
     * Retrieves the configured {@link Network}, or null if was not set
     *
     * <p>This getter is for internal use. Not matter {@link Builder#Builder(Context)} or {@link
     * Builder#Builder()} is used, this method will always return null if no Network was set by the
     * caller.
     *
     * @hide
     */
    @Nullable
    public Network getConfiguredNetwork() {
        return mCallerConfiguredNetwork;
    }

    // This method was first released as a @NonNull System APi and has been changed to @Nullable
    // since Android S. This method needs to be @Nullable because a new Builder constructor {@link
    // Builder#Builder() was added in Android S, and by using the new constructor the return value
    // of this method will be null if no network was set.
    // For apps that are using a null-safe language, making this method @Nullable will break
    // compilation, and apps need to update their code. For apps that are not using null-safe
    // language, making this change will not break the backwards compatibility because for any app
    // that uses the deprecated constructor {@link Builder#Builder(Context)}, the return value of
    // this method is still guaranteed to be non-null.
    /**
     * Retrieves the configured {@link Network}, or null if was not set.
     *
     * <p>@see {@link Builder#setNetwork(Network)}
     */
    @Nullable
    public Network getNetwork() {
        return mDefaultOrConfiguredNetwork;
    }

    /**
     * Retrieves all IkeSaProposals configured
     *
     * @deprecated Callers should use {@link #getIkeSaProposals()}. This method is deprecated
     *     because its name does not match the return type.
     * @hide
     */
    @Deprecated
    @SystemApi
    @NonNull
    public List<IkeSaProposal> getSaProposals() {
        return getIkeSaProposals();
    }

    /** Retrieves all IkeSaProposals configured */
    @NonNull
    public List<IkeSaProposal> getIkeSaProposals() {
        return Arrays.asList(mSaProposals);
    }

    /** @hide */
    public IkeSaProposal[] getSaProposalsInternal() {
        return mSaProposals;
    }

    /** Retrieves the local (client) identity */
    @NonNull
    public IkeIdentification getLocalIdentification() {
        return mLocalIdentification;
    }

    /** Retrieves the required remote (server) identity */
    @NonNull
    public IkeIdentification getRemoteIdentification() {
        return mRemoteIdentification;
    }

    /** Retrieves the local (client) authentication configuration */
    @NonNull
    public IkeAuthConfig getLocalAuthConfig() {
        return mLocalAuthConfig;
    }

    /** Retrieves the remote (server) authentication configuration */
    @NonNull
    public IkeAuthConfig getRemoteAuthConfig() {
        return mRemoteAuthConfig;
    }

    /** Retrieves hard lifetime in seconds */
    // Use "second" because smaller unit won't make sense to describe a rekey interval.
    @SuppressLint("MethodNameUnits")
    @IntRange(from = IKE_HARD_LIFETIME_SEC_MINIMUM, to = IKE_HARD_LIFETIME_SEC_MAXIMUM)
    public int getHardLifetimeSeconds() {
        return mHardLifetimeSec;
    }

    /** Retrieves soft lifetime in seconds */
    // Use "second" because smaller unit does not make sense to a rekey interval.
    @SuppressLint("MethodNameUnits")
    @IntRange(from = IKE_SOFT_LIFETIME_SEC_MINIMUM, to = IKE_HARD_LIFETIME_SEC_MAXIMUM)
    public int getSoftLifetimeSeconds() {
        return mSoftLifetimeSec;
    }

    /** Retrieves the Dead Peer Detection(DPD) delay in seconds */
    // Use "second" because smaller unit does not make sense to a DPD delay.
    @SuppressLint("MethodNameUnits")
    @IntRange(from = IKE_DPD_DELAY_SEC_MIN, to = IKE_DPD_DELAY_SEC_MAX)
    public int getDpdDelaySeconds() {
        return mDpdDelaySec;
    }

    /** Retrieves the Network Address Translation Traversal (NATT) keepalive delay in seconds */
    // Use "second" because smaller unit does not make sense for a NATT Keepalive delay.
    @SuppressLint("MethodNameUnits")
    @IntRange(from = IKE_NATT_KEEPALIVE_DELAY_SEC_MIN, to = IKE_NATT_KEEPALIVE_DELAY_SEC_MAX)
    public int getNattKeepAliveDelaySeconds() {
        return mNattKeepaliveDelaySec;
    }

    /**
     * Retrieves the DSCP field of IKE packets.
     *
     * @hide
     */
    @SystemApi
    @IntRange(from = DSCP_MIN, to = DSCP_MAX)
    public int getDscp() {
        return mDscp;
    }

    /**
     * Retrieves the relative retransmission timeout list in milliseconds
     *
     * <p>@see {@link Builder#setRetransmissionTimeoutsMillis(int[])}
     */
    @NonNull
    public int[] getRetransmissionTimeoutsMillis() {
        return mRetransTimeoutMsList;
    }

    /**
     * Retrieves the configured Ike3gppExtension, or null if it was not set.
     *
     * @hide
     */
    @SystemApi
    @Nullable
    public Ike3gppExtension getIke3gppExtension() {
        return mIke3gppExtension;
    }

    private static boolean hasIkeOption(long ikeOptionsRecord, @IkeOption int ikeOption) {
        validateIkeOptionOrThrow(ikeOption);
        return (ikeOptionsRecord & getOptionBitValue(ikeOption)) != 0;
    }

    /** Checks if the given IKE Session negotiation option is set */
    public boolean hasIkeOption(@IkeOption int ikeOption) {
        return hasIkeOption(mIkeOptions, ikeOption);
    }

    /** @hide */
    public long getHardLifetimeMsInternal() {
        return TimeUnit.SECONDS.toMillis((long) mHardLifetimeSec);
    }

    /** @hide */
    public long getSoftLifetimeMsInternal() {
        return TimeUnit.SECONDS.toMillis((long) mSoftLifetimeSec);
    }

    /** @hide */
    public boolean isIkeFragmentationSupported() {
        return mIsIkeFragmentationSupported;
    }

    /** @hide */
    public IkeConfigAttribute[] getConfigurationAttributesInternal() {
        return mConfigRequests;
    }

    /**
     * Retrieves the list of Configuration Requests
     *
     * @hide
     */
    @SystemApi
    @NonNull
    public List<IkeConfigRequest> getConfigurationRequests() {
        return Collections.unmodifiableList(Arrays.asList(mConfigRequests));
    }

    /** @hide */
    @Override
    public int hashCode() {
        return Objects.hash(
                mServerHostname,
                mCallerConfiguredNetwork,
                Arrays.hashCode(mSaProposals),
                mLocalIdentification,
                mRemoteIdentification,
                mLocalAuthConfig,
                mRemoteAuthConfig,
                mIke3gppExtension,
                Arrays.hashCode(mConfigRequests),
                Arrays.hashCode(mRetransTimeoutMsList),
                mIkeOptions,
                mHardLifetimeSec,
                mSoftLifetimeSec,
                mDpdDelaySec,
                mNattKeepaliveDelaySec,
                mDscp,
                mIsIkeFragmentationSupported);
    }

    /** @hide */
    @Override
    public boolean equals(Object o) {
        if (!(o instanceof IkeSessionParams)) {
            return false;
        }

        IkeSessionParams other = (IkeSessionParams) o;

        return mServerHostname.equals(other.mServerHostname)
                && Objects.equals(mCallerConfiguredNetwork, other.mCallerConfiguredNetwork)
                && Arrays.equals(mSaProposals, other.mSaProposals)
                && mLocalIdentification.equals(other.mLocalIdentification)
                && mRemoteIdentification.equals(other.mRemoteIdentification)
                && mLocalAuthConfig.equals(other.mLocalAuthConfig)
                && mRemoteAuthConfig.equals(other.mRemoteAuthConfig)
                && Objects.equals(mIke3gppExtension, other.mIke3gppExtension)
                && Arrays.equals(mConfigRequests, other.mConfigRequests)
                && Arrays.equals(mRetransTimeoutMsList, other.mRetransTimeoutMsList)
                && mIkeOptions == other.mIkeOptions
                && mHardLifetimeSec == other.mHardLifetimeSec
                && mSoftLifetimeSec == other.mSoftLifetimeSec
                && mDpdDelaySec == other.mDpdDelaySec
                && mNattKeepaliveDelaySec == other.mNattKeepaliveDelaySec
                && mDscp == other.mDscp
                && mIsIkeFragmentationSupported == other.mIsIkeFragmentationSupported;
    }

    /**
     * Represents an IKE session configuration request type
     *
     * @hide
     */
    @SystemApi
    public interface IkeConfigRequest {}

    /**
     * Represents an IPv4 P_CSCF request
     *
     * @hide
     */
    @SystemApi
    public interface ConfigRequestIpv4PcscfServer extends IkeConfigRequest {
        /**
         * Retrieves the requested IPv4 P_CSCF server address
         *
         * @return The requested P_CSCF server address, or null if no specific P_CSCF server was
         *     requested
         */
        @Nullable
        Inet4Address getAddress();
    }

    /**
     * Represents an IPv6 P_CSCF request
     *
     * @hide
     */
    @SystemApi
    public interface ConfigRequestIpv6PcscfServer extends IkeConfigRequest {
        /**
         * Retrieves the requested IPv6 P_CSCF server address
         *
         * @return The requested P_CSCF server address, or null if no specific P_CSCF server was
         *     requested
         */
        @Nullable
        Inet6Address getAddress();
    }

    /** This class contains common information of an IKEv2 authentication configuration. */
    public abstract static class IkeAuthConfig {
        private static final String AUTH_METHOD_KEY = "mAuthMethod";
        private static final String AUTH_DIRECTION_KEY = "mAuthDirection";
        /** @hide */
        @IkeAuthMethod public final int mAuthMethod;
        /** @hide */
        @AuthDirection public final int mAuthDirection;

        /** @hide */
        IkeAuthConfig(@IkeAuthMethod int authMethod, @AuthDirection int authDirection) {
            mAuthMethod = authMethod;
            mAuthDirection = authDirection;
        }

        /**
         * Constructs this object by deserializing a PersistableBundle
         *
         * @hide
         */
        @NonNull
        public static IkeAuthConfig fromPersistableBundle(PersistableBundle in) {
            Objects.requireNonNull(in, "PersistableBundle is null");

            int authMethod = in.getInt(AUTH_METHOD_KEY);
            switch (authMethod) {
                case IKE_AUTH_METHOD_PSK:
                    return IkeAuthPskConfig.fromPersistableBundle(in);
                case IKE_AUTH_METHOD_PUB_KEY_SIGNATURE:
                    switch (in.getInt(AUTH_DIRECTION_KEY)) {
                        case AUTH_DIRECTION_LOCAL:
                            return IkeAuthDigitalSignLocalConfig.fromPersistableBundle(in);
                        case AUTH_DIRECTION_REMOTE:
                            return IkeAuthDigitalSignRemoteConfig.fromPersistableBundle(in);
                        default:
                            throw new IllegalArgumentException(
                                    "Digital-signature-based auth configuration with invalid"
                                            + " direction: "
                                            + in.getInt(AUTH_DIRECTION_KEY));
                    }
                case IKE_AUTH_METHOD_EAP:
                    return IkeAuthEapConfig.fromPersistableBundle(in);
                default:
                    throw new IllegalArgumentException("Invalid Auth Method: " + authMethod);
            }
        }

        /**
         * Serializes this object to a PersistableBundle
         *
         * @hide
         */
        @NonNull
        protected PersistableBundle toPersistableBundle() {
            final PersistableBundle result = new PersistableBundle();

            result.putInt(AUTH_METHOD_KEY, mAuthMethod);
            result.putInt(AUTH_DIRECTION_KEY, mAuthDirection);
            return result;
        }

        @Override
        public int hashCode() {
            return Objects.hash(mAuthMethod, mAuthDirection);
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof IkeAuthConfig)) {
                return false;
            }

            IkeAuthConfig other = (IkeAuthConfig) o;

            return mAuthMethod == other.mAuthMethod && mAuthDirection == other.mAuthDirection;
        }
    }

    /**
     * This class represents the configuration to support IKEv2 pre-shared-key-based authentication
     * of local or remote side.
     */
    public static class IkeAuthPskConfig extends IkeAuthConfig {
        private static final String PSK_KEY = "mPsk";
        /** @hide */
        @NonNull public final byte[] mPsk;

        /** @hide */
        @VisibleForTesting
        IkeAuthPskConfig(byte[] psk) {
            super(IKE_AUTH_METHOD_PSK, AUTH_DIRECTION_BOTH);
            mPsk = psk;
        }

        /**
         * Constructs this object by deserializing a PersistableBundle
         *
         * @hide
         */
        @NonNull
        public static IkeAuthPskConfig fromPersistableBundle(@NonNull PersistableBundle in) {
            Objects.requireNonNull(in, "PersistableBundle is null");

            PersistableBundle pskBundle = in.getPersistableBundle(PSK_KEY);
            Objects.requireNonNull(in, "PSK bundle is null");

            return new IkeAuthPskConfig(PersistableBundleUtils.toByteArray(pskBundle));
        }

        /**
         * Serializes this object to a PersistableBundle
         *
         * @hide
         */
        @Override
        @NonNull
        public PersistableBundle toPersistableBundle() {
            final PersistableBundle result = super.toPersistableBundle();

            result.putPersistableBundle(PSK_KEY, PersistableBundleUtils.fromByteArray(mPsk));
            return result;
        }

        /** Retrieves the pre-shared key */
        @NonNull
        public byte[] getPsk() {
            return Arrays.copyOf(mPsk, mPsk.length);
        }

        @Override
        public int hashCode() {
            return Objects.hash(super.hashCode(), Arrays.hashCode(mPsk));
        }

        @Override
        public boolean equals(Object o) {
            if (!super.equals(o) || !(o instanceof IkeAuthPskConfig)) {
                return false;
            }

            return Arrays.equals(mPsk, ((IkeAuthPskConfig) o).mPsk);
        }
    }

    /**
     * This class represents the configuration to support IKEv2 public-key-signature-based
     * authentication of the remote side.
     */
    public static class IkeAuthDigitalSignRemoteConfig extends IkeAuthConfig {
        private static final String TRUST_CERT_KEY = "TRUST_CERT_KEY";
        /** @hide */
        @Nullable public final TrustAnchor mTrustAnchor;

        /**
         * If a certificate is provided, it MUST be the root CA used by the remote (server), or
         * authentication will fail. If no certificate is provided, any root CA in the system's
         * truststore is considered acceptable.
         *
         * @hide
         */
        @VisibleForTesting
        IkeAuthDigitalSignRemoteConfig(@Nullable X509Certificate caCert) {
            super(IKE_AUTH_METHOD_PUB_KEY_SIGNATURE, AUTH_DIRECTION_REMOTE);
            if (caCert == null) {
                mTrustAnchor = null;
            } else {
                // The name constraints extension, defined in RFC 5280, indicates a name space
                // within which all subject names in subsequent certificates in a certification path
                // MUST be located.
                mTrustAnchor = new TrustAnchor(caCert, null /*nameConstraints*/);

                // TODO: Investigate if we need to support the name constraints extension.
            }
        }

        /**
         * Constructs this object by deserializing a PersistableBundle
         *
         * @hide
         */
        @NonNull
        public static IkeAuthDigitalSignRemoteConfig fromPersistableBundle(
                @NonNull PersistableBundle in) {
            Objects.requireNonNull(in, "PersistableBundle is null");

            PersistableBundle trustCertBundle = in.getPersistableBundle(TRUST_CERT_KEY);

            X509Certificate caCert = null;
            if (trustCertBundle != null) {
                byte[] encodedCert = PersistableBundleUtils.toByteArray(trustCertBundle);
                caCert = certificateFromByteArray(encodedCert);
            }

            return new IkeAuthDigitalSignRemoteConfig(caCert);
        }

        /**
         * Serializes this object to a PersistableBundle
         *
         * @hide
         */
        @Override
        @NonNull
        public PersistableBundle toPersistableBundle() {
            final PersistableBundle result = super.toPersistableBundle();

            try {
                if (mTrustAnchor != null) {
                    result.putPersistableBundle(
                            TRUST_CERT_KEY,
                            PersistableBundleUtils.fromByteArray(
                                    mTrustAnchor.getTrustedCert().getEncoded()));
                }

            } catch (CertificateEncodingException e) {
                throw new IllegalArgumentException("Fail to encode the certificate");
            }

            return result;
        }

        /** Retrieves the provided CA certificate for validating the remote certificate(s) */
        @Nullable
        public X509Certificate getRemoteCaCert() {
            if (mTrustAnchor == null) return null;
            return mTrustAnchor.getTrustedCert();
        }

        @Override
        public int hashCode() {
            // Use #getTrustedCert() because TrustAnchor does not override #hashCode()
            return Objects.hash(
                    super.hashCode(),
                    (mTrustAnchor == null) ? null : mTrustAnchor.getTrustedCert());
        }

        @Override
        public boolean equals(Object o) {
            if (!super.equals(o) || !(o instanceof IkeAuthDigitalSignRemoteConfig)) {
                return false;
            }

            IkeAuthDigitalSignRemoteConfig other = (IkeAuthDigitalSignRemoteConfig) o;

            if (mTrustAnchor == null && other.mTrustAnchor == null) {
                return true;
            }

            // Compare #getTrustedCert() because TrustAnchor does not override #equals(Object)
            return mTrustAnchor != null
                    && other.mTrustAnchor != null
                    && Objects.equals(
                            mTrustAnchor.getTrustedCert(), other.mTrustAnchor.getTrustedCert());
        }
    }

    /**
     * This class represents the configuration to support IKEv2 public-key-signature-based
     * authentication of the local side.
     */
    public static class IkeAuthDigitalSignLocalConfig extends IkeAuthConfig {
        private static final String END_CERT_KEY = "mEndCert";
        private static final String INTERMEDIATE_CERTS_KEY = "mIntermediateCerts";
        private static final String PRIVATE_KEY_KEY = "mPrivateKey";
        /** @hide */
        @NonNull public final X509Certificate mEndCert;

        /** @hide */
        @NonNull public final List<X509Certificate> mIntermediateCerts;

        /** @hide */
        @NonNull public final PrivateKey mPrivateKey;

        /** @hide */
        @VisibleForTesting
        IkeAuthDigitalSignLocalConfig(
                @NonNull X509Certificate clientEndCert,
                @NonNull List<X509Certificate> clientIntermediateCerts,
                @NonNull PrivateKey privateKey) {
            super(IKE_AUTH_METHOD_PUB_KEY_SIGNATURE, AUTH_DIRECTION_LOCAL);
            mEndCert = clientEndCert;
            mIntermediateCerts = clientIntermediateCerts;
            mPrivateKey = privateKey;
        }

        /**
         * Constructs this object by deserializing a PersistableBundle
         *
         * @hide
         */
        @NonNull
        public static IkeAuthDigitalSignLocalConfig fromPersistableBundle(
                @NonNull PersistableBundle in) {
            Objects.requireNonNull(in, "PersistableBundle is null");

            PersistableBundle endCertBundle = in.getPersistableBundle(END_CERT_KEY);
            Objects.requireNonNull(endCertBundle, "End cert not provided");
            byte[] encodedCert = PersistableBundleUtils.toByteArray(endCertBundle);
            X509Certificate endCert = certificateFromByteArray(encodedCert);

            PersistableBundle certsBundle = in.getPersistableBundle(INTERMEDIATE_CERTS_KEY);
            Objects.requireNonNull(certsBundle, "Intermediate certs not provided");
            List<byte[]> encodedCertList =
                    PersistableBundleUtils.toList(certsBundle, PersistableBundleUtils::toByteArray);
            List<X509Certificate> certList = new ArrayList<>(encodedCertList.size());
            for (byte[] encoded : encodedCertList) {
                certList.add(certificateFromByteArray(encoded));
            }

            PersistableBundle privateKeyBundle = in.getPersistableBundle(PRIVATE_KEY_KEY);
            Objects.requireNonNull(privateKeyBundle, "PrivateKey bundle is null");
            PrivateKey privateKey =
                    privateKeyFromByteArray(PersistableBundleUtils.toByteArray(privateKeyBundle));
            Objects.requireNonNull(privateKeyBundle, "PrivateKey is null");

            return new IkeAuthDigitalSignLocalConfig(endCert, certList, privateKey);
        }

        /**
         * Serializes this object to a PersistableBundle
         *
         * @hide
         */
        @Override
        @NonNull
        public PersistableBundle toPersistableBundle() {
            final PersistableBundle result = super.toPersistableBundle();

            try {
                result.putPersistableBundle(
                        END_CERT_KEY, PersistableBundleUtils.fromByteArray(mEndCert.getEncoded()));

                List<byte[]> encodedCertList = new ArrayList<>(mIntermediateCerts.size());
                for (X509Certificate cert : mIntermediateCerts) {
                    encodedCertList.add(cert.getEncoded());
                }
                PersistableBundle certsBundle =
                        PersistableBundleUtils.fromList(
                                encodedCertList, PersistableBundleUtils::fromByteArray);
                result.putPersistableBundle(INTERMEDIATE_CERTS_KEY, certsBundle);
            } catch (CertificateEncodingException e) {
                throw new IllegalArgumentException("Fail to encode certificate");
            }

            // TODO: b/170670506 Consider putting PrivateKey in Android KeyStore
            result.putPersistableBundle(
                    PRIVATE_KEY_KEY,
                    PersistableBundleUtils.fromByteArray(mPrivateKey.getEncoded()));

            return result;
        }

        /** Retrieves the client end certificate */
        @NonNull
        public X509Certificate getClientEndCertificate() {
            return mEndCert;
        }

        /** Retrieves the intermediate certificates */
        @NonNull
        public List<X509Certificate> getIntermediateCertificates() {
            return mIntermediateCerts;
        }

        /** Retrieves the private key */
        @NonNull
        public PrivateKey getPrivateKey() {
            return mPrivateKey;
        }

        @Override
        public int hashCode() {
            return Objects.hash(super.hashCode(), mEndCert, mIntermediateCerts, mPrivateKey);
        }

        @Override
        public boolean equals(Object o) {
            if (!super.equals(o) || !(o instanceof IkeAuthDigitalSignLocalConfig)) {
                return false;
            }

            IkeAuthDigitalSignLocalConfig other = (IkeAuthDigitalSignLocalConfig) o;

            return mEndCert.equals(other.mEndCert)
                    && mIntermediateCerts.equals(other.mIntermediateCerts)
                    && mPrivateKey.equals(other.mPrivateKey);
        }
    }

    /**
     * This class represents the configuration to support EAP authentication of the local side.
     *
     * <p>@see {@link IkeSessionParams.Builder#setAuthEap(X509Certificate, EapSessionConfig)}
     */
    public static class IkeAuthEapConfig extends IkeAuthConfig {
        private static final String EAP_CONFIG_KEY = "mEapConfig";

        /** @hide */
        @NonNull public final EapSessionConfig mEapConfig;

        /** @hide */
        @VisibleForTesting
        IkeAuthEapConfig(EapSessionConfig eapConfig) {
            super(IKE_AUTH_METHOD_EAP, AUTH_DIRECTION_LOCAL);

            mEapConfig = eapConfig;
        }

        /**
         * Constructs this object by deserializing a PersistableBundle
         *
         * @hide
         */
        @NonNull
        public static IkeAuthEapConfig fromPersistableBundle(@NonNull PersistableBundle in) {
            Objects.requireNonNull(in, "PersistableBundle null");

            PersistableBundle eapBundle = in.getPersistableBundle(EAP_CONFIG_KEY);
            Objects.requireNonNull(in, "EAP Config bundle is null");

            EapSessionConfig eapConfig = EapSessionConfig.fromPersistableBundle(eapBundle);
            Objects.requireNonNull(eapConfig, "EAP Config is null");

            return new IkeAuthEapConfig(eapConfig);
        }

        /**
         * Serializes this object to a PersistableBundle
         *
         * @hide
         */
        @Override
        @NonNull
        public PersistableBundle toPersistableBundle() {
            final PersistableBundle result = super.toPersistableBundle();
            result.putPersistableBundle(EAP_CONFIG_KEY, mEapConfig.toPersistableBundle());
            return result;
        }

        /** Retrieves EAP configuration */
        @NonNull
        public EapSessionConfig getEapConfig() {
            return mEapConfig;
        }

        @Override
        public int hashCode() {
            return Objects.hash(super.hashCode(), mEapConfig);
        }

        @Override
        public boolean equals(Object o) {
            if (!super.equals(o) || !(o instanceof IkeAuthEapConfig)) {
                return false;
            }

            return mEapConfig.equals(((IkeAuthEapConfig) o).mEapConfig);
        }
    }

    /** This class can be used to incrementally construct a {@link IkeSessionParams}. */
    public static final class Builder {
        // This field has changed from @NonNull to @Nullable since Android S. It has to be @Nullable
        // because the new constructor #Builder() will not need and will not able to get a
        // ConnectivityManager instance anymore. Making it @Nullable does not break the backwards
        // compatibility because if apps use the old constructor #Builder(Context), the Builder and
        // the IkeSessionParams built from it will still work in the old way. @see #Builder(Context)
        @Nullable private ConnectivityManager mConnectivityManager;

        @NonNull private final List<IkeSaProposal> mSaProposalList = new LinkedList<>();
        @NonNull private final List<IkeConfigAttribute> mConfigRequestList = new ArrayList<>();

        @NonNull
        private int[] mRetransTimeoutMsList =
                Arrays.copyOf(
                        IKE_RETRANS_TIMEOUT_MS_LIST_DEFAULT,
                        IKE_RETRANS_TIMEOUT_MS_LIST_DEFAULT.length);

        @NonNull private String mServerHostname;
        @Nullable private Network mCallerConfiguredNetwork;

        @Nullable private IkeIdentification mLocalIdentification;
        @Nullable private IkeIdentification mRemoteIdentification;

        @Nullable private IkeAuthConfig mLocalAuthConfig;
        @Nullable private IkeAuthConfig mRemoteAuthConfig;

        @Nullable private Ike3gppExtension mIke3gppExtension;

        private long mIkeOptions = 0;

        private int mHardLifetimeSec = IKE_HARD_LIFETIME_SEC_DEFAULT;
        private int mSoftLifetimeSec = IKE_SOFT_LIFETIME_SEC_DEFAULT;

        private int mDpdDelaySec = IKE_DPD_DELAY_SEC_DEFAULT;
        private int mNattKeepaliveDelaySec = IKE_NATT_KEEPALIVE_DELAY_SEC_DEFAULT;
        private int mDscp = DSCP_DEFAULT;

        private final boolean mIsIkeFragmentationSupported = true;

        /**
         * Construct Builder
         *
         * <p>This constructor is deprecated since Android S. Apps that use this constructor can
         * still expect {@link #build()} to throw if no configured or default network was found. But
         * apps that use {@link #Builder()} MUST NOT expect that behavior anymore.
         *
         * <p>For a caller that used this constructor and did not set any Network, {@link
         * IkeSessionParams#getNetwork()} will return the default Network resolved in {@link
         * IkeSessionParams.Builder#build()}. This return value is only informational because if
         * MOBIKE is enabled, IKE Session may switch to a different default Network.
         *
         * @param context a valid {@link Context} instance.
         * @deprecated Callers should use {@link #Builder()}.This method is deprecated because it is
         *     unnecessary to try resolving a default network or to validate network is connected
         *     before {@link IkeSession} starts the setup process.
         * @hide
         */
        @Deprecated
        @SystemApi
        public Builder(@NonNull Context context) {
            this((ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE));
        }

        /**
         * Construct Builder
         */
        public Builder() {}

        /** @hide */
        // TODO: b/178389011 This constructor should be removed when #Builder(Context) can be safely
        // removed. See #Builder(Context) for reasons.
        @VisibleForTesting
        public Builder(ConnectivityManager connectManager) {
            mConnectivityManager = connectManager;
        }

        /**
         * Construct Builder from the {@link IkeSessionParams} object.
         *
         * @param ikeSessionParams the object this Builder will be constructed with.
         */
        public Builder(@NonNull IkeSessionParams ikeSessionParams) {
            mSaProposalList.addAll(ikeSessionParams.getSaProposals());
            mConfigRequestList.addAll(Arrays.asList(ikeSessionParams.mConfigRequests));

            int[] retransmissionTimeouts = ikeSessionParams.getRetransmissionTimeoutsMillis();
            mRetransTimeoutMsList =
                    Arrays.copyOf(retransmissionTimeouts, retransmissionTimeouts.length);

            mServerHostname = ikeSessionParams.getServerHostname();
            mCallerConfiguredNetwork = ikeSessionParams.getConfiguredNetwork();
            mLocalIdentification = ikeSessionParams.getLocalIdentification();
            mRemoteIdentification = ikeSessionParams.getRemoteIdentification();
            mLocalAuthConfig = ikeSessionParams.getLocalAuthConfig();
            mRemoteAuthConfig = ikeSessionParams.getRemoteAuthConfig();

            mIke3gppExtension = ikeSessionParams.getIke3gppExtension();

            mHardLifetimeSec = ikeSessionParams.getHardLifetimeSeconds();
            mSoftLifetimeSec = ikeSessionParams.getSoftLifetimeSeconds();
            mDpdDelaySec = ikeSessionParams.getDpdDelaySeconds();
            mNattKeepaliveDelaySec = ikeSessionParams.getNattKeepAliveDelaySeconds();
            mDscp = ikeSessionParams.getDscp();

            mIkeOptions = ikeSessionParams.mIkeOptions;

            if (!ikeSessionParams.mIsIkeFragmentationSupported) {
                throw new IllegalStateException(
                        "mIsIkeFragmentationSupported should never be false");
            }
        }

        /**
         * Sets the server hostname for the {@link IkeSessionParams} being built.
         *
         * @param serverHostname the hostname of the IKE server, such as "ike.android.com".
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setServerHostname(@NonNull String serverHostname) {
            Objects.requireNonNull(serverHostname, "Required argument not provided");

            mServerHostname = serverHostname;
            return this;
        }

        /**
         * Sets the {@link Network} for the {@link IkeSessionParams} being built.
         *
         * <p>If no {@link Network} is provided, the default Network (as per {@link
         * ConnectivityManager#getActiveNetwork()}) will be used when constructing an {@link
         * IkeSession}.
         *
         * @param network the {@link Network} that IKE Session will use, or {@code null} to clear
         *     the previously set {@link Network}
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setNetwork(@Nullable Network network) {
            mCallerConfiguredNetwork = network;
            return this;
        }

        /**
         * Sets local IKE identification for the {@link IkeSessionParams} being built.
         *
         * <p>It is not allowed to use KEY ID together with digital-signature-based authentication
         * as per RFC 7296.
         *
         * @param identification the local IKE identification.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setLocalIdentification(@NonNull IkeIdentification identification) {
            if (identification == null) {
                throw new NullPointerException("Required argument not provided");
            }

            mLocalIdentification = identification;
            return this;
        }

        /**
         * Sets remote IKE identification for the {@link IkeSessionParams} being built.
         *
         * @param identification the remote IKE identification.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setRemoteIdentification(@NonNull IkeIdentification identification) {
            if (identification == null) {
                throw new NullPointerException("Required argument not provided");
            }

            mRemoteIdentification = identification;
            return this;
        }

        /**
         * Adds an IKE SA proposal to the {@link IkeSessionParams} being built.
         *
         * @param proposal IKE SA proposal.
         * @return Builder this, to facilitate chaining.
         * @deprecated Callers should use {@link #addIkeSaProposal(IkeSaProposal)}. This method is
         *     deprecated because its name does not match the input type.
         * @hide
         */
        @Deprecated
        @SystemApi
        @NonNull
        public Builder addSaProposal(@NonNull IkeSaProposal proposal) {
            return addIkeSaProposal(proposal);
        }

        /**
         * Adds an IKE SA proposal to the {@link IkeSessionParams} being built.
         *
         * @param proposal IKE SA proposal.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder addIkeSaProposal(@NonNull IkeSaProposal proposal) {
            if (proposal == null) {
                throw new NullPointerException("Required argument not provided");
            }

            if (proposal.getProtocolId() != IkePayload.PROTOCOL_ID_IKE) {
                throw new IllegalArgumentException(
                        "Expected IKE SA Proposal but received Child SA proposal");
            }
            mSaProposalList.add(proposal);
            return this;
        }

        /**
         * Configures authentication for IKE Session. Internal use only.
         *
         * @hide
         */
        @NonNull
        private Builder setAuth(IkeAuthConfig local, IkeAuthConfig remote) {
            mLocalAuthConfig = local;
            mRemoteAuthConfig = remote;
            return this;
        }

        /**
         * Configures the {@link IkeSession} to use pre-shared-key-based authentication.
         *
         * <p>Both client and server MUST be authenticated using the provided shared key. IKE
         * authentication will fail if the remote peer tries to use other authentication methods.
         *
         * <p>Callers MUST declare only one authentication method. Calling this function will
         * override the previously set authentication configuration.
         *
         * <p>Callers SHOULD NOT use this if any other authentication methods can be used; PSK-based
         * authentication is generally considered insecure.
         *
         * @param sharedKey the shared key.
         * @return Builder this, to facilitate chaining.
         */
        // #getLocalAuthConfig and #getRemoveAuthConfig are defined to retrieve
        // authentication configurations
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder setAuthPsk(@NonNull byte[] sharedKey) {
            if (sharedKey == null) {
                throw new NullPointerException("Required argument not provided");
            }

            return setAuth(new IkeAuthPskConfig(sharedKey), new IkeAuthPskConfig(sharedKey));
        }

        /**
         * Configures the {@link IkeSession} to use EAP authentication.
         *
         * <p>Not all EAP methods provide mutual authentication. As such EAP MUST be used in
         * conjunction with a public-key-signature-based authentication of the remote server, unless
         * EAP-Only authentication is enabled.
         *
         * <p>Callers may enable EAP-Only authentication by setting {@link
         * #IKE_OPTION_EAP_ONLY_AUTH}, which will make IKE library request the remote to use
         * EAP-Only authentication. The remote may opt to reject the request, at which point the
         * received certificates and authentication payload WILL be validated with the provided root
         * CA or system's truststore as usual. Only safe EAP methods as listed in RFC 5998 will be
         * accepted for EAP-Only authentication.
         *
         * <p>If {@link #IKE_OPTION_EAP_ONLY_AUTH} is set, callers MUST configure EAP as the
         * authentication method and all EAP methods set in EAP Session configuration MUST be safe
         * methods that are accepted for EAP-Only authentication. Otherwise callers will get an
         * exception when building the {@link IkeSessionParams}
         *
         * <p>Callers MUST declare only one authentication method. Calling this function will
         * override the previously set authentication configuration.
         *
         * @see <a href="https://tools.ietf.org/html/rfc5280">RFC 5280, Internet X.509 Public Key
         *     Infrastructure Certificate and Certificate Revocation List (CRL) Profile</a>
         * @see <a href="https://tools.ietf.org/html/rfc5998">RFC 5998, An Extension for EAP-Only
         *     Authentication in IKEv2</a>
         * @param serverCaCert the CA certificate for validating the received server certificate(s).
         *     If a certificate is provided, it MUST be the root CA used by the server, or
         *     authentication will fail. If no certificate is provided, any root CA in the system's
         *     truststore is considered acceptable.
         * @return Builder this, to facilitate chaining.
         */
        // TODO(b/151667921): Consider also supporting configuring EAP method that is not accepted
        // by EAP-Only when {@link #IKE_OPTION_EAP_ONLY_AUTH} is set
        // MissingGetterMatchingBuilder: #getLocalAuthConfig and #getRemoveAuthConfig are defined to
        // retrieve authentication configurations
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder setAuthEap(
                @Nullable X509Certificate serverCaCert, @NonNull EapSessionConfig eapConfig) {
            if (eapConfig == null) {
                throw new NullPointerException("Required argument not provided");
            }

            return setAuth(
                    new IkeAuthEapConfig(eapConfig),
                    new IkeAuthDigitalSignRemoteConfig(serverCaCert));
        }

        /**
         * Configures the {@link IkeSession} to use public-key-signature-based authentication.
         *
         * <p>The public key included by the client end certificate and the private key used for
         * signing MUST be a matching key pair.
         *
         * <p>The IKE library will use the strongest signature algorithm supported by both sides.
         *
         * <p>Currenly only RSA digital signature is supported.
         *
         * @param serverCaCert the CA certificate for validating the received server certificate(s).
         *     If a certificate is provided, it MUST be the root CA used by the server, or
         *     authentication will fail. If no certificate is provided, any root CA in the system's
         *     truststore is considered acceptable.
         * @param clientEndCert the end certificate for remote server to verify the locally
         *     generated signature.
         * @param clientPrivateKey private key to generate outbound digital signature. The {@link
         *     PrivateKey} MUST be an instance of {@link RSAKey}.
         * @return Builder this, to facilitate chaining.
         */
        // #getLocalAuthConfig and #getRemoveAuthConfig are defined to retrieve
        // authentication configurations
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder setAuthDigitalSignature(
                @Nullable X509Certificate serverCaCert,
                @NonNull X509Certificate clientEndCert,
                @NonNull PrivateKey clientPrivateKey) {
            return setAuthDigitalSignature(
                    serverCaCert,
                    clientEndCert,
                    new LinkedList<X509Certificate>(),
                    clientPrivateKey);
        }

        /**
         * Configures the {@link IkeSession} to use public-key-signature-based authentication.
         *
         * <p>The public key included by the client end certificate and the private key used for
         * signing MUST be a matching key pair.
         *
         * <p>The IKE library will use the strongest signature algorithm supported by both sides.
         *
         * <p>Currenly only RSA digital signature is supported.
         *
         * @param serverCaCert the CA certificate for validating the received server certificate(s).
         *     If a null value is provided, IKE library will try all default CA certificates stored
         *     in Android system to do the validation. Otherwise, it will only use the provided CA
         *     certificate.
         * @param clientEndCert the end certificate for remote server to verify locally generated
         *     signature.
         * @param clientIntermediateCerts intermediate certificates for the remote server to
         *     validate the end certificate.
         * @param clientPrivateKey private key to generate outbound digital signature. The {@link
         *     PrivateKey} MUST be an instance of {@link RSAKey}.
         * @return Builder this, to facilitate chaining.
         */
        // #getLocalAuthConfig and #getRemoveAuthConfig are defined to retrieve
        // authentication configurations
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder setAuthDigitalSignature(
                @Nullable X509Certificate serverCaCert,
                @NonNull X509Certificate clientEndCert,
                @NonNull List<X509Certificate> clientIntermediateCerts,
                @NonNull PrivateKey clientPrivateKey) {
            if (clientEndCert == null
                    || clientIntermediateCerts == null
                    || clientPrivateKey == null) {
                throw new NullPointerException("Required argument not provided");
            }

            if (!(clientPrivateKey instanceof RSAKey)) {
                throw new IllegalArgumentException("Unsupported private key type");
            }

            IkeAuthConfig localConfig =
                    new IkeAuthDigitalSignLocalConfig(
                            clientEndCert, clientIntermediateCerts, clientPrivateKey);
            IkeAuthConfig remoteConfig = new IkeAuthDigitalSignRemoteConfig(serverCaCert);

            return setAuth(localConfig, remoteConfig);
        }

        /**
         * Adds a configuration request. Internal use only.
         *
         * @hide
         */
        @NonNull
        private Builder addConfigRequest(IkeConfigAttribute configReq) {
            mConfigRequestList.add(configReq);
            return this;
        }

        /**
         * Adds a specific internal P_CSCF server request to the {@link IkeSessionParams} being
         * built.
         *
         * @param address the requested P_CSCF address.
         * @return Builder this, to facilitate chaining.
         * @hide
         */
        // #getConfigurationRequests is defined to retrieve PCSCF server requests
        @SuppressLint("MissingGetterMatchingBuilder")
        @SystemApi
        @NonNull
        public Builder addPcscfServerRequest(@NonNull InetAddress address) {
            if (address == null) {
                throw new NullPointerException("Required argument not provided");
            }

            if (address instanceof Inet4Address) {
                return addConfigRequest(new ConfigAttributeIpv4Pcscf((Inet4Address) address));
            } else if (address instanceof Inet6Address) {
                return addConfigRequest(new ConfigAttributeIpv6Pcscf((Inet6Address) address));
            } else {
                throw new IllegalArgumentException("Invalid address family");
            }
        }

        /**
         * Adds a internal P_CSCF server request to the {@link IkeSessionParams} being built.
         *
         * @param addressFamily the address family. Only {@code AF_INET} and {@code AF_INET6} are
         *     allowed.
         * @return Builder this, to facilitate chaining.
         * @hide
         */
        // #getConfigurationRequests is defined to retrieve PCSCF server requests
        @SuppressLint("MissingGetterMatchingBuilder")
        @SystemApi
        @NonNull
        public Builder addPcscfServerRequest(int addressFamily) {
            if (addressFamily == AF_INET) {
                return addConfigRequest(new ConfigAttributeIpv4Pcscf());
            } else if (addressFamily == AF_INET6) {
                return addConfigRequest(new ConfigAttributeIpv6Pcscf());
            } else {
                throw new IllegalArgumentException("Invalid address family: " + addressFamily);
            }
        }

        /**
         * Sets hard and soft lifetimes.
         *
         * <p>Lifetimes will not be negotiated with the remote IKE server.
         *
         * @param hardLifetimeSeconds number of seconds after which IKE SA will expire. Defaults to
         *     14400 seconds (4 hours). MUST be a value from 300 seconds (5 minutes) to 86400
         *     seconds (24 hours), inclusive.
         * @param softLifetimeSeconds number of seconds after which IKE SA will request rekey.
         *     Defaults to 7200 seconds (2 hours). MUST be at least 120 seconds (2 minutes), and at
         *     least 60 seconds (1 minute) shorter than the hard lifetime.
         * @return Builder this, to facilitate chaining.
         */
        // #getHardLifetimeSeconds and #getSoftLifetimeSeconds are defined for callers to retrieve
        // the lifetimes
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder setLifetimeSeconds(
                @IntRange(from = IKE_HARD_LIFETIME_SEC_MINIMUM, to = IKE_HARD_LIFETIME_SEC_MAXIMUM)
                        int hardLifetimeSeconds,
                @IntRange(from = IKE_SOFT_LIFETIME_SEC_MINIMUM, to = IKE_HARD_LIFETIME_SEC_MAXIMUM)
                        int softLifetimeSeconds) {
            if (hardLifetimeSeconds < IKE_HARD_LIFETIME_SEC_MINIMUM
                    || hardLifetimeSeconds > IKE_HARD_LIFETIME_SEC_MAXIMUM
                    || softLifetimeSeconds < IKE_SOFT_LIFETIME_SEC_MINIMUM
                    || hardLifetimeSeconds - softLifetimeSeconds
                            < IKE_LIFETIME_MARGIN_SEC_MINIMUM) {
                throw new IllegalArgumentException("Invalid lifetime value");
            }

            mHardLifetimeSec = hardLifetimeSeconds;
            mSoftLifetimeSec = softLifetimeSeconds;
            return this;
        }

        /**
         * Sets the Dead Peer Detection(DPD) delay in seconds.
         *
         * @param dpdDelaySeconds number of seconds after which IKE SA will initiate DPD if no
         *     inbound cryptographically protected IKE message was received. Defaults to 120
         *     seconds. MUST be a value from 20 seconds to 1800 seconds, inclusive.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setDpdDelaySeconds(
                @IntRange(from = IKE_DPD_DELAY_SEC_MIN, to = IKE_DPD_DELAY_SEC_MAX)
                        int dpdDelaySeconds) {
            if (dpdDelaySeconds < IKE_DPD_DELAY_SEC_MIN
                    || dpdDelaySeconds > IKE_DPD_DELAY_SEC_MAX) {
                throw new IllegalArgumentException("Invalid DPD delay value");
            }
            mDpdDelaySec = dpdDelaySeconds;
            return this;
        }

        /**
         * Sets the Network Address Translation Traversal (NATT) keepalive delay in seconds.
         *
         * @param nattKeepaliveDelaySeconds number of seconds between keepalive packet
         *     transmissions. Defaults to 10 seconds. MUST be a value from 10 seconds to 3600
         *     seconds, inclusive.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setNattKeepAliveDelaySeconds(
                @IntRange(
                                from = IKE_NATT_KEEPALIVE_DELAY_SEC_MIN,
                                to = IKE_NATT_KEEPALIVE_DELAY_SEC_MAX)
                        int nattKeepaliveDelaySeconds) {
            if (nattKeepaliveDelaySeconds < IKE_NATT_KEEPALIVE_DELAY_SEC_MIN
                    || nattKeepaliveDelaySeconds > IKE_NATT_KEEPALIVE_DELAY_SEC_MAX) {
                throw new IllegalArgumentException("Invalid NATT keepalive delay value");
            }
            mNattKeepaliveDelaySec = nattKeepaliveDelaySeconds;
            return this;
        }

        /**
         * Sets the DSCP field of the IKE packets.
         *
         * <p>Differentiated services code point (DSCP) is a 6-bit field in the IP header that is
         * used for packet classification and prioritization. The DSCP field is encoded in the 6
         * higher order bits of the Type of Service (ToS) in IPv4 header, or the traffic class (TC)
         * field in IPv6 header.
         *
         * <p>Any 6-bit values (0 to 63) are acceptable, whether IANA-defined, or
         * implementation-specific values.
         *
         * @see <a href="https://tools.ietf.org/html/rfc2474">RFC 2474, Definition of the
         *     Differentiated Services Field (DS Field) in the IPv4 and IPv6 Headers</a>
         * @see <a href="https://www.iana.org/assignments/dscp-registry/dscp-registry.xhtml">
         *     Differentiated Services Field Codepoints (DSCP)</a>
         * @param dscp the dscp value. Defaults to 0.
         * @return Builder this, to facilitate chaining.
         * @hide
         */
        @SystemApi
        @NonNull
        public Builder setDscp(@IntRange(from = DSCP_MIN, to = DSCP_MAX) int dscp) {
            if (dscp < DSCP_MIN || dscp > DSCP_MAX) {
                throw new IllegalArgumentException("Invalid DSCP value");
            }
            mDscp = dscp;
            return this;
        }

        /**
         * Sets the retransmission timeout list in milliseconds.
         *
         * <p>Configures the retransmission by providing an array of relative retransmission
         * timeouts in milliseconds. After sending out a request and before receiving the response,
         * the IKE Session will iterate through the array and wait for the relative timeout before
         * the next retry. If the last timeout is exceeded, the IKE Session will be terminated.
         *
         * <p>Each element in the array MUST be a value from 500 ms to 1800000 ms (30 minutes). The
         * length of the array MUST NOT exceed 10. This retransmission timeout list defaults to
         * {0.5s, 1s, 2s, 4s, 8s}
         *
         * @param retransTimeoutMillisList the array of relative retransmission timeout in
         *     milliseconds.
         * @return Builder this, to facilitate chaining.
         */
        @NonNull
        public Builder setRetransmissionTimeoutsMillis(@NonNull int[] retransTimeoutMillisList) {
            boolean isValid = true;
            if (retransTimeoutMillisList == null
                    || retransTimeoutMillisList.length == 0
                    || retransTimeoutMillisList.length > IKE_RETRANS_MAX_ATTEMPTS_MAX) {
                isValid = false;
            }
            for (int t : retransTimeoutMillisList) {
                if (t < IKE_RETRANS_TIMEOUT_MS_MIN || t > IKE_RETRANS_TIMEOUT_MS_MAX) {
                    isValid = false;
                }
            }
            if (!isValid) throw new IllegalArgumentException("Invalid retransmission timeout list");

            mRetransTimeoutMsList = retransTimeoutMillisList;
            return this;
        }

        /**
         * Sets the parameters to be used for 3GPP-specific behavior during the IKE Session.
         *
         * <p>Setting the Ike3gppExtension also enables support for non-configurable payloads, such
         * as the Notify - BACKOFF_TIMER payload.
         *
         * @see 3GPP ETSI TS 24.302: Access to the 3GPP Evolved Packet Core (EPC) via non-3GPP
         *     access networks
         * @param ike3gppExtension the Ike3gppExtension to use for this IKE Session.
         * @return Builder this, to facilitate chaining.
         * @hide
         */
        @SystemApi
        @NonNull
        public Builder setIke3gppExtension(@NonNull Ike3gppExtension ike3gppExtension) {
            Objects.requireNonNull(ike3gppExtension, "ike3gppExtension must not be null");

            mIke3gppExtension = ike3gppExtension;
            return this;
        }

        /**
         * Sets the specified IKE Option as enabled.
         *
         * @param ikeOption the option to be enabled.
         * @return Builder this, to facilitate chaining.
         */
        // Use #hasIkeOption instead of @getIkeOptions because #hasIkeOption allows callers to check
        // the presence of one IKE option more easily
        @SuppressLint("MissingGetterMatchingBuilder")
        @NonNull
        public Builder addIkeOption(@IkeOption int ikeOption) {
            validateIkeOptionOrThrow(ikeOption);
            if (ikeOption == IKE_OPTION_MOBIKE && !SdkLevel.isAtLeastS()) {
                throw new UnsupportedOperationException("MOBIKE only supported for S+");
            }
            mIkeOptions |= getOptionBitValue(ikeOption);
            return this;
        }

        /**
         * Resets (disables) the specified IKE Option.
         *
         * @param ikeOption the option to be disabled.
         * @return Builder this, to facilitate chaining.
         */
        // Use #removeIkeOption instead of #clearIkeOption because "clear" sounds indicating
        // clearing all enabled IKE options
        @SuppressLint("BuilderSetStyle")
        @NonNull
        public Builder removeIkeOption(@IkeOption int ikeOption) {
            validateIkeOptionOrThrow(ikeOption);
            mIkeOptions &= ~getOptionBitValue(ikeOption);
            return this;
        }

        /**
         * Validates and builds the {@link IkeSessionParams}.
         *
         * @return IkeSessionParams the validated IkeSessionParams.
         */
        @NonNull
        public IkeSessionParams build() {
            if (mSaProposalList.isEmpty()) {
                throw new IllegalArgumentException("IKE SA proposal not found");
            }

            // TODO: b/178389011 This code block should be removed when
            // IkeSessionParams#getNetwork() and #Builder(Context) can be safely removed. This block
            // makes sure if the Builder is constructed with the deprecated constructor
            // #Builder(Context), #build() still works in the same way and will throw exception when
            // there is no configured or default network.
            Network defaultOrConfiguredNetwork = mCallerConfiguredNetwork;
            if (mConnectivityManager != null && defaultOrConfiguredNetwork == null) {
                defaultOrConfiguredNetwork = mConnectivityManager.getActiveNetwork();
                if (defaultOrConfiguredNetwork == null) {
                    throw new IllegalArgumentException("Network not found");
                }
            }

            if (mServerHostname == null
                    || mLocalIdentification == null
                    || mRemoteIdentification == null
                    || mLocalAuthConfig == null
                    || mRemoteAuthConfig == null) {
                throw new IllegalArgumentException("Necessary parameter missing.");
            }

            if ((mIkeOptions & getOptionBitValue(IKE_OPTION_EAP_ONLY_AUTH)) != 0) {
                if (!(mLocalAuthConfig instanceof IkeAuthEapConfig)) {
                    throw new IllegalArgumentException(
                            "If IKE_OPTION_EAP_ONLY_AUTH is set,"
                                    + " eap authentication needs to be configured.");
                }

                IkeAuthEapConfig ikeAuthEapConfig = (IkeAuthEapConfig) mLocalAuthConfig;
                if (!ikeAuthEapConfig.getEapConfig().areAllMethodsEapOnlySafe()) {
                    throw new IllegalArgumentException(
                            "Only EAP-only safe method allowed" + " when using EAP-only option.");
                }
            }

            if (mLocalAuthConfig.mAuthMethod == IKE_AUTH_METHOD_PUB_KEY_SIGNATURE
                    && mLocalIdentification.idType == IkeIdentification.ID_TYPE_KEY_ID) {
                throw new IllegalArgumentException(
                        "It is not allowed to use KEY_ID as local ID when local authentication"
                                + " method is digital-signature-based");
            }

            return new IkeSessionParams(
                    mServerHostname,
                    defaultOrConfiguredNetwork,
                    mCallerConfiguredNetwork,
                    mSaProposalList.toArray(new IkeSaProposal[0]),
                    mLocalIdentification,
                    mRemoteIdentification,
                    mLocalAuthConfig,
                    mRemoteAuthConfig,
                    mConfigRequestList.toArray(new IkeConfigAttribute[0]),
                    mRetransTimeoutMsList,
                    mIke3gppExtension,
                    mIkeOptions,
                    mHardLifetimeSec,
                    mSoftLifetimeSec,
                    mDpdDelaySec,
                    mNattKeepaliveDelaySec,
                    mDscp,
                    mIsIkeFragmentationSupported);
        }

        // TODO: add methods for supporting IKE fragmentation.
    }
}
