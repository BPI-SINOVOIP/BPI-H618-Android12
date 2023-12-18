package android.location.cts.privileged;

import static org.junit.Assert.assertEquals;

import android.location.GnssCapabilities;
import android.os.Parcel;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests fundamental functionality of {@link GnssCapabilities}. This includes writing and reading
 * from parcel, and verifying setters.
 */
@RunWith(AndroidJUnit4.class)
public class GnssCapabilitiesTest {

    @Test
    public void testBuilderWithGnssCapabilities() {
        GnssCapabilities gnssCapabilities =
            new GnssCapabilities.Builder(getTestGnssCapabilities()).build();
        verifyTestValues(gnssCapabilities);
    }

    @Test
    public void testGetValues() {
        GnssCapabilities gnssCapabilities = getTestGnssCapabilities();
        verifyTestValues(gnssCapabilities);
    }

    @Test
    public void testWriteToParcel() {
        GnssCapabilities gnssCapabilities = getTestGnssCapabilities();
        Parcel parcel = Parcel.obtain();
        gnssCapabilities.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        GnssCapabilities newGnssCapabilities = GnssCapabilities.CREATOR.createFromParcel(parcel);
        verifyTestValues(newGnssCapabilities);
        parcel.recycle();
    }

    private static GnssCapabilities getTestGnssCapabilities() {
        GnssCapabilities.Builder builder = new GnssCapabilities.Builder();
        builder.setHasAntennaInfo(true);
        builder.setHasGeofencing(true);
        builder.setHasLowPowerMode(true);
        builder.setHasMeasurements(true);
        builder.setHasMeasurementCorrections(true);
        builder.setHasMeasurementCorrectionsExcessPathLength(true);
        builder.setHasMeasurementCorrectionsForDriving(true);
        builder.setHasMeasurementCorrectionsLosSats(true);
        builder.setHasMeasurementCorrectionsReflectingPlane(true);
        builder.setHasMeasurementCorrelationVectors(true);
        builder.setHasNavigationMessages(true);
        builder.setHasSatelliteBlocklist(true);
        builder.setHasSatellitePvt(true);
        return builder.build();
    }

    private static void verifyTestValues(GnssCapabilities gnssCapabilities) {
        assertEquals(true, gnssCapabilities.hasAntennaInfo());
        assertEquals(true, gnssCapabilities.hasGeofencing());
        assertEquals(true, gnssCapabilities.hasLowPowerMode());
        assertEquals(true, gnssCapabilities.hasMeasurements());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrections());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrectionsExcessPathLength());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrectionsForDriving());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrectionsLosSats());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrectionsReflectingPlane());
        assertEquals(true, gnssCapabilities.hasMeasurementCorrelationVectors());
        assertEquals(true, gnssCapabilities.hasNavigationMessages());
        assertEquals(true, gnssCapabilities.hasSatelliteBlocklist());
        assertEquals(true, gnssCapabilities.hasSatellitePvt());
    }
}