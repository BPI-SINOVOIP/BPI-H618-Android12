package com.bigbigcloud.devicehive.entity;

import com.google.gson.JsonArray;
import com.google.gson.annotations.SerializedName;

import java.io.Serializable;

/**
 * Represents a device notification, a unit of information dispatched from
 */

public class DeviceNotification implements Serializable {
	private int id;
	@SerializedName("notification")
	private String name;
	private String timestamp;
	private JsonArray users;
	private JsonStringWrapper parameters;

	public DeviceNotification(int id, String name, String timestamp,
								 String parameters) {
		this.id = id;
		this.name = name;
		this.timestamp = timestamp;
		this.parameters = new JsonStringWrapper(parameters);
	}

	public DeviceNotification(String name, JsonArray users,
								 String parameters) {
		this.id = -1;
		this.name = name;
		this.timestamp = null;
		this.users = users;
		this.parameters = new JsonStringWrapper(parameters);

	}

	/**
	 * Construct a new notification with given name and parameters.
	 * 
	 * @param name
	 *            DeviceNotification name.
	 * @param parameters
	 *            DeviceNotification parameters.
	 */
	public DeviceNotification(String name, String parameters) {
		this(-1, name, null, parameters);
	}

	/**
	 * Get notification identifier.
	 * 
	 * @return DeviceNotification identifier.
	 */
	public int getId() {
		return id;
	}

	/**
	 * Get notification name.
	 * 
	 * @return DeviceNotification name.
	 */
	public String getName() {
		return name;
	}

	/**
	 * Get notification timestamp(UTC).
	 * 
	 * @return DeviceNotification timestamp(UTC).
	 */
	public String getTimestamp() {
		return timestamp;
	}

	/**
	 * Get notification parameters dictionary.
	 * 
	 * @return DeviceNotification parameters dictionary.
	 */
	public String getParameters() {
		return parameters != null ? parameters.getJsonString() : null;
	}


	@Override
	public String toString() {
		return "DeviceNotification [id=" + id + ", name=" + name + ", timestamp="
				+ timestamp + ", parameters=" + parameters + "]";
	}

}
