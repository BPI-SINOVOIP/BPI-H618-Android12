package com.bigbigcloud.devicehive.entity;

import java.io.Serializable;

/**
 * Represents device data.
 */

public class Device implements Serializable {

	private JsonStringWrapper data;
	private String deviceGuid;
	private String deviceId;
	private String name;
	private String mac;
	private String vendor;
	private String firmwareVersion;
	private String romType;
	private DeviceClass deviceClass;

	/**
	 * Construct device data with given parameters.
	 * 
	 * @param deviceId
	 *            Device unique identifier.
	 *
	 * @param name
	 *            Device display name.
	 */
	public Device(String deviceId, String name, String mac, String vendor, String firmwareversion, String romType, DeviceClass deviceClass) {
		this(null, deviceId, name, mac, vendor, firmwareversion, romType,deviceClass);
	}


	/* package */Device(JsonStringWrapper data, String deviceId, String name, String mac,
						 String vendor, String firmwareVersion, String romType, DeviceClass deviceClass) {
		this.data = data;
		this.deviceId = deviceId;
		this.name = name;
		this.mac = mac;
		this.vendor = vendor;
		this.firmwareVersion = firmwareVersion;
		this.romType = romType;
		this.deviceClass = deviceClass;
	}


	public String getDeviceGuid() {
		return deviceGuid;
	}

	public void setDeviceGuid(String deviceGuid) {
		this.deviceGuid = deviceGuid;
	}

	/**
	 * Get device identifier.
	 * 
	 * @return Device identifier.
	 */
	public String getDeviceId() {
		return deviceId;
	}

	/**
	 * Get device display name.
	 * 
	 * @return Device display name.
	 */
	public String getName() {
		return name;
	}

	public String getMac() {
		return mac;
	}

	public String getVendor() {
		return vendor;
	}

	public String getFirmwareVersion() {
		return firmwareVersion;
	}

	public String getRomType() {
		return romType;
	}

	public DeviceClass getDeviceClass() {
		return deviceClass;
	}

	public String getData() {
		return data.getJsonString();
	}

	public void setDeviceId(String deviceId) {
		this.deviceId = deviceId;
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setMac(String mac) {
		this.mac = mac;
	}

	public void setVendor(String vendor) {
		this.vendor = vendor;
	}

	public void setFirmwareVersion(String firmwareVersion) {
		this.firmwareVersion = firmwareVersion;
	}

	public void setRomType(String romType) {
		this.romType = romType;
	}

	public void setDeviceClass(DeviceClass deviceClass) {
		this.deviceClass = deviceClass;
	}

	public void setData(String data) {
		this.data = new JsonStringWrapper(data);
	}

	@Override
	public boolean equals(Object o) {
		if (this == o) return true;
		if (o == null || getClass() != o.getClass()) return false;

		Device device = (Device) o;

		return deviceId.equals(device.deviceId);

	}

	@Override
	public int hashCode() {
		return deviceId.hashCode();
	}

	@Override
	public String toString() {
		return "Device [deviceGuid= " + deviceGuid + "id=" + deviceId + ", +  name=" + name + " mac=" + mac + " vendor=" + vendor
				 + "firmwareVersion = " + firmwareVersion + "romType = " + romType + "model = " + deviceClass.getName()+ " data=" + data + "]";
	}

}
