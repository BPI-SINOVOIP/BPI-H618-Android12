package com.softwinner.timerswitch.data;

import android.util.ArrayMap;
import java.io.Serializable;
import java.util.Calendar;
import java.util.Collections;
import java.util.Map;
import java.util.Objects;

public class Weekdays implements Serializable,Comparable<Weekdays> {

    /** All valid bits set. */
    private static final int ALL_DAYS = 0x7F;

    /** An instance with no weekdays in the weekly repeat cycle. */
    public static final Weekdays NONE = Weekdays.fromBits(0);

    public static final Weekdays MONDAY = Weekdays.fromBits(0x01);
    public static final Weekdays TUESDAY = Weekdays.fromBits(0x02);
    public static final Weekdays WEDNESDAY = Weekdays.fromBits(0x04);
    public static final Weekdays THURSDAY = Weekdays.fromBits(0x08);
    public static final Weekdays FRIDAY = Weekdays.fromBits(0x10);
    public static final Weekdays SATURDAY = Weekdays.fromBits(0x20);
    public static final Weekdays SUNDAY = Weekdays.fromBits(0x40);

    /** Maps calendar weekdays to the bit masks that represent them in this class. */
    private static final Map<Integer, Integer> sCalendarDayToBit;
    static {
        final Map<Integer, Integer> map = new ArrayMap<>(7);
        map.put(Calendar.MONDAY,    0x01);
        map.put(Calendar.TUESDAY,   0x02);
        map.put(Calendar.WEDNESDAY, 0x04);
        map.put(Calendar.THURSDAY,  0x08);
        map.put(Calendar.FRIDAY,    0x10);
        map.put(Calendar.SATURDAY,  0x20);
        map.put(Calendar.SUNDAY,    0x40);
        sCalendarDayToBit = Collections.unmodifiableMap(map);
    }

    /** An encoded form of a weekly repeat schedule. */
    private final int mBits;

    private Weekdays(int bits) {
        // Mask off the unused bits.
        mBits = ALL_DAYS & bits;
    }

    /**
     * @param bits representing the encoded weekly repeat schedule
     */
    public static Weekdays fromBits(int bits) {
        return new Weekdays(bits);
    }

    public static Weekdays fromCalendarDay(String calendarDay){
        if(calendarDay == null) return NONE;
        switch (calendarDay){
            case "MONDAY":
                return MONDAY;
            case "TUESDAY":
                return TUESDAY;
            case "WEDNESDAY":
                return WEDNESDAY;
            case "THURSDAY":
                return THURSDAY;
            case "FRIDAY":
                return FRIDAY;
            case "SATURDAY":
                return SATURDAY;
            case "SUNDAY":
                return SUNDAY;
        }
        return NONE;
    }

    /**
     * @param calendarDay any of the following values
     */
    public boolean isBitOn(int calendarDay) {
        final Integer bit = sCalendarDayToBit.get(calendarDay);
        if (bit == null) {
            throw new IllegalArgumentException(calendarDay + " is not a valid weekday");
        }
        return (mBits & bit) > 0;
    }

    public int getBits() {
        return mBits;
    }

    /**
     * only the day-of-week is read from the . The time fields
     * are not considered in this computation.
     */
    public int getDistanceToNextDay(Calendar time) {
        int calendarDay = time.get(Calendar.DAY_OF_WEEK);
        for (int count = 0; count < 7; count++) {
            if (isBitOn(calendarDay)) {
                return count;
            }
            calendarDay++;
            // One week after calculation
            if (calendarDay > Calendar.SATURDAY) {
                calendarDay = Calendar.SUNDAY;
            }
        }
        return -1;
    }

    public static Weekdays getNextWeekDays(Weekdays curWeekdays){
        int bit = curWeekdays.mBits;
        if(bit == 0x40) {
            bit = 0x01;
            return Weekdays.fromBits(bit);
        }
        return Weekdays.fromBits(bit * 2);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Weekdays weekdays = (Weekdays) o;
        return mBits == weekdays.mBits;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mBits);
    }

    @Override
    public String toString() {
        String ret;
        switch (mBits){
            case 0x01 :
                ret = "MONDAY";
                break;
            case 0x02 :
                ret = "TUESDAY";
                break;
            case 0x04 :
                ret = "WEDNESDAY";
                break;
            case 0x08 :
                ret = "THURSDAY";
                break;
            case 0x10 :
                ret = "FRIDAY";
                break;
            case 0x20 :
                ret = "SATURDAY";
                break;
            case 0x40 :
                ret = "SUNDAY";
                break;
            default:
                ret = null;
        }
        return ret;
    }

    @Override
    public int compareTo(Weekdays o) {
        return Integer.compare(this.mBits, o.mBits);
    }
}