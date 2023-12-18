package android.location.cts.gnss;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.location.GnssAntennaInfo;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.cts.common.TestLocationManager;
import android.location.cts.common.TestMeasurementUtil;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.List;

/** Tests {@link GnssAntennaInfo} values. */
@RunWith(JUnit4.class)
public class GnssAntennaInfoTest {

    private static final String TAG = "GnssAntInfoValuesTest";
    private static final int STATUS_TO_COLLECT_COUNT = 20;
    private static final int HZ_PER_MHZ = (int) 1e6;
    private static final double CARRIER_FREQ_TOLERANCE_HZ = 10 * 1e6;

    private TestLocationManager mTestLocationManager;

    @Before
    public void setUp() throws Exception {
        Context context = ApplicationProvider.getApplicationContext();
        mTestLocationManager = new TestLocationManager(context);
    }

    /**
     * Tests that the carrier frequency reported from {@link GnssAntennaInfo} can be found in
     * GnssStatus.
     */
    @Test
    public void testGnssAntennaInfoValues() throws Exception {
        // Checks if GPS hardware feature is present, skips test (pass) if not
        assumeTrue(TestMeasurementUtil.canTestRunOnCurrentDevice(mTestLocationManager, TAG));

        // Checks if ANTENNA_INFO capability is supported, skips the test if no
        assumeTrue(
                mTestLocationManager.getLocationManager().getGnssCapabilities().hasAntennaInfo());

        // Registers GnssStatus Listener
        TestGnssStatusCallback testGnssStatusCallback =
                new TestGnssStatusCallback(TAG, STATUS_TO_COLLECT_COUNT);
        checkGnssChange(testGnssStatusCallback);

        float[] carrierFrequencies = testGnssStatusCallback.getCarrierFrequencies();
        List<GnssAntennaInfo> antennaInfos =
                mTestLocationManager.getLocationManager().getGnssAntennaInfos();

        assertThat(antennaInfos).isNotNull();
        for (GnssAntennaInfo antennaInfo : antennaInfos) {
            double antennaInfoFreqHz = antennaInfo.getCarrierFrequencyMHz() * HZ_PER_MHZ;
            assertWithMessage(
                    "Carrier frequency in GnssAntennaInfo must be found in GnssStatus.").that(
                    carrierFrequencies).usingTolerance(CARRIER_FREQ_TOLERANCE_HZ).contains(
                    antennaInfoFreqHz);
        }
    }

    private void checkGnssChange(TestGnssStatusCallback testGnssStatusCallback)
            throws InterruptedException {
        mTestLocationManager.registerGnssStatusCallback(testGnssStatusCallback);

        LocationListener listener = location -> {};
        mTestLocationManager.getLocationManager().requestLocationUpdates(
                LocationManager.GPS_PROVIDER, /* minTimeMs= */0, /* minDistanceM= */ 0, listener,
                Looper.getMainLooper());

        boolean success = testGnssStatusCallback.awaitStart();
        success = success ? testGnssStatusCallback.awaitStatus() : false;
        success = success ? testGnssStatusCallback.awaitTtff() : false;
        mTestLocationManager.getLocationManager().removeUpdates(listener);
        success = success ? testGnssStatusCallback.awaitStop() : false;
        mTestLocationManager.unregisterGnssStatusCallback(testGnssStatusCallback);

        assertWithMessage(
                "Time elapsed without getting the right status changes."
                        + " Possibly, the test has been run deep indoors."
                        + " Consider retrying test outdoors.").that(success).isTrue();
    }
}
