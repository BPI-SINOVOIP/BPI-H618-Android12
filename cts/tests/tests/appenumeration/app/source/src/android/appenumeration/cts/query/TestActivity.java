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

package android.appenumeration.cts.query;

import static android.appenumeration.cts.Constants.ACTION_CHECK_SIGNATURES;
import static android.appenumeration.cts.Constants.ACTION_GET_INSTALLED_PACKAGES;
import static android.appenumeration.cts.Constants.ACTION_GET_NAMES_FOR_UIDS;
import static android.appenumeration.cts.Constants.ACTION_GET_NAME_FOR_UID;
import static android.appenumeration.cts.Constants.ACTION_GET_PACKAGES_FOR_UID;
import static android.appenumeration.cts.Constants.ACTION_GET_PACKAGE_INFO;
import static android.appenumeration.cts.Constants.ACTION_HAS_SIGNING_CERTIFICATE;
import static android.appenumeration.cts.Constants.ACTION_JUST_FINISH;
import static android.appenumeration.cts.Constants.ACTION_QUERY_ACTIVITIES;
import static android.appenumeration.cts.Constants.ACTION_QUERY_PROVIDERS;
import static android.appenumeration.cts.Constants.ACTION_QUERY_SERVICES;
import static android.appenumeration.cts.Constants.ACTION_SEND_RESULT;
import static android.appenumeration.cts.Constants.ACTION_START_DIRECTLY;
import static android.appenumeration.cts.Constants.ACTION_START_FOR_RESULT;
import static android.appenumeration.cts.Constants.ACTION_START_SENDER_FOR_RESULT;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_INVALID;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGES_AVAILABLE;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGES_SUSPENDED;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGES_UNAVAILABLE;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGES_UNSUSPENDED;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGE_ADDED;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGE_CHANGED;
import static android.appenumeration.cts.Constants.CALLBACK_EVENT_PACKAGE_REMOVED;
import static android.appenumeration.cts.Constants.EXTRA_AUTHORITY;
import static android.appenumeration.cts.Constants.EXTRA_CERT;
import static android.appenumeration.cts.Constants.EXTRA_DATA;
import static android.appenumeration.cts.Constants.EXTRA_ERROR;
import static android.appenumeration.cts.Constants.EXTRA_FLAGS;
import static android.appenumeration.cts.Constants.EXTRA_REMOTE_CALLBACK;
import static android.appenumeration.cts.Constants.EXTRA_REMOTE_READY_CALLBACK;
import static android.content.Intent.EXTRA_COMPONENT_NAME;
import static android.content.Intent.EXTRA_PACKAGES;
import static android.content.Intent.EXTRA_RETURN_RESULT;
import static android.content.pm.PackageManager.CERT_INPUT_RAW_X509;
import static android.os.Process.INVALID_UID;

import android.app.Activity;
import android.app.PendingIntent;
import android.appenumeration.cts.Constants;
import android.appenumeration.cts.MissingBroadcastException;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProviderInfo;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.ServiceConnection;
import android.content.SyncAdapterType;
import android.content.pm.LauncherApps;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.SharedLibraryInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Parcelable;
import android.os.PatternMatcher;
import android.os.Process;
import android.os.RemoteCallback;
import android.os.UserHandle;
import android.util.SparseArray;
import android.view.accessibility.AccessibilityManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

public class TestActivity extends Activity {

    private final static long TIMEOUT_MS = 3000;

    /**
     * Extending the timeout time of non broadcast receivers, avoid not
     * receiving callbacks in time on some common low-end platforms and
     * do not affect the situation that callback can be received in advance.
     */
    private final static long EXTENDED_TIMEOUT_MS = 5000;

    SparseArray<RemoteCallback> callbacks = new SparseArray<>();

    private Handler mainHandler;
    private Handler backgroundHandler;
    private HandlerThread backgroundThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mainHandler = new Handler(getMainLooper());
        backgroundThread = new HandlerThread("testBackground");
        backgroundThread.start();
        backgroundHandler = new Handler(backgroundThread.getLooper());
        super.onCreate(savedInstanceState);
        handleIntent(getIntent());
        onCommandReady(getIntent());
    }

    @Override
    protected void onDestroy() {
        backgroundThread.quitSafely();
        super.onDestroy();
    }

    private void handleIntent(Intent intent) {
        RemoteCallback remoteCallback = intent.getParcelableExtra(EXTRA_REMOTE_CALLBACK);
        try {
            final String action = intent.getAction();
            final Intent queryIntent = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            if (ACTION_GET_PACKAGE_INFO.equals(action)) {
                final String packageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                sendPackageInfo(remoteCallback, packageName);
            } else if (ACTION_GET_PACKAGES_FOR_UID.equals(action)) {
                final int uid = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                sendPackagesForUid(remoteCallback, uid);
            } else if (ACTION_GET_NAME_FOR_UID.equals(action)) {
                final int uid = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                sendNameForUid(remoteCallback, uid);
            } else if (ACTION_GET_NAMES_FOR_UIDS.equals(action)) {
                final int uid = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                sendNamesForUids(remoteCallback, uid);
            } else if (ACTION_CHECK_SIGNATURES.equals(action)) {
                final int uid1 = getPackageManager().getApplicationInfo(
                        getPackageName(), /* flags */ 0).uid;
                final int uid2 = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                sendCheckSignatures(remoteCallback, uid1, uid2);
            } else if (ACTION_HAS_SIGNING_CERTIFICATE.equals(action)) {
                final int uid = intent.getIntExtra(Intent.EXTRA_UID, INVALID_UID);
                final byte[] cert = intent.getBundleExtra(EXTRA_DATA).getByteArray(EXTRA_CERT);
                sendHasSigningCertificate(remoteCallback, uid, cert, CERT_INPUT_RAW_X509);
            } else if (ACTION_START_FOR_RESULT.equals(action)) {
                final String packageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                int requestCode = RESULT_FIRST_USER + callbacks.size();
                callbacks.put(requestCode, remoteCallback);
                startActivityForResult(
                        new Intent(ACTION_SEND_RESULT).setComponent(
                                new ComponentName(packageName, getClass().getCanonicalName())),
                        requestCode);
                // don't send anything... await result callback
            } else if (ACTION_SEND_RESULT.equals(action)) {
                try {
                    setResult(RESULT_OK,
                            getIntent().putExtra(
                                    Intent.EXTRA_RETURN_RESULT,
                                    getPackageManager().getPackageInfo(getCallingPackage(), 0)));
                } catch (PackageManager.NameNotFoundException e) {
                    setResult(RESULT_FIRST_USER, new Intent().putExtra("error", e));
                }
                finish();
            } else if (ACTION_QUERY_ACTIVITIES.equals(action)) {
                sendQueryIntentActivities(remoteCallback, queryIntent);
            } else if (ACTION_QUERY_SERVICES.equals(action)) {
                sendQueryIntentServices(remoteCallback, queryIntent);
            } else if (ACTION_QUERY_PROVIDERS.equals(action)) {
                sendQueryIntentProviders(remoteCallback, queryIntent);
            } else if (ACTION_START_DIRECTLY.equals(action)) {
                try {
                    startActivity(queryIntent);
                    remoteCallback.sendResult(new Bundle());
                } catch (ActivityNotFoundException e) {
                    sendError(remoteCallback, e);
                }
                finish();
            } else if (ACTION_JUST_FINISH.equals(action)) {
                finish();
            } else if (ACTION_GET_INSTALLED_PACKAGES.equals(action)) {
                sendGetInstalledPackages(remoteCallback, queryIntent.getIntExtra(EXTRA_FLAGS, 0));
            } else if (ACTION_START_SENDER_FOR_RESULT.equals(action)) {
                PendingIntent pendingIntent = intent.getParcelableExtra("pendingIntent");
                int requestCode = RESULT_FIRST_USER + callbacks.size();
                callbacks.put(requestCode, remoteCallback);
                try {
                    startIntentSenderForResult(pendingIntent.getIntentSender(), requestCode, null,
                            0, 0, 0);
                } catch (IntentSender.SendIntentException e) {
                    sendError(remoteCallback, e);
                }
            } else if (Constants.ACTION_AWAIT_PACKAGE_REMOVED.equals(action)) {
                final String packageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                awaitPackageBroadcast(
                        remoteCallback, packageName, Intent.ACTION_PACKAGE_REMOVED, TIMEOUT_MS);
            } else if (Constants.ACTION_AWAIT_PACKAGE_ADDED.equals(action)) {
                final String packageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                awaitPackageBroadcast(
                        remoteCallback, packageName, Intent.ACTION_PACKAGE_ADDED, TIMEOUT_MS);
            } else if (Constants.ACTION_QUERY_RESOLVER.equals(action)) {
                final String authority = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                queryResolverForVisiblePackages(remoteCallback, authority);
            } else if (Constants.ACTION_BIND_SERVICE.equals(action)) {
                final String packageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                bindService(remoteCallback, packageName);
            } else if (Constants.ACTION_GET_SYNCADAPTER_TYPES.equals(action)) {
                sendSyncAdapterTypes(remoteCallback);
            } else if (Constants.ACTION_GET_INSTALLED_APPWIDGET_PROVIDERS.equals(action)) {
                sendInstalledAppWidgetProviders(remoteCallback);
            } else if (Constants.ACTION_AWAIT_PACKAGES_SUSPENDED.equals(action)) {
                final String[] awaitPackages = intent.getBundleExtra(EXTRA_DATA)
                        .getStringArray(EXTRA_PACKAGES);
                awaitSuspendedPackagesBroadcast(remoteCallback, Arrays.asList(awaitPackages),
                        Intent.ACTION_PACKAGES_SUSPENDED, TIMEOUT_MS);
            } else if (Constants.ACTION_LAUNCHER_APPS_IS_ACTIVITY_ENABLED.equals(action)) {
                final String componentName = intent.getBundleExtra(EXTRA_DATA)
                        .getString(EXTRA_COMPONENT_NAME);
                sendIsActivityEnabled(remoteCallback, ComponentName.unflattenFromString(
                        componentName));
            } else if (Constants.ACTION_GET_SYNCADAPTER_PACKAGES_FOR_AUTHORITY.equals(action)) {
                final String authority = intent.getBundleExtra(EXTRA_DATA)
                        .getString(EXTRA_AUTHORITY);
                final int userId = intent.getBundleExtra(EXTRA_DATA)
                        .getInt(Intent.EXTRA_USER);
                sendSyncAdapterPackagesForAuthorityAsUser(remoteCallback, authority, userId);
            } else if (Constants.ACTION_AWAIT_LAUNCHER_APPS_CALLBACK.equals(action)) {
                final int expectedEventCode = intent.getBundleExtra(EXTRA_DATA)
                        .getInt(EXTRA_FLAGS, CALLBACK_EVENT_INVALID);
                awaitLauncherAppsCallback(remoteCallback, expectedEventCode, EXTENDED_TIMEOUT_MS);
            } else if (Constants.ACTION_GET_SHAREDLIBRARY_DEPENDENT_PACKAGES.equals(action)) {
                final String sharedLibName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                sendGetSharedLibraryDependentPackages(remoteCallback, sharedLibName);
            } else if (Constants.ACTION_GET_PREFERRED_ACTIVITIES.equals(action)) {
                sendGetPreferredActivities(remoteCallback);
            } else if (Constants.ACTION_SET_INSTALLER_PACKAGE_NAME.equals(action)) {
                final String targetPackageName = intent.getStringExtra(Intent.EXTRA_PACKAGE_NAME);
                final String installerPackageName = intent.getBundleExtra(EXTRA_DATA)
                        .getString(Intent.EXTRA_INSTALLER_PACKAGE_NAME);
                sendSetInstallerPackageName(remoteCallback, targetPackageName,
                        installerPackageName);
            } else if (Constants.ACTION_GET_INSTALLED_ACCESSIBILITYSERVICES_PACKAGES.equals(
                    action)) {
                sendGetInstalledAccessibilityServicePackages(remoteCallback);
            } else {
                sendError(remoteCallback, new Exception("unknown action " + action));
            }
        } catch (Exception e) {
            sendError(remoteCallback, e);
        }
    }

    private void sendGetInstalledAccessibilityServicePackages(RemoteCallback remoteCallback) {
        final String[] packages = getSystemService(
                AccessibilityManager.class).getInstalledAccessibilityServiceList().stream().map(
                p -> p.getComponentName().getPackageName()).distinct().toArray(String[]::new);
        final Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, packages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void onCommandReady(Intent intent) {
        final RemoteCallback callback = intent.getParcelableExtra(EXTRA_REMOTE_READY_CALLBACK);
        if (callback != null) {
            callback.sendResult(null);
        }
    }

    private void awaitPackageBroadcast(RemoteCallback remoteCallback, String packageName,
            String action, long timeoutMs) {
        final IntentFilter filter = new IntentFilter(action);
        filter.addDataScheme("package");
        filter.addDataSchemeSpecificPart(packageName, PatternMatcher.PATTERN_LITERAL);
        final Object token = new Object();
        registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final Bundle result = new Bundle();
                result.putString(EXTRA_DATA, intent.getDataString());
                remoteCallback.sendResult(result);
                mainHandler.removeCallbacksAndMessages(token);
                finish();
            }
        }, filter);
        mainHandler.postDelayed(
                () -> sendError(remoteCallback,
                        new MissingBroadcastException(action, timeoutMs)),
                token, timeoutMs);
    }

    private void awaitSuspendedPackagesBroadcast(RemoteCallback remoteCallback,
            List<String> awaitList, String action, long timeoutMs) {
        final IntentFilter filter = new IntentFilter(action);
        final ArrayList<String> suspendedList = new ArrayList<>();
        final Object token = new Object();
        final Runnable sendResult = () -> {
            final Bundle result = new Bundle();
            result.putStringArray(EXTRA_PACKAGES, suspendedList.toArray(new String[] {}));
            remoteCallback.sendResult(result);
            finish();
        };
        registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final Bundle extras = intent.getExtras();
                final String[] changedList = extras.getStringArray(
                        Intent.EXTRA_CHANGED_PACKAGE_LIST);
                suspendedList.addAll(Arrays.stream(changedList).filter(
                        p -> awaitList.contains(p)).collect(Collectors.toList()));
                if (suspendedList.size() == awaitList.size()) {
                    mainHandler.removeCallbacksAndMessages(token);
                    sendResult.run();
                }
            }
        }, filter);
        mainHandler.postDelayed(() -> sendResult.run(), token, timeoutMs);
    }

    private void awaitLauncherAppsCallback(RemoteCallback remoteCallback, int expectedEventCode,
            long timeoutMs) {
        final Object token = new Object();
        final Bundle result = new Bundle();
        final LauncherApps launcherApps = getSystemService(LauncherApps.class);
        final LauncherApps.Callback launcherAppsCallback = new LauncherApps.Callback() {

            private void onPackageStateUpdated(String[] packageNames, int resultCode) {
                if (resultCode != expectedEventCode) {
                    return;
                }

                mainHandler.removeCallbacksAndMessages(token);
                result.putStringArray(EXTRA_PACKAGES, packageNames);
                result.putInt(EXTRA_FLAGS, resultCode);
                remoteCallback.sendResult(result);

                launcherApps.unregisterCallback(this);
                finish();
            }

            @Override
            public void onPackageRemoved(String packageName, UserHandle user) {
                onPackageStateUpdated(new String[]{packageName}, CALLBACK_EVENT_PACKAGE_REMOVED);
            }

            @Override
            public void onPackageAdded(String packageName, UserHandle user) {
                onPackageStateUpdated(new String[]{packageName}, CALLBACK_EVENT_PACKAGE_ADDED);
            }

            @Override
            public void onPackageChanged(String packageName, UserHandle user) {
                onPackageStateUpdated(new String[]{packageName}, CALLBACK_EVENT_PACKAGE_CHANGED);
            }

            @Override
            public void onPackagesAvailable(String[] packageNames, UserHandle user,
                    boolean replacing) {
                onPackageStateUpdated(packageNames, CALLBACK_EVENT_PACKAGES_AVAILABLE);
            }

            @Override
            public void onPackagesUnavailable(String[] packageNames, UserHandle user,
                    boolean replacing) {
                onPackageStateUpdated(packageNames, CALLBACK_EVENT_PACKAGES_UNAVAILABLE);
            }

            @Override
            public void onPackagesSuspended(String[] packageNames, UserHandle user) {
                onPackageStateUpdated(packageNames, CALLBACK_EVENT_PACKAGES_SUSPENDED);
                super.onPackagesSuspended(packageNames, user);
            }

            @Override
            public void onPackagesUnsuspended(String[] packageNames, UserHandle user) {
                onPackageStateUpdated(packageNames, CALLBACK_EVENT_PACKAGES_UNSUSPENDED);
                super.onPackagesUnsuspended(packageNames, user);
            }
        };

        launcherApps.registerCallback(launcherAppsCallback);

        mainHandler.postDelayed(() -> {
            result.putStringArray(EXTRA_PACKAGES, new String[]{});
            result.putInt(EXTRA_FLAGS, CALLBACK_EVENT_INVALID);
            remoteCallback.sendResult(result);

            launcherApps.unregisterCallback(launcherAppsCallback);
            finish();
        }, token, timeoutMs);
    }

    private void sendGetInstalledPackages(RemoteCallback remoteCallback, int flags) {
        String[] packages =
                getPackageManager().getInstalledPackages(flags)
                        .stream().map(p -> p.packageName).distinct().toArray(String[]::new);
        Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, packages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendQueryIntentActivities(RemoteCallback remoteCallback, Intent queryIntent) {
        final String[] resolveInfos = getPackageManager().queryIntentActivities(
                queryIntent, 0 /* flags */).stream()
                .map(ri -> ri.activityInfo.applicationInfo.packageName)
                .distinct()
                .toArray(String[]::new);
        Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, resolveInfos);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendQueryIntentServices(RemoteCallback remoteCallback, Intent queryIntent) {
        final String[] resolveInfos = getPackageManager().queryIntentServices(
                queryIntent, 0 /* flags */).stream()
                .map(ri -> ri.serviceInfo.applicationInfo.packageName)
                .distinct()
                .toArray(String[]::new);
        Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, resolveInfos);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendQueryIntentProviders(RemoteCallback remoteCallback, Intent queryIntent) {
        final String[] resolveInfos = getPackageManager().queryIntentContentProviders(
                queryIntent, 0 /* flags */).stream()
                .map(ri -> ri.providerInfo.applicationInfo.packageName)
                .distinct()
                .toArray(String[]::new);
        Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, resolveInfos);
        remoteCallback.sendResult(result);
        finish();
    }

    private void queryResolverForVisiblePackages(RemoteCallback remoteCallback, String authority) {
        backgroundHandler.post(() -> {
            Uri queryUri = Uri.parse("content://" + authority + "/test");
            Cursor query = getContentResolver().query(queryUri, null, null, null, null);
            if (query == null || !query.moveToFirst()) {
                sendError(remoteCallback,
                        new IllegalStateException(
                                "Query of " + queryUri + " could not be completed"));
                return;
            }
            ArrayList<String> visiblePackages = new ArrayList<>();
            while (!query.isAfterLast()) {
                visiblePackages.add(query.getString(0));
                query.moveToNext();
            }
            query.close();

            mainHandler.post(() -> {
                Bundle result = new Bundle();
                result.putStringArray(EXTRA_RETURN_RESULT, visiblePackages.toArray(new String[]{}));
                remoteCallback.sendResult(result);
                finish();
            });

        });
    }

    private void sendError(RemoteCallback remoteCallback, Exception failure) {
        Bundle result = new Bundle();
        result.putSerializable(EXTRA_ERROR, failure);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendPackageInfo(RemoteCallback remoteCallback, String packageName) {
        final PackageInfo pi;
        try {
            pi = getPackageManager().getPackageInfo(packageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            sendError(remoteCallback, e);
            return;
        }
        Bundle result = new Bundle();
        result.putParcelable(EXTRA_RETURN_RESULT, pi);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendPackagesForUid(RemoteCallback remoteCallback, int uid) {
        final String[] packages = getPackageManager().getPackagesForUid(uid);
        final Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, packages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendNameForUid(RemoteCallback remoteCallback, int uid) {
        final String name = getPackageManager().getNameForUid(uid);
        final Bundle result = new Bundle();
        result.putString(EXTRA_RETURN_RESULT, name);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendNamesForUids(RemoteCallback remoteCallback, int uid) {
        final String[] names = getPackageManager().getNamesForUids(new int[]{uid});
        final Bundle result = new Bundle();
        result.putStringArray(EXTRA_RETURN_RESULT, names);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendCheckSignatures(RemoteCallback remoteCallback, int uid1, int uid2) {
        final int signatureResult = getPackageManager().checkSignatures(uid1, uid2);
        final Bundle result = new Bundle();
        result.putInt(EXTRA_RETURN_RESULT, signatureResult);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendHasSigningCertificate(RemoteCallback remoteCallback, int uid, byte[] cert,
            int type) {
        final boolean signatureResult = getPackageManager().hasSigningCertificate(uid, cert, type);
        final Bundle result = new Bundle();
        result.putBoolean(EXTRA_RETURN_RESULT, signatureResult);
        remoteCallback.sendResult(result);
        finish();
    }

    /**
     * Instead of sending a list of package names, this function sends a List of
     * {@link SyncAdapterType}, since the {@link SyncAdapterType#getPackageName()} is a test api
     * which can only be invoked in the instrumentation.
     */
    private void sendSyncAdapterTypes(RemoteCallback remoteCallback) {
        final SyncAdapterType[] types = ContentResolver.getSyncAdapterTypes();
        final ArrayList<Parcelable> parcelables = new ArrayList<>();
        for (SyncAdapterType type : types) {
            parcelables.add(type);
        }
        final Bundle result = new Bundle();
        result.putParcelableArrayList(EXTRA_RETURN_RESULT, parcelables);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendIsActivityEnabled(RemoteCallback remoteCallback, ComponentName componentName) {
        final LauncherApps launcherApps = getSystemService(LauncherApps.class);
        final Bundle result = new Bundle();
        try {
            result.putBoolean(EXTRA_RETURN_RESULT, launcherApps.isActivityEnabled(componentName,
                    Process.myUserHandle()));
        } catch (IllegalArgumentException e) {
        }
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendInstalledAppWidgetProviders(RemoteCallback remoteCallback) {
        final AppWidgetManager appWidgetManager = getSystemService(AppWidgetManager.class);
        final List<AppWidgetProviderInfo> providers = appWidgetManager.getInstalledProviders();
        final ArrayList<Parcelable> parcelables = new ArrayList<>();
        for (AppWidgetProviderInfo info : providers) {
            parcelables.add(info);
        }
        final Bundle result = new Bundle();
        result.putParcelableArrayList(EXTRA_RETURN_RESULT, parcelables);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendSyncAdapterPackagesForAuthorityAsUser(RemoteCallback remoteCallback,
            String authority, int userId) {
        final String[] syncAdapterPackages = ContentResolver
                .getSyncAdapterPackagesForAuthorityAsUser(authority, userId);
        final Bundle result = new Bundle();
        result.putStringArray(Intent.EXTRA_PACKAGES, syncAdapterPackages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendGetSharedLibraryDependentPackages(RemoteCallback remoteCallback,
            String sharedLibName) {
        final List<SharedLibraryInfo> sharedLibraryInfos = getPackageManager()
                .getSharedLibraries(0 /* flags */);
        SharedLibraryInfo sharedLibraryInfo = sharedLibraryInfos.stream().filter(
                info -> sharedLibName.equals(info.getName())).findAny().orElse(null);
        final String[] dependentPackages = sharedLibraryInfo == null ? null
                : sharedLibraryInfo.getDependentPackages().stream()
                        .map(versionedPackage -> versionedPackage.getPackageName())
                        .distinct().collect(Collectors.toList()).toArray(new String[]{});
        final Bundle result = new Bundle();
        result.putStringArray(Intent.EXTRA_PACKAGES, dependentPackages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendGetPreferredActivities(RemoteCallback remoteCallback) {
        final List<IntentFilter> filters = new ArrayList<>();
        final List<ComponentName> activities = new ArrayList<>();
        getPackageManager().getPreferredActivities(filters, activities, null /* packageName*/);
        final String[] packages = activities.stream()
                .map(componentName -> componentName.getPackageName()).distinct()
                .collect(Collectors.toList()).toArray(new String[]{});
        final Bundle result = new Bundle();
        result.putStringArray(Intent.EXTRA_PACKAGES, packages);
        remoteCallback.sendResult(result);
        finish();
    }

    private void sendSetInstallerPackageName(RemoteCallback remoteCallback,
            String targetPackageName, String installerPackageName) {
        try {
            getPackageManager().setInstallerPackageName(targetPackageName, installerPackageName);
            remoteCallback.sendResult(null);
            finish();
        } catch (Exception e) {
            sendError(remoteCallback, e);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        final RemoteCallback remoteCallback = callbacks.get(requestCode);
        if (resultCode != RESULT_OK) {
            Exception e = (Exception) data.getSerializableExtra(EXTRA_ERROR);
            sendError(remoteCallback, e == null ? new Exception("Result was " + resultCode) : e);
            return;
        }
        final Bundle result = new Bundle();
        result.putParcelable(EXTRA_RETURN_RESULT, data.getParcelableExtra(EXTRA_RETURN_RESULT));
        remoteCallback.sendResult(result);
        finish();
    }

    private void bindService(RemoteCallback remoteCallback, String packageName) {
        final String SERVICE_NAME = "android.appenumeration.testapp.DummyService";
        final Intent intent = new Intent();
        intent.setClassName(packageName, SERVICE_NAME);
        final ServiceConnection serviceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName className, IBinder service) {
                // No-op
            }

            @Override
            public void onServiceDisconnected(ComponentName className) {
                // No-op
            }

            @Override
            public void onBindingDied(ComponentName name) {
                // Remote service die
                finish();
            }

            @Override
            public void onNullBinding(ComponentName name) {
                // Since the DummyService doesn't implement onBind, it returns null and
                // onNullBinding would be called. Use postDelayed to keep this service
                // connection alive for 3 seconds.
                mainHandler.postDelayed(() -> {
                    unbindService(this);
                    finish();
                }, TIMEOUT_MS);
            }
        };

        final boolean bound = bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        final Bundle result = new Bundle();
        result.putBoolean(EXTRA_RETURN_RESULT, bound);
        remoteCallback.sendResult(result);
        // Don't invoke finish() right here if service is bound successfully to keep the service
        // connection alive since the ServiceRecord would be remove from the ServiceMap once no
        // client is binding the service.
        if (!bound) finish();
    }
}
