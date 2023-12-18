package android.car.telemetry;

import android.car.telemetry.ICarTelemetryServiceListener;
import android.car.telemetry.ManifestKey;

/**
 * Internal binder interface for {@code CarTelemetryService}, used by {@code CarTelemetryManager}.
 *
 * @hide
 */
interface ICarTelemetryService {

    /**
     * Registers a listener with CarTelemetryService for the service to send data to cloud app.
     */
    void setListener(in ICarTelemetryServiceListener listener);

    /**
     * Clears the listener registered with CarTelemetryService.
     */
    void clearListener();

    /**
     * Sends telemetry manifests to CarTelemetryService.
     */
    int addManifest(in ManifestKey key, in byte[] manifest);

    /**
     * Removes a manifest based on the key.
     */
    boolean removeManifest(in ManifestKey key);

    /**
     * Removes all manifests.
     */
    void removeAllManifests();

    /**
     * Sends script results associated with the given key using the
     * {@code ICarTelemetryServiceListener}.
     */
    void sendFinishedReports(in ManifestKey key);

    /**
     * Sends all script results associated using the {@code ICarTelemetryServiceListener}.
     */
    void sendAllFinishedReports();

    /**
     * Sends all errors using the {@code ICarTelemetryServiceListener}.
     */
    void sendScriptExecutionErrors();
}