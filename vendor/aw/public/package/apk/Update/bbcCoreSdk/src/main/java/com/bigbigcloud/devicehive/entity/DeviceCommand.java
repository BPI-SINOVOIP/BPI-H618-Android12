package com.bigbigcloud.devicehive.entity;

import java.io.Serializable;

/**
 * Represents a device command, a unit of information sent to devices.
 */

public class DeviceCommand implements Serializable {
	private int id;
	private String timestamp;
	private String command;
	private JsonStringWrapper parameters;
	private int lifetime;
	private int flags;
	private String status;
	private String result;

	/* package */DeviceCommand(int id, String timestamp, String command,
							   String parameters, int lifetime, int flags, String status,
							   String result) {
		this.id = id;
		this.timestamp = timestamp;
		this.command = command;
		this.parameters = new JsonStringWrapper(parameters);
		this.lifetime = lifetime;
		this.flags = flags;
		this.status = status;
		this.result = result;
	}

	public DeviceCommand(String command, String parameters, int lifetime,
						 int flags) {
		this(-1, null, command, parameters, lifetime, flags, null, null);
	}

	/**
	 * Create command with given name and parameters.
	 * 
	 * @param command
	 *            DeviceCommand name.
	 * @param parameters
	 *            Parameters dictionary.
	 */
	public DeviceCommand(String command, String parameters) {
		this(-1, null, command, parameters, 0, 0, null, null);
	}

	/**
	 * Get command identifier.
	 * 
	 * @return DeviceCommand identifier set by the server.
	 */
	public int getId() {
		return id;
	}

	/**
	 * Get command timestamp (UTC).
	 * 
	 * @return Datetime timestamp associated with this command.
	 */
	public String getTimestamp() {
		return timestamp;
	}

	/**
	 * Get command name.
	 * 
	 * @return DeviceCommand name.
	 */
	public String getCommand() {
		return command;
	}

	/**
	 * Get command parameters dictionary.
	 * 
	 * @return DeviceCommand parameters dictionary.
	 */
	public Serializable getParameters() {
		return parameters != null ? parameters.getJsonString() : null;
	}

	/**
	 * Get command lifetime.
	 * 
	 * @return Number of seconds until this command expires.
	 */
	public int getLifetime() {
		return lifetime;
	}

	/**
	 * Get command flags. It's optional.
	 * 
	 * @return Value that could be supplied for device or related
	 *         infrastructure.
	 */
	public int getFlags() {
		return flags;
	}

	/**
	 * Get command status, as reported by device or related infrastructure.
	 * 
	 * @return DeviceCommand status.
	 */
	public String getStatus() {
		return status;
	}

	/**
	 * Get command execution result. It's optional value that could be provided
	 * by device.
	 * 
	 * @return DeviceCommand execution result.
	 */
	public String getResult() {
		return result;
	}


	@Override
	public String toString() {
		return "DeviceCommand [id=" + id + ", timestamp=" + timestamp + ", command="
				+ command + ", parameters=" + parameters + ", lifetime="
				+ lifetime + ", flags=" + flags + ", status=" + status
				+ ", result=" + result + "]";
	}

}
