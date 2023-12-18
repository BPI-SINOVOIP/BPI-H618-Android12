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

package android.devicepolicy.cts;

import static android.app.admin.DevicePolicyManager.INSTALLKEY_SET_USER_SELECTABLE;

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import static org.testng.Assert.assertThrows;

import android.app.AppOpsManager;
import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.net.Uri;
import android.os.Binder;
import android.os.Process;
import android.security.AppUriAuthenticationPolicy;
import android.security.AttestedKeyPair;
import android.security.KeyChain;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.activitycontext.ActivityContext;
import com.android.bedstead.harrier.BedsteadJUnit4;
import com.android.bedstead.harrier.annotations.Postsubmit;
import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.permissions.PermissionContext;
import com.android.compatibility.common.util.BlockingCallback;
import com.android.compatibility.common.util.FakeKeys;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayInputStream;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.Arrays;
import java.util.List;

@RunWith(BedsteadJUnit4.class)
public class CredentialManagementAppTest {

    private static final PrivateKey PRIVATE_KEY =
            getPrivateKey(FakeKeys.FAKE_RSA_1.privateKey, "RSA");
    private static final Certificate CERTIFICATE =
            getCertificate(FakeKeys.FAKE_RSA_1.caCertificate);
    private static final Certificate[] CERTIFICATES = new Certificate[]{CERTIFICATE};
    private static final TestApis sTestApis = new TestApis();

    private static final Context CONTEXT = ApplicationProvider.getApplicationContext();
    private static final String MANAGE_CREDENTIALS = "android:manage_credentials";

    private static final String ALIAS = "com.android.test.rsa";
    private static final String NOT_IN_USER_POLICY_ALIAS = "anotherAlias";
    private final static String PACKAGE_NAME = CONTEXT.getPackageName();
    private final static Uri URI = Uri.parse("https://test.com");
    private final static AppUriAuthenticationPolicy AUTHENTICATION_POLICY =
            new AppUriAuthenticationPolicy.Builder()
                    .addAppAndUriMapping(PACKAGE_NAME, URI, ALIAS)
                    .build();

    private final static String MANAGE_CREDENTIAL_MANAGEMENT_APP_PERMISSION =
            "android.permission.MANAGE_CREDENTIAL_MANAGEMENT_APP";

    private final DevicePolicyManager mDpm = CONTEXT.getSystemService(DevicePolicyManager.class);
    private final int mUserId = Process.myUserHandle().getIdentifier();

    @Postsubmit(reason="new")
    @Test
    public void installKeyPair_withoutManageCredentialAppOp_throwsException() throws Exception {
        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ false, mUserId);
        assertThrows(SecurityException.class,
                () -> mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES,
                        ALIAS, /* flags = */ 0));
    }

    @Postsubmit(reason="new")
    @Test
    public void removeKeyPair_withoutManageCredentialAppOp_throwsException() throws Exception {
        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ false, mUserId);
        assertThrows(SecurityException.class,
                () -> mDpm.removeKeyPair(/* admin = */ null, ALIAS));
    }

    @Postsubmit(reason="new")
    @Test
    public void generateKeyPair_withoutManageCredentialAppOp_throwsException() throws Exception {
        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ false, mUserId);
        assertThrows(SecurityException.class,
                () -> mDpm.generateKeyPair(/* admin = */ null, "RSA",
                        buildRsaKeySpec(ALIAS, /* useStrongBox = */ false),
                        /* idAttestationFlags = */ 0));
    }

    @Postsubmit(reason="new")
    @Test
    public void setKeyPairCertificate_withoutManageCredentialAppOp_throwsException()
            throws Exception {
        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ false, mUserId);
        assertThrows(SecurityException.class,
                () -> mDpm.setKeyPairCertificate(/* admin = */ null, ALIAS,
                        Arrays.asList(CERTIFICATE), /* isUserSelectable = */ false));
    }

    @Postsubmit(reason="new")
    @Test
    public void installKeyPair_isUserSelectableFlagSet_throwsException() throws Exception {
        setCredentialManagementApp();
        assertThrows(SecurityException.class,
                () -> mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES,
                        ALIAS, /* flags = */ INSTALLKEY_SET_USER_SELECTABLE));
    }

    @Postsubmit(reason="new")
    @Test
    public void installKeyPair_aliasIsNotInAuthenticationPolicy_throwsException() throws Exception {
        setCredentialManagementApp();
        assertThrows(SecurityException.class,
                () -> mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES,
                        NOT_IN_USER_POLICY_ALIAS, /* flags = */ 0));
    }

    @Postsubmit(reason="new")
    @Test
    public void installKeyPair_isCredentialManagementApp_success() throws Exception {
        setCredentialManagementApp();
        try {
            // Install keypair as credential management app
            assertThat(mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES,
                    ALIAS, 0)).isTrue();
        } finally {
            // Remove keypair as credential management app
            mDpm.removeKeyPair(/* admin = */ null, ALIAS);
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void hasKeyPair_aliasIsNotInAuthenticationPolicy_throwsException() throws Exception {
        setCredentialManagementApp();

        try {
            assertThrows(SecurityException.class, () -> mDpm.hasKeyPair(NOT_IN_USER_POLICY_ALIAS));
        } finally {
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void hasKeyPair_isCredentialManagementApp_success() throws Exception {
        setCredentialManagementApp();
        try {
            mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES, ALIAS,
                    /* flags = */0);

            assertThat(mDpm.hasKeyPair(ALIAS)).isTrue();
        } finally {
            mDpm.removeKeyPair(/* admin = */ null, ALIAS);
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void removeKeyPair_isCredentialManagementApp_success() throws Exception {
        setCredentialManagementApp();
        try {
            // Install keypair as credential management app
            mDpm.installKeyPair(/* admin = */ null, PRIVATE_KEY, CERTIFICATES, ALIAS, 0);
        } finally {
            // Remove keypair as credential management app
            assertThat(mDpm.removeKeyPair(/* admin = */ null, ALIAS)).isTrue();
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void generateKeyPair_isCredentialManagementApp_success() throws Exception {
        setCredentialManagementApp();
        try {
            // Generate keypair as credential management app
            AttestedKeyPair generated = mDpm.generateKeyPair(/* admin = */ null, "RSA",
                    buildRsaKeySpec(ALIAS, /* useStrongBox = */ false),
                    /* idAttestationFlags = */ 0);

            assertThat(generated).isNotNull();
            verifySignatureOverData("SHA256withRSA", generated.getKeyPair());
        } finally {
            // Remove keypair as credential management app
            mDpm.removeKeyPair(/* admin = */ null, ALIAS);
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void setKeyPairCertificate_isCredentialManagementApp_success() throws Exception {
        setCredentialManagementApp();
        try {
            // Generate keypair and aet keypair certificate as credential management app
            KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(ALIAS,
                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY).setDigests(
                    KeyProperties.DIGEST_SHA256).build();
            AttestedKeyPair generated = mDpm.generateKeyPair(/* admin = */ null, "EC", spec, 0);
            List<Certificate> certificates = Arrays.asList(CERTIFICATE);
            mDpm.setKeyPairCertificate(/* admin = */ null, ALIAS, certificates, false);

            // Make sure certificates can be retrieved from KeyChain
            Certificate[] fetchedCerts = KeyChain.getCertificateChain(CONTEXT, ALIAS);

            assertThat(generated).isNotNull();
            assertThat(fetchedCerts).isNotNull();
            assertThat(fetchedCerts.length).isEqualTo(certificates.size());
            assertThat(fetchedCerts[0].getEncoded()).isEqualTo(certificates.get(0).getEncoded());
        } finally {
            // Remove keypair as credential management app
            mDpm.removeKeyPair(/* admin = */ null, ALIAS);
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="b/181207615 flaky")
    @Test
    public void choosePrivateKeyAlias_isCredentialManagementApp_aliasSelected() throws Exception {
        setCredentialManagementApp();
        try {
            // Install keypair as credential management app
            mDpm.installKeyPair(null, PRIVATE_KEY, new Certificate[]{CERTIFICATE}, ALIAS, 0);
            KeyChainAliasCallback callback = new KeyChainAliasCallback();

            ActivityContext.runWithContext((activity) ->
                    KeyChain.choosePrivateKeyAlias(activity, callback,
                            /* keyTypes= */ null, /* issuers= */ null, URI, /* alias = */ null)
            );

            assertThat(callback.await()).isEqualTo(ALIAS);
        } finally {
            // Remove keypair as credential management app
            mDpm.removeKeyPair(/* admin = */ null, ALIAS);
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void isCredentialManagementApp_isNotCredentialManagementApp_returnFalse()
            throws Exception {
        removeCredentialManagementApp();
        assertFalse(KeyChain.isCredentialManagementApp(CONTEXT));
    }

    @Postsubmit(reason="new")
    @Test
    public void isCredentialManagementApp_isCredentialManagementApp_returnTrue() throws Exception {
        setCredentialManagementApp();
        try {
            assertTrue(KeyChain.isCredentialManagementApp(CONTEXT));
        } finally {
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void getCredentialManagementAppPolicy_isNotCredentialManagementApp_throwException()
            throws Exception {
        removeCredentialManagementApp();
        assertThrows(SecurityException.class,
                () -> KeyChain.getCredentialManagementAppPolicy(CONTEXT));
    }

    @Postsubmit(reason="new")
    @Test
    public void getCredentialManagementAppPolicy_isCredentialManagementApp_returnPolicy()
            throws Exception {
        setCredentialManagementApp();
        try {
            assertThat(KeyChain.getCredentialManagementAppPolicy(CONTEXT))
                    .isEqualTo(AUTHENTICATION_POLICY);
        } finally {
            removeCredentialManagementApp();
        }
    }

    @Postsubmit(reason="new")
    @Test
    public void unregisterAsCredentialManagementApp_returnTrue()
            throws Exception {
        setCredentialManagementApp();

        try {
            assertTrue(KeyChain.removeCredentialManagementApp(CONTEXT));

            assertFalse(KeyChain.isCredentialManagementApp(CONTEXT));
        } catch (Exception e) {
            removeCredentialManagementApp();
        }
    }

    // TODO(scottjonathan): Using either code generation or reflection we could remove the need for
    //  these boilerplate classes
    private static class KeyChainAliasCallback extends BlockingCallback<String> implements
            android.security.KeyChainAliasCallback {
        @Override
        public void alias(final String chosenAlias) {
            callbackTriggered(chosenAlias);
        }
    }

    // TODO (b/174677062): Move this into infrastructure
    private void setCredentialManagementApp() throws Exception {
        try (PermissionContext p = sTestApis.permissions().withPermission(
                MANAGE_CREDENTIAL_MANAGEMENT_APP_PERMISSION)){
            assertTrue("Unable to set credential management app",
                    KeyChain.setCredentialManagementApp(CONTEXT, PACKAGE_NAME,
                            AUTHENTICATION_POLICY));
        }

        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ true, mUserId);
        assertTrue("CredentialManagementApp should have app op MANAGE_CREDENTIALS",
                isCredentialManagementApp());
    }

    // TODO (b/174677062): Move this into infrastructure
    private void removeCredentialManagementApp() throws Exception {
        try (PermissionContext p = sTestApis.permissions().withPermission(
                MANAGE_CREDENTIAL_MANAGEMENT_APP_PERMISSION)){
            assertTrue("Unable to remove credential management app",
                    KeyChain.removeCredentialManagementApp(CONTEXT));
        }
        setManageCredentialsAppOps(PACKAGE_NAME, /* allowed = */ false, mUserId);
    }

    private void setManageCredentialsAppOps(String packageName, boolean allowed, int userId)
            throws Exception {
        String command = "appops set --user " + userId + " " + packageName + " " +
                "MANAGE_CREDENTIALS " + (allowed ? "allow" : "default");
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), command);
    }

    void verifySignature(String algoIdentifier, PublicKey publicKey, byte[] signature)
            throws Exception {
        byte[] data = "hello".getBytes();
        Signature verify = Signature.getInstance(algoIdentifier);
        verify.initVerify(publicKey);
        verify.update(data);
        assertThat(verify.verify(signature)).isTrue();
    }

    private void verifySignatureOverData(String algoIdentifier, KeyPair keyPair) throws Exception {
        verifySignature(algoIdentifier, keyPair.getPublic(),
                signDataWithKey(algoIdentifier, keyPair.getPrivate()));
    }

    private byte[] signDataWithKey(String algoIdentifier, PrivateKey privateKey) throws Exception {
        byte[] data = "hello".getBytes();
        Signature sign = Signature.getInstance(algoIdentifier);
        sign.initSign(privateKey);
        sign.update(data);
        return sign.sign();
    }

    private static PrivateKey getPrivateKey(final byte[] key, String type) {
        try {
            return KeyFactory.getInstance(type).generatePrivate(
                    new PKCS8EncodedKeySpec(key));
        } catch (InvalidKeySpecException | NoSuchAlgorithmException e) {
            throw new AssertionError("Unable to get certificate." + e);
        }
    }

    private static Certificate getCertificate(byte[] cert) {
        try {
            return CertificateFactory.getInstance("X.509").generateCertificate(
                    new ByteArrayInputStream(cert));
        } catch (CertificateException e) {
            throw new AssertionError("Unable to get certificate." + e);
        }
    }

    private boolean isCredentialManagementApp() {
        AppOpsManager appOpsManager = CONTEXT.getSystemService(AppOpsManager.class);
        return appOpsManager.unsafeCheckOpNoThrow(MANAGE_CREDENTIALS,
                Binder.getCallingUid(), CONTEXT.getPackageName()) == AppOpsManager.MODE_ALLOWED;
    }

    private KeyGenParameterSpec buildRsaKeySpec(String alias, boolean useStrongBox) {
        return new KeyGenParameterSpec.Builder(
                alias,
                KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                .setKeySize(2048)
                .setDigests(KeyProperties.DIGEST_SHA256)
                .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PSS,
                        KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                .setIsStrongBoxBacked(useStrongBox)
                .build();
    }
}
