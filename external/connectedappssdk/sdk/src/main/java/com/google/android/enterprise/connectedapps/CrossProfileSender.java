/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.enterprise.connectedapps;

import static com.google.android.enterprise.connectedapps.CrossProfileSDKUtilities.filterUsersByAvailabilityRestrictions;
import static com.google.android.enterprise.connectedapps.CrossProfileSDKUtilities.selectUserHandleToBind;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.CrossProfileApps;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;
import com.google.android.enterprise.connectedapps.annotations.AvailabilityRestrictions;
import com.google.android.enterprise.connectedapps.exceptions.MissingApiException;
import com.google.android.enterprise.connectedapps.exceptions.ProfileRuntimeException;
import com.google.android.enterprise.connectedapps.exceptions.UnavailableProfileException;
import com.google.android.enterprise.connectedapps.internal.CrossProfileParcelCallSender;
import com.google.android.enterprise.connectedapps.internal.ParcelCallReceiver;
import com.google.android.enterprise.connectedapps.internal.ParcelUtilities;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.checkerframework.checker.nullness.qual.Nullable;

/** This class is used internally by the Connected Apps SDK to send messages cross-profile. */
public class CrossProfileSender {

  private static final class CrossProfileCall {
    private final long crossProfileTypeIdentifier;
    private final int methodIdentifier;
    private final Parcel params;
    private final LocalCallback callback;
    private final long timeoutMillis;

    CrossProfileCall(
        long crossProfileTypeIdentifier,
        int methodIdentifier,
        Parcel params,
        LocalCallback callback,
        long timeoutMillis) {
      if (params == null || callback == null) {
        throw new NullPointerException();
      }
      this.crossProfileTypeIdentifier = crossProfileTypeIdentifier;
      this.methodIdentifier = methodIdentifier;
      this.params = params;
      this.callback = callback;
      this.timeoutMillis = timeoutMillis;
    }

    void recycle() {
      params.recycle();
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if (o == null || getClass() != o.getClass()) {
        return false;
      }
      CrossProfileCall that = (CrossProfileCall) o;
      return crossProfileTypeIdentifier == that.crossProfileTypeIdentifier
          && methodIdentifier == that.methodIdentifier
          && params.equals(that.params)
          && callback.equals(that.callback)
          && timeoutMillis == that.timeoutMillis;
    }

    @Override
    public int hashCode() {
      return Objects.hash(
          crossProfileTypeIdentifier, methodIdentifier, params, callback, timeoutMillis);
    }
  }

  private static final class OngoingCrossProfileCall extends ICrossProfileCallback.Stub {

    private final CrossProfileSender sender;
    private final LocalCallback originalCallback;
    private final AtomicBoolean complete = new AtomicBoolean(false);
    private ScheduledFuture<?> timeoutFuture;
    private final long timeoutMillis;
    private final ParcelCallReceiver parcelCallReceiver = new ParcelCallReceiver();

    private OngoingCrossProfileCall(
        CrossProfileSender sender, LocalCallback originalCallback, long timeoutMillis) {
      if (sender == null || originalCallback == null) {
        throw new NullPointerException();
      }
      this.sender = sender;
      this.originalCallback = originalCallback;
      this.timeoutMillis = timeoutMillis;
    }

    void scheduleTimeout(ScheduledExecutorService timeoutExecutor) {
      if (this.timeoutFuture != null) {
        throw new IllegalStateException("Each call can only have a single timeout scheduled.");
      }
      if (complete.get()) {
        return;
      }
      this.timeoutFuture =
          timeoutExecutor.schedule(this::onTimeout, timeoutMillis, TimeUnit.MILLISECONDS);
    }

    private void onTimeout() {
      if (complete.get()) {
        return;
      }
      Parcel throwableParcel =
          createThrowableParcel(
              new UnavailableProfileException(
                  "The call timed out after " + timeoutMillis + " milliseconds"));

      onException(throwableParcel);
      throwableParcel.recycle();
    }

    @Override
    public void prepareResult(long callId, int blockId, int numBytes, byte[] params) {
      parcelCallReceiver.prepareCall(callId, blockId, numBytes, params);
    }

    @Override
    public void onResult(long callId, int blockId, int methodIdentifier, byte[] paramsBytes) {
      if (complete.getAndSet(true)) {
        return;
      }
      if (timeoutFuture != null) {
        timeoutFuture.cancel(/* mayInterruptIfRunning= */ true);
      }
      sender.ongoingCallComplete(this);

      Parcel parcel = parcelCallReceiver.getPreparedCall(callId, blockId, paramsBytes);

      originalCallback.onResult(methodIdentifier, parcel);
      parcel.recycle();

      sender.maybeScheduleAutomaticDisconnection();
    }

    @Override
    public void onException(long callId, int blockId, byte[] paramsBytes) {
      Parcel parcel = parcelCallReceiver.getPreparedCall(callId, blockId, paramsBytes);

      onException(parcel);

      parcel.recycle();
    }

    public void onException(Parcel exception) {
      if (complete.getAndSet(true)) {
        return;
      }
      if (timeoutFuture != null) {
        timeoutFuture.cancel(/* mayInterruptIfRunning= */ true);
      }
      sender.ongoingCallComplete(this);

      originalCallback.onException(exception);

      sender.maybeScheduleAutomaticDisconnection();
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if (o == null || getClass() != o.getClass()) {
        return false;
      }
      OngoingCrossProfileCall that = (OngoingCrossProfileCall) o;
      return sender.equals(that.sender)
          && originalCallback.equals(that.originalCallback)
          && complete.equals(that.complete);
    }

    @Override
    public int hashCode() {
      return Objects.hash(sender, originalCallback, complete);
    }
  }

  private void ongoingCallComplete(OngoingCrossProfileCall call) {
    ongoingCrossProfileCalls.removeFirstOccurrence(call);
  }

  public static final int MAX_BYTES_PER_BLOCK = 250000;

  private static final String LOG_TAG = "CrossProfileSender";
  private static final long INITIAL_BIND_RETRY_DELAY_MS = 500;
  private static final int DEFAULT_AUTOMATIC_DISCONNECTION_TIMEOUT_SECONDS = 30;

  private final ScheduledExecutorService scheduledExecutorService;
  private final Context context;
  private final ComponentName bindToService;
  private boolean canUseReflectedApis;
  private long bindRetryDelayMs = 500;
  private AtomicBoolean isBinding = new AtomicBoolean(false);
  private final AtomicReference<ICrossProfileService> iCrossProfileService =
      new AtomicReference<>();
  private final ConnectionListener connectionListener;
  private final AvailabilityListener availabilityListener;
  private final ConnectionBinder binder;
  @Nullable private volatile ScheduledFuture<Void> automaticDisconnectionFuture;
  private final AvailabilityRestrictions availabilityRestrictions;

  private boolean isManuallyManagingConnection = false;
  private ConcurrentLinkedDeque<OngoingCrossProfileCall> ongoingCrossProfileCalls =
      new ConcurrentLinkedDeque<>();
  private ConcurrentLinkedDeque<CrossProfileCall> asyncCallQueue = new ConcurrentLinkedDeque<>();

  private static final int NONE = 0;
  private static final int UNAVAILABLE = 1;
  private static final int AVAILABLE = 2;
  private static final int DISCONNECTED = UNAVAILABLE;
  private static final int CONNECTED = AVAILABLE;

  private ScheduledFuture<?> scheduledTryBind;

  private int lastReportedAvailabilityStatus = NONE;
  private int lastReportedConnectedStatus = NONE;

  CrossProfileSender(
      Context context,
      String connectedAppsServiceClassName,
      ConnectionBinder binder,
      ConnectionListener connectionListener,
      AvailabilityListener availabilityListener,
      ScheduledExecutorService scheduledExecutorService,
      AvailabilityRestrictions availabilityRestrictions) {
    this.context = context.getApplicationContext();
    if (connectionListener == null
        || availabilityListener == null
        || availabilityRestrictions == null
        || binder == null
        || scheduledExecutorService == null) {
      throw new NullPointerException();
    }
    this.binder = binder;
    this.connectionListener = connectionListener;
    this.availabilityListener = availabilityListener;
    bindToService = new ComponentName(context.getPackageName(), connectedAppsServiceClassName);
    canUseReflectedApis = ReflectionUtilities.canUseReflectedApis();
    this.scheduledExecutorService = scheduledExecutorService;
    this.availabilityRestrictions = availabilityRestrictions;
  }

  private final BroadcastReceiver profileAvailabilityReceiver =
      new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          checkAvailability();
        }
      };

  private final ServiceConnection connection =
      new ServiceConnection() {
        // Called when the connection with the service is established
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
          scheduledExecutorService.execute(
              () -> {
                if (!isBinding.get()) {
                  unbind();
                  return;
                }
                iCrossProfileService.set(ICrossProfileService.Stub.asInterface(service));

                tryMakeAsyncCalls();
                checkConnected();
                onBindingAttemptSucceeded();
              });
        }

        // Called when the connection with the service disconnects unexpectedly
        @Override
        public void onServiceDisconnected(ComponentName className) {
          scheduledExecutorService.execute(
              () -> {
                Log.e(LOG_TAG, "Unexpected disconnection");
                if (!asyncCallQueue.isEmpty() || !ongoingCrossProfileCalls.isEmpty()) {
                  Log.d(LOG_TAG, "Found in progress calls");
                  throwExceptionForAsyncCalls(
                      new UnavailableProfileException("Lost connection to other profile"));
                  // These disconnections can be temporary - so to avoid an exception on an async
                  // call leading to bad user experience - we send the availability update again
                  // to prompt a retry/refresh
                  updateAvailability();
                }
                iCrossProfileService.set(null);
                checkConnected();
                cancelAutomaticDisconnection();
                startTryBinding();
              });
        }
      };

  private final Object automaticDisconnectionFutureLock = new Object();

  private void cancelAutomaticDisconnection() {
    if (automaticDisconnectionFuture != null) {
      synchronized (automaticDisconnectionFutureLock) {
        if (automaticDisconnectionFuture != null) {
          automaticDisconnectionFuture.cancel(/* mayInterruptIfRunning= */ true);
          automaticDisconnectionFuture = null;
        }
      }
    }
  }

  private void maybeScheduleAutomaticDisconnection() {
    if (!isManuallyManagingConnection
        && asyncCallQueue.isEmpty()
        && ongoingCrossProfileCalls.isEmpty()
        && isBound()
        && automaticDisconnectionFuture == null) {
      synchronized (automaticDisconnectionFutureLock) {
        if (automaticDisconnectionFuture == null) {
          automaticDisconnectionFuture =
              scheduledExecutorService.schedule(
                  this::automaticallyDisconnect,
                  DEFAULT_AUTOMATIC_DISCONNECTION_TIMEOUT_SECONDS,
                  TimeUnit.SECONDS);
        }
      }
    }
  }

  private Void automaticallyDisconnect() {
    if (!isManuallyManagingConnection
        && asyncCallQueue.isEmpty()
        && ongoingCrossProfileCalls.isEmpty()
        && isBound()) {
      unbind();
    }
    return null;
  }

  void beginMonitoringAvailabilityChanges() {
    IntentFilter filter = new IntentFilter();
    filter.addAction(Intent.ACTION_MANAGED_PROFILE_UNLOCKED);
    filter.addAction(Intent.ACTION_MANAGED_PROFILE_AVAILABLE);
    filter.addAction(Intent.ACTION_MANAGED_PROFILE_UNAVAILABLE);
    context.registerReceiver(profileAvailabilityReceiver, filter);
  }

  private volatile CountDownLatch manuallyBindLatch;

  void manuallyBind() throws UnavailableProfileException {
    Log.e(LOG_TAG, "Calling manuallyBind");
    if (isRunningOnUIThread()) {
      throw new IllegalStateException("connect()/manuallyBind() cannot be called from UI thread");
    }

    if (!isBindingPossible()) {
      throw new UnavailableProfileException("Profile not available");
    }

    if (!binder.hasPermissionToBind(context)) {
      throw new UnavailableProfileException("Permission not granted");
    }

    cancelAutomaticDisconnection();
    isManuallyManagingConnection = true;

    if (isBound()) {
      // If we're already bound there's no need to block the thread
      return;
    }

    if (manuallyBindLatch == null) {
      synchronized (this) {
        if (manuallyBindLatch == null) {
          manuallyBindLatch = new CountDownLatch(1);
        }
      }
    }

    bind();

    Log.i(LOG_TAG, "Blocking for bind");
    try {
      if (manuallyBindLatch != null) {
        manuallyBindLatch.await();
      }
    } catch (InterruptedException e) {
      Log.e(LOG_TAG, "Interrupted waiting for manually bind", e);
    }

    if (!isBound()) {
      unbind(); // ensure we don't continue trying to connect if we throw an exception
      isManuallyManagingConnection = false;
      throw new UnavailableProfileException("Profile not available");
    }
  }

  private static boolean isRunningOnUIThread() {
    return Looper.myLooper() == Looper.getMainLooper();
  }

  /**
   * Start trying to bind to the other profile and start manually managing the connection.
   *
   * <p>This will mean that the connection will not be dropped automatically to save resources.
   *
   * <p>Must be called before interacting with synchronous cross-profile methods.
   */
  void startManuallyBinding() {
    cancelAutomaticDisconnection();
    isManuallyManagingConnection = true;
    bind();
  }

  /**
   * Stop manual connection management.
   *
   * <p>This can be called after {@link #startManuallyBinding()} or {@link #manuallyBind()} to
   * return connection management responsibilities to the SDK.
   *
   * <p>You should not make any synchronous cross-profile calls after calling this method.
   */
  public void stopManualConnectionManagement() {
    isManuallyManagingConnection = false;
    maybeScheduleAutomaticDisconnection();
  }

  /**
   * Attempt to bind to the other profile.
   *
   * <p>This will continually attempt to form a binding to the other profile in a background thread.
   */
  private void bind() {
    if (isBinding.getAndSet(true)) {
      return;
    }

    startTryBinding();
  }

  private void onBindingAttemptSucceeded() {
    Log.i(LOG_TAG, "Binding attempt succeeded");
    checkTriggerManualConnectionLock();
  }

  private void onBindingAttemptFailed(String reason) {
    onBindingAttemptFailed(reason, /* terminal= */ false);
  }

  private void onBindingAttemptFailed(String reason, boolean terminal) {
    Log.i(LOG_TAG, "Binding attempt failed: " + reason);
    throwExceptionForAsyncCalls(new UnavailableProfileException(reason));
    if (terminal || !isManuallyManagingConnection || manuallyBindLatch != null) {
      unbind();
      checkTriggerManualConnectionLock();
    } else {
      scheduleBindAttempt();
    }
  }

  private void checkTriggerManualConnectionLock() {
    if (manuallyBindLatch != null) {
      synchronized (this) {
        if (manuallyBindLatch != null) {
          manuallyBindLatch.countDown();
          manuallyBindLatch = null;
        }
      }
    }
  }

  /**
   * Stop attempting to bind to the other profile.
   *
   * <p>If there is already a binding present, it will be killed.
   */
  void unbind() {
    Log.i(LOG_TAG, "Unbind");
    throwExceptionForAsyncCalls(new UnavailableProfileException("No profile available"));
    isBinding.set(false);
    if (isBound()) {
      context.unbindService(connection);
      iCrossProfileService.set(null);
      checkConnected();
      cancelAutomaticDisconnection();
    }
    checkTriggerManualConnectionLock();
  }

  boolean isBindingPossible() {
    return binder.bindingIsPossible(context, availabilityRestrictions);
  }

  private void startTryBinding() {
    bindRetryDelayMs = INITIAL_BIND_RETRY_DELAY_MS;
    scheduledExecutorService.execute(this::tryBind);
  }

  private void tryBind() {
    Log.i(LOG_TAG, "Attempting to bind");

    if (scheduledTryBind != null) {
      scheduledTryBind.cancel(/* mayInterruptIfRunning= */ false);
      scheduledTryBind = null;
    }

    if (!canUseReflectedApis) {
      onBindingAttemptFailed("Required APIs are unavailable. Binding is not possible.");
      return;
    }

    if (!isBinding.get()) {
      onBindingAttemptFailed("Not trying to bind");
      return;
    }

    if (isBound()) {
      onBindingAttemptSucceeded();
      return;
    }

    if (!binder.hasPermissionToBind(context)) {
      onBindingAttemptFailed("Permission not granted");
      return;
    }

    if (!isBindingPossible()) {
      onBindingAttemptFailed("No profile available");
      return;
    }

    try {
      if (!binder.tryBind(context, bindToService, connection, availabilityRestrictions)) {
        onBindingAttemptFailed("No profile available or app not installed in other profile");
      }
    } catch (MissingApiException e) {
      Log.e(LOG_TAG, "MissingApiException when trying to bind", e);
      onBindingAttemptFailed("Missing API");
    }
  }

  private void scheduleBindAttempt() {
    if (scheduledTryBind != null && !scheduledTryBind.isDone()) {
      return;
    }

    bindRetryDelayMs *= 2;
    scheduledTryBind =
        scheduledExecutorService.schedule(this::tryBind, bindRetryDelayMs, TimeUnit.MILLISECONDS);
  }

  boolean isBound() {
    return iCrossProfileService.get() != null;
  }

  /**
   * Make a synchronous cross-profile call.
   *
   * @return A {@link Parcel} containing the return value. This must be recycled after use.
   * @throws UnavailableProfileException if a connection is not already established
   */
  public Parcel call(long crossProfileTypeIdentifier, int methodIdentifier, Parcel params)
      throws UnavailableProfileException {
    try {
      return callWithExceptions(crossProfileTypeIdentifier, methodIdentifier, params);
    } catch (UnavailableProfileException | RuntimeException e) {
      throw e;
    } catch (Throwable e) {
      throw new UnavailableProfileException("Unexpected checked exception", e);
    }
  }

  /**
   * Make a synchronous cross-profile call which expects some checked exceptions to be thrown.
   *
   * <p>Behaves the same as {@link #call(long, int, Parcel)} except that it deals with checked
   * exceptions by throwing {@link Throwable}.
   *
   * @return A {@link Parcel} containing the return value. This must be recycled after use.
   * @throws UnavailableProfileException if a connection is not already established
   */
  public Parcel callWithExceptions(
      long crossProfileTypeIdentifier, int methodIdentifier, Parcel params) throws Throwable {

    if (!isBound()) {
      throw new UnavailableProfileException("Could not access other profile");
    }

    if (!isManuallyManagingConnection) {
      throw new UnavailableProfileException(
          "Synchronous calls can only be used when manually connected");
    }

    CrossProfileParcelCallSender callSender =
        new CrossProfileParcelCallSender(
            iCrossProfileService.get(),
            crossProfileTypeIdentifier,
            methodIdentifier,
            /* callback= */ null);
    Parcel parcel = callSender.makeParcelCall(params); // Recycled by caller
    boolean hasError = parcel.readInt() == 1;

    if (hasError) {
      Throwable t = ParcelUtilities.readThrowableFromParcel(parcel);
      if (t instanceof RuntimeException) {
        throw new ProfileRuntimeException((RuntimeException) t);
      }
      throw t;
    }

    return parcel;
  }

  /**
   * Make an asynchronous cross-profile call.
   *
   * @param params These will be cached and will be recycled after the call is complete.
   */
  public void callAsync(
      long crossProfileTypeIdentifier,
      int methodIdentifier,
      Parcel params,
      LocalCallback callback,
      long timeoutMillis) {

    cancelAutomaticDisconnection();

    asyncCallQueue.add(
        new CrossProfileCall(
            crossProfileTypeIdentifier, methodIdentifier, params, callback, timeoutMillis));

    tryMakeAsyncCalls();
    if (isManuallyManagingConnection) {
      if (!isBindingPossible()) {
        throwExceptionForAsyncCalls(new UnavailableProfileException("Profile not available"));
      }
    } else {
      bind();
    }
  }

  private void throwExceptionForAsyncCalls(Throwable throwable) {
    Parcel throwableParcel = createThrowableParcel(throwable);

    while (true) {
      CrossProfileCall call = asyncCallQueue.pollFirst();
      if (call == null) {
        break;
      }

      call.callback.onException(throwableParcel);
      throwableParcel.setDataPosition(0);
      call.recycle();
    }

    while (true) {
      OngoingCrossProfileCall call = ongoingCrossProfileCalls.pollFirst();
      if (call == null) {
        break;
      }

      call.onException(throwableParcel);
      throwableParcel.setDataPosition(0);
    }

    throwableParcel.recycle();
  }

  private void tryMakeAsyncCalls() {
    if (!isBound()) {
      return;
    }

    scheduledExecutorService.execute(this::drainAsyncQueue);
  }

  private void drainAsyncQueue() {
    while (true) {
      CrossProfileCall call = asyncCallQueue.pollFirst();
      if (call == null) {
        break;
      }
      OngoingCrossProfileCall ongoingCall =
          new OngoingCrossProfileCall(this, call.callback, call.timeoutMillis);
      ongoingCrossProfileCalls.add(ongoingCall);

      try {
        CrossProfileParcelCallSender callSender =
            new CrossProfileParcelCallSender(
                iCrossProfileService.get(),
                call.crossProfileTypeIdentifier,
                call.methodIdentifier,
                ongoingCall);
        Parcel p = callSender.makeParcelCall(call.params);

        boolean hasError = p.readInt() == 1;
        call.recycle();

        if (hasError) {
          RuntimeException exception =
              (RuntimeException) ParcelUtilities.readThrowableFromParcel(p);
          p.recycle();
          ongoingCrossProfileCalls.remove(ongoingCall);
          throw new ProfileRuntimeException(exception);
        }

        p.recycle();
        ongoingCall.scheduleTimeout(scheduledExecutorService);
      } catch (UnavailableProfileException e) {
        ongoingCrossProfileCalls.remove(ongoingCall);
        asyncCallQueue.add(call);
        return;
      }
    }
  }

  void checkAvailability() {
    if (isBindingPossible() && (lastReportedAvailabilityStatus != AVAILABLE)) {
      updateAvailability();
    } else if (!isBindingPossible() && (lastReportedAvailabilityStatus != UNAVAILABLE)) {
      updateAvailability();
    }
  }

  void updateAvailability() {
    scheduledExecutorService.execute(availabilityListener::availabilityChanged);
    lastReportedAvailabilityStatus = isBindingPossible() ? AVAILABLE : UNAVAILABLE;
  }

  void checkConnected() {
    if (isBound() && lastReportedConnectedStatus != CONNECTED) {
      scheduledExecutorService.execute(connectionListener::connectionChanged);
      lastReportedConnectedStatus = CONNECTED;
    } else if (!isBound() && lastReportedConnectedStatus != DISCONNECTED) {
      scheduledExecutorService.execute(connectionListener::connectionChanged);
      lastReportedConnectedStatus = DISCONNECTED;
    }
  }

  boolean isManuallyManagingConnection() {
    return isManuallyManagingConnection;
  }

  /**
   * Create a {@link Parcel} containing a {@link Throwable}.
   *
   * <p>The {@link Parcel} must be recycled after use.
   */
  private static Parcel createThrowableParcel(Throwable throwable) {
    Parcel parcel = Parcel.obtain(); // Recycled by caller
    ParcelUtilities.writeThrowableToParcel(parcel, throwable);
    parcel.setDataPosition(0);
    return parcel;
  }

  @Nullable
  static UserHandle getOtherUserHandle(
      Context context, AvailabilityRestrictions availabilityRestrictions) {
    if (VERSION.SDK_INT < VERSION_CODES.P) {
      // CrossProfileApps was introduced in P
      return findDifferentRunningUser(
          context, android.os.Process.myUserHandle(), availabilityRestrictions);
    }

    CrossProfileApps crossProfileApps = context.getSystemService(CrossProfileApps.class);
    List<UserHandle> otherUsers =
        filterUsersByAvailabilityRestrictions(
            context, crossProfileApps.getTargetUserProfiles(), availabilityRestrictions);

    return selectUserHandleToBind(context, otherUsers);
  }

  @Nullable
  private static UserHandle findDifferentRunningUser(
      Context context,
      UserHandle ignoreUserHandle,
      AvailabilityRestrictions availabilityRestrictions) {
    UserManager userManager = context.getSystemService(UserManager.class);
    List<UserHandle> otherUsers = new ArrayList<>();

    for (UserHandle userHandle : userManager.getUserProfiles()) {
      if (!userHandle.equals(ignoreUserHandle)) {
        otherUsers.add(userHandle);
      }
    }

    otherUsers =
        filterUsersByAvailabilityRestrictions(context, otherUsers, availabilityRestrictions);

    return selectUserHandleToBind(context, otherUsers);
  }
}
