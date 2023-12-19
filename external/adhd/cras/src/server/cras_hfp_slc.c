/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/socket.h>
#include <syslog.h>

#include "stdbool.h"

#include "cras_bt_device.h"
#include "cras_bt_log.h"
#include "cras_observer.h"
#include "cras_telephony.h"
#include "cras_hfp_slc.h"
#include "cras_server_metrics.h"
#include "cras_system_state.h"
#include "cras_tm.h"
#include "cras_util.h"

/* Message start and end with "\r\n". refer to spec 4.33. */
#define AT_CMD(cmd) "\r\n" cmd "\r\n"

/* The timeout between event reporting and HF indicator commands */
#define HF_INDICATORS_TIMEOUT_MS 2000
/* The sleep time before reading and processing the following AT commands during
 * codec connection setup.
 */
#define CODEC_CONN_SLEEP_TIME_US 2000
#define SLC_BUF_SIZE_BYTES 256

/* Indicator update command response and indicator indices.
 * Note that indicator index starts from '1', index 0 is used for CRAS to record
 * if the event report has been enabled or not.
 */
#define CRAS_INDICATOR_ENABLE_INDEX 0
#define BATTERY_IND_INDEX 1
#define SIGNAL_IND_INDEX 2
#define SERVICE_IND_INDEX 3
#define CALL_IND_INDEX 4
#define CALLSETUP_IND_INDEX 5
#define CALLHELD_IND_INDEX 6
#define ROAM_IND_INDEX 7
#define INDICATOR_IND_MAX 8
#define INDICATOR_UPDATE_RSP                                                   \
	"+CIND: "                                                              \
	"(\"battchg\",(0-5)),"                                                 \
	"(\"signal\",(0-5)),"                                                  \
	"(\"service\",(0,1)),"                                                 \
	"(\"call\",(0,1)),"                                                    \
	"(\"callsetup\",(0-3)),"                                               \
	"(\"callheld\",(0-2)),"                                                \
	"(\"roam\",(0,1))"                                                     \
	""
/* Mode values for standard event reporting activation/deactivation AT
 * command AT+CMER. Used for indicator events reporting in HFP. */
#define FORWARD_UNSOLICIT_RESULT_CODE 3

/* Handle object to hold required info to initialize and maintain
 * an HFP service level connection.
 * Args:
 *    buf - Buffer hold received commands.
 *    buf_read_idx - Read index for buf.
 *    buf_write_idx - Write index for buf.
 *    rfcomm_fd - File descriptor for the established RFCOMM connection.
 *    init_cb - Callback to be triggered when an SLC is initialized.
 *    cli_active - Calling line identification notification is enabled or not.
 *    battery - Current battery level of AG stored in SLC.
 *    signal - Current signal strength of AG stored in SLC.
 *    service - Current service availability of AG stored in SLC.
 *    callheld - Current callheld status of AG stored in SLC.
 *    ind_event_reports - Activate statuses of indicator events reporting.
 *    ag_supported_features - Supported AG features bitmap.
 *    hf_supported_features - Bit map of HF supported features.
 *    hf_supports_battery_indicator - Bit map of battery indicator support of
 *        connected HF.
 *    hf_battery - Current battery level of HF reported by the HF. The data
 *    range should be 0 ~ 100. Use -1 for no battery level reported.
 *    preferred_codec - CVSD or mSBC based on the situation and strategy. This
 *        needs not to be equal to selected_codec because codec negotiation
 *        process may fail.
 *    selected_codec - The codec id defaults to HFP_CODEC_UNUSED and changes
 *        only if codec negotiation is supported and the negotiation flow
 *        has completed.
 *    telephony - A reference of current telephony handle.
 *    device - The associated bt device.
 */
struct hfp_slc_handle {
	char buf[SLC_BUF_SIZE_BYTES];
	int buf_read_idx;
	int buf_write_idx;
	int is_hsp;
	int rfcomm_fd;
	hfp_slc_init_cb init_cb;
	hfp_slc_disconnect_cb disconnect_cb;
	int cli_active;
	int battery;
	int signal;
	int service;
	int callheld;
	int ind_event_reports[INDICATOR_IND_MAX];
	int ag_supported_features;
	bool hf_codec_supported[HFP_MAX_CODECS];
	int hf_supported_features;
	int hf_supports_battery_indicator;
	int hf_battery;
	int preferred_codec;
	int selected_codec;
	struct cras_bt_device *device;
	struct cras_timer *timer;

	struct cras_telephony_handle *telephony;
};

/* AT command exchanges between AG(Audio gateway) and HF(Hands-free device) */
struct at_command {
	const char *cmd;
	int (*callback)(struct hfp_slc_handle *handle, const char *cmd);
};

/* Sends a response or command to HF */
static int hfp_send(struct hfp_slc_handle *handle, const char *buf)
{
	int written, err, len;

	if (handle->rfcomm_fd < 0)
		return -EIO;

	len = strlen(buf);
	written = 0;
	while (written < len) {
		err = write(handle->rfcomm_fd, buf + written, len - written);
		if (err < 0)
			return -errno;
		written += err;
	}

	return 0;
}

/* Sends a response for indicator event reporting. */
static int hfp_send_ind_event_report(struct hfp_slc_handle *handle,
				     int ind_index, int value)
{
	char cmd[64];

	if (handle->is_hsp ||
	    !handle->ind_event_reports[CRAS_INDICATOR_ENABLE_INDEX] ||
	    !handle->ind_event_reports[ind_index])
		return 0;

	snprintf(cmd, 64, AT_CMD("+CIEV: %d,%d"), ind_index, value);
	return hfp_send(handle, cmd);
}

/* Sends calling line identification unsolicited result code and
 * standard call waiting notification. */
static int hfp_send_calling_line_identification(struct hfp_slc_handle *handle,
						const char *number, int type)
{
	char cmd[64];

	if (handle->is_hsp)
		return 0;

	if (handle->telephony->call) {
		snprintf(cmd, 64, AT_CMD("+CCWA: \"%s\",%d"), number, type);
	} else {
		snprintf(cmd, 64, AT_CMD("+CLIP: \"%s\",%d"), number, type);
	}
	return hfp_send(handle, cmd);
}

/* ATA command to accept an incoming call. Mandatory support per spec 4.13. */
static int answer_call(struct hfp_slc_handle *handle, const char *cmd)
{
	int rc;
	rc = hfp_send(handle, AT_CMD("OK"));
	if (rc)
		return rc;

	return cras_telephony_event_answer_call();
}

/* AT+CCWA command to enable the "Call Waiting notification" function.
 * Mandatory support per spec 4.21. */
static int call_waiting_notify(struct hfp_slc_handle *handle, const char *buf)
{
	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+CLIP command to enable the "Calling Line Identification notification"
 * function. Mandatory per spec 4.23.
 */
static int cli_notification(struct hfp_slc_handle *handle, const char *cmd)
{
	if (strlen(cmd) < 9) {
		syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
		return hfp_send(handle, AT_CMD("ERROR"));
	}
	handle->cli_active = (cmd[8] == '1');
	return hfp_send(handle, AT_CMD("OK"));
}

/* ATDdd...dd command to place call with supplied number, or ATD>nnn...
 * command to dial the number stored at memory location. Mandatory per
 * spec 4.18 and 4.19.
 */
static int dial_number(struct hfp_slc_handle *handle, const char *cmd)
{
	int rc, cmd_len;

	cmd_len = strlen(cmd);
	if (cmd_len < 4)
		goto error_out;

	if (cmd[3] == '>') {
		/* Handle memory dial. Extract memory location from command
		 * ATD>nnn...; and lookup. */
		int memory_location;
		memory_location = strtol(cmd + 4, NULL, 0);
		if (handle->telephony->dial_number == NULL ||
		    memory_location != 1)
			return hfp_send(handle, AT_CMD("ERROR"));
	} else {
		/* ATDddddd; Store dial number to the only memory slot. */
		cras_telephony_store_dial_number(cmd_len - 3 - 1, cmd + 3);
	}

	rc = hfp_send(handle, AT_CMD("OK"));
	if (rc)
		return rc;

	handle->telephony->callsetup = 2;
	return hfp_send_ind_event_report(handle, CALLSETUP_IND_INDEX, 2);

error_out:
	syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
	return hfp_send(handle, AT_CMD("ERROR"));
}

/* AT+VTS command to generate a DTMF code. Mandatory per spec 4.27. */
static int dtmf_tone(struct hfp_slc_handle *handle, const char *buf)
{
	return hfp_send(handle, AT_CMD("OK"));
}

/* Sends +BCS command to tell HF about our preferred codec. This shall
 * be called only if codec negotiation is supported.
 */
static void select_preferred_codec(struct hfp_slc_handle *handle)
{
	char buf[64];
	snprintf(buf, 64, AT_CMD("+BCS:%d"), handle->preferred_codec);
	hfp_send(handle, buf);
	BTLOG(btlog, BT_CODEC_SELECTION, 0, handle->preferred_codec);
}

/* Marks SLC handle as initialized and trigger HFP AG's init_cb. */
static void initialize_slc_handle(struct cras_timer *timer, void *arg)
{
	struct hfp_slc_handle *handle = (struct hfp_slc_handle *)arg;
	if (timer)
		handle->timer = NULL;

	if (handle->init_cb) {
		handle->init_cb(handle);
		handle->init_cb = NULL;
	}
}

/* Handles the event that headset request to start a codec connection
 * procedure.
 */
static int bluetooth_codec_connection(struct hfp_slc_handle *handle,
				      const char *cmd)
{
	/* Reset current selected codec to force a new codec connection
	 * procedure when the next hfp_slc_codec_connection_setup is called.
	 */
	handle->selected_codec = HFP_CODEC_UNUSED;
	return hfp_send(handle, AT_CMD("OK"));
}

/* Handles the event that headset request to select specific codec. */
static int bluetooth_codec_selection(struct hfp_slc_handle *handle,
				     const char *cmd)
{
	char *tokens = strdup(cmd);
	char *codec;
	int id, err;

	strtok(tokens, "=");
	codec = strtok(NULL, ",");
	if (!codec)
		goto bcs_cmd_cleanup;
	id = atoi(codec);
	if ((id <= HFP_CODEC_UNUSED) || (id >= HFP_MAX_CODECS)) {
		syslog(LOG_ERR, "%s: invalid codec id: '%s'", __func__, cmd);
		free(tokens);
		return hfp_send(handle, AT_CMD("ERROR"));
	}

	if (id != handle->preferred_codec)
		syslog(LOG_WARNING, "%s: inconsistent codec id: '%s'", __func__,
		       cmd);

	BTLOG(btlog, BT_CODEC_SELECTION, 1, id);
	handle->selected_codec = id;

bcs_cmd_cleanup:
	free(tokens);
	err = hfp_send(handle, AT_CMD("OK"));
	return err;
}

/*
 * AT+IPHONEACCEV command from HF to report state change.You can find details
 * of this command in the Accessory Design Guidelines for Apple Devices R11
 * section 16.1.
 */
static int apple_accessory_state_change(struct hfp_slc_handle *handle,
					const char *cmd)
{
	char *tokens, *num, *key, *val;
	int i, level;

	/* AT+IPHONEACCEV=Number of key/value pairs,key1,val1,key2,val2,...
	 * Number of key/value pairs: The number of parameters coming next.
	 * key: the type of change being reported:
         *      1 = Battery Level
         *      2 = Dock State
         * val: the value of the change:
         * Battery Level: string value between '0' and '9'
         * Dock State: 0 = undocked, 1 = docked
	 */
	tokens = strdup(cmd);
	strtok(tokens, "=");
	num = strtok(NULL, ",");
	if (!num) {
		free(tokens);
		return hfp_send(handle, AT_CMD("ERROR"));
	}

	for (i = 0; i < atoi(num); i++) {
		key = strtok(NULL, ",");
		val = strtok(NULL, ",");
		if (!key || !val) {
			syslog(LOG_WARNING,
			       "IPHONEACCEV: Expected %d kv pairs but got %d",
			       atoi(num), i);
			break;
		}

		if (atoi(key) == 1) {
			level = atoi(val);
			if (level >= 0 && level < 10) {
				cras_server_metrics_hfp_battery_report(
					CRAS_HFP_BATTERY_INDICATOR_APPLE);
				level = (level + 1) * 10;
				if (handle->hf_battery != level) {
					handle->hf_battery = level;
					cras_observer_notify_bt_battery_changed(
						cras_bt_device_address(
							handle->device),
						(uint32_t)(level));
				}
			} else {
				syslog(LOG_ERR,
				       "Get invalid battery status from cmd:%s",
				       cmd);
			}
		}
	}
	free(tokens);
	return hfp_send(handle, AT_CMD("OK"));
}

/*
 * AT+XAPL command from HF to enable Apple custom features. You can find details
 * of it in the Accessory Design Guidelines for Apple Devices R11 section 15.1.
 */
static int apple_supported_features(struct hfp_slc_handle *handle,
				    const char *cmd)
{
	char *tokens, *features;
	int apple_features, err;
	char buf[64];

	/* AT+XAPL=<vendorID>-<productID>-<version>,<features>
	 * Parse <features>, the only token we care about.
	 */
	tokens = strdup(cmd);
	strtok(tokens, "=");

	strtok(NULL, ",");
	features = strtok(NULL, ",");
	if (!features)
		goto error_out;

	apple_features = atoi(features);

	if (apple_features & APL_BATTERY)
		handle->hf_supports_battery_indicator |=
			CRAS_HFP_BATTERY_INDICATOR_APPLE;

	snprintf(buf, 64, AT_CMD("+XAPL=iPhone,%d"),
		 CRAS_APL_SUPPORTED_FEATURES);
	err = hfp_send(handle, buf);
	if (err)
		goto error_out;

	err = hfp_send(handle, AT_CMD("OK"));
	free(tokens);
	return err;

error_out:
	syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
	free(tokens);
	return hfp_send(handle, AT_CMD("ERROR"));
}

/* Handles the event when headset reports its available codecs list. */
static int available_codecs(struct hfp_slc_handle *handle, const char *cmd)
{
	char *tokens, *id_str;
	int id;

	for (id = 0; id < HFP_MAX_CODECS; id++)
		handle->hf_codec_supported[id] = false;

	tokens = strdup(cmd);
	strtok(tokens, "=");
	id_str = strtok(NULL, ",");
	while (id_str) {
		id = atoi(id_str);
		if ((id > HFP_CODEC_UNUSED) && (id < HFP_MAX_CODECS)) {
			handle->hf_codec_supported[id] = true;
			BTLOG(btlog, BT_AVAILABLE_CODECS, 0, id);
		}
		id_str = strtok(NULL, ",");
	}

	if (hfp_slc_get_wideband_speech_supported(handle))
		handle->preferred_codec = HFP_CODEC_ID_MSBC;
	else
		handle->preferred_codec = HFP_CODEC_ID_CVSD;

	free(tokens);
	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+CMER command enables the registration status update function in AG.
 * The service level connection is consider initialized when successfully
 * responded OK to the AT+CMER command. Mandatory support per spec 4.4.
 */
static int event_reporting(struct hfp_slc_handle *handle, const char *cmd)
{
	char *tokens, *mode, *tmp;
	int err = 0;

	/* AT+CMER=[<mode>[,<keyp>[,<disp>[,<ind> [,<bfr>]]]]]
	 * Parse <ind>, the only token we care about.
	 */
	tokens = strdup(cmd);
	strtok(tokens, "=");

	mode = strtok(NULL, ",");
	tmp = strtok(NULL, ",");
	tmp = strtok(NULL, ",");
	tmp = strtok(NULL, ",");

	/* mode = 3 for forward unsolicited result codes.
	 * AT+CMER=3,0,0,1 activates “indicator events reporting”.
	 * The service level connection is considered established after
	 * successfully responded with OK, regardless of the indicator
	 * events reporting status.
	 */
	if (!mode || !tmp) {
		syslog(LOG_ERR, "Invalid event reporting” cmd %s", cmd);
		err = -EINVAL;
		goto event_reporting_done;
	}
	if (atoi(mode) == FORWARD_UNSOLICIT_RESULT_CODE)
		handle->ind_event_reports[CRAS_INDICATOR_ENABLE_INDEX] =
			atoi(tmp);

	err = hfp_send(handle, AT_CMD("OK"));
	if (err) {
		syslog(LOG_ERR, "Error sending response for command %s", cmd);
		goto event_reporting_done;
	}

	/*
	 * Wait for HF to retrieve information about HF indicators and consider
	 * the Service Level Connection to be fully initialized, and thereby
	 * established, if HF doesn't support HF indicators.
	 */
	if (hfp_slc_get_hf_hf_indicators_supported(handle))
		handle->timer =
			cras_tm_create_timer(cras_system_state_get_tm(),
					     HF_INDICATORS_TIMEOUT_MS,
					     initialize_slc_handle, handle);
	/*
	 * Otherwise, regard the Service Level Connection to be fully
	 * initialized and ready for the potential codec negotiation.
	 */
	else
		initialize_slc_handle(NULL, (void *)handle);

event_reporting_done:
	free(tokens);
	return err;
}

/* AT+CMEE command to set the "Extended Audio Gateway Error Result Code".
 * Mandatory per spec 4.9.
 */
static int extended_errors(struct hfp_slc_handle *handle, const char *buf)
{
	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+CKPD command to handle the user initiated action from headset profile
 * device.
 */
static int key_press(struct hfp_slc_handle *handle, const char *buf)
{
	hfp_send(handle, AT_CMD("OK"));

	/* Release the call and connection. */
	if (handle->telephony->call || handle->telephony->callsetup) {
		cras_telephony_event_terminate_call();
		handle->disconnect_cb(handle);
		return -EIO;
	}
	return 0;
}

/* AT+BLDN command to re-dial the last number. Mandatory support
 * per spec 4.20.
 */
static int last_dialed_number(struct hfp_slc_handle *handle, const char *buf)
{
	int rc;

	if (!handle->telephony->dial_number)
		return hfp_send(handle, AT_CMD("ERROR"));

	rc = hfp_send(handle, AT_CMD("OK"));
	if (rc)
		return rc;

	handle->telephony->callsetup = 2;
	return hfp_send_ind_event_report(handle, CALLSETUP_IND_INDEX, 2);
}

/* AT+CLCC command to query list of current calls. Mandatory support
 * per spec 4.31.
 *
 * +CLCC: <idx>,<direction>,<status>,<mode>,<multiparty>
 */
static int list_current_calls(struct hfp_slc_handle *handle, const char *cmd)
{
	char buf[64];

	int idx = 1;
	int rc;
	/* Fake the call list base on callheld and call status
	 * since we have no API exposed to manage call list.
	 * This is a hack to pass qualification test which ask us to
	 * handle the basic case that one call is active and
	 * the other is on hold. */
	if (handle->telephony->callheld) {
		snprintf(buf, 64, AT_CMD("+CLCC: %d,1,1,0,0"), idx++);
		rc = hfp_send(handle, buf);
		if (rc)
			return rc;
	}

	if (handle->telephony->call) {
		snprintf(buf, 64, AT_CMD("+CLCC: %d,1,0,0,0"), idx++);
		rc = hfp_send(handle, buf);
		if (rc)
			return rc;
	}

	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+COPS command to query currently selected operator or set name format.
 * Mandatory support per spec 4.8.
 */
static int operator_selection(struct hfp_slc_handle *handle, const char *buf)
{
	int rc;
	if (buf[7] == '?') {
		/* HF sends AT+COPS? command to find current network operator.
		 * AG responds with +COPS:<mode>,<format>,<operator>, where
		 * the mode=0 means automatic for network selection. If no
		 * operator is selected, <format> and <operator> are omitted.
		 */
		rc = hfp_send(handle, AT_CMD("+COPS: 0"));
		if (rc)
			return rc;
	}
	return hfp_send(handle, AT_CMD("OK"));
}

/* The AT+CHLD command is used to control call hold, release, and multiparty
 * states.
 */
static int call_hold(struct hfp_slc_handle *handle, const char *buf)
{
	int rc;

	// Chrome OS doesn't yet support CHLD features but we need to reply
	// the query with an empty feature list rather than "ERROR" to increase
	// interoperability with certain devices (b/172413440).
	if (strlen(buf) > 8 && buf[7] == '=' && buf[8] == '?') {
		rc = hfp_send(handle, AT_CMD("+CHLD:"));
		if (rc)
			return rc;
		return hfp_send(handle, AT_CMD("OK"));
	}

	return hfp_send(handle, AT_CMD("ERROR"));
}

/* AT+CIND command retrieves the supported indicator and its corresponding
 * range and order index or read current status of indicators. Mandatory
 * support per spec 4.2.
 */
static int report_indicators(struct hfp_slc_handle *handle, const char *cmd)
{
	int err;
	char buf[64];

	if (strlen(cmd) < 8) {
		syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
		return hfp_send(handle, AT_CMD("ERROR"));
	}

	if (cmd[7] == '=') {
		/* Indicator update test command "AT+CIND=?" */
		err = hfp_send(handle, AT_CMD(INDICATOR_UPDATE_RSP));
	} else {
		/* Indicator update read command "AT+CIND?".
		 * Respond with current status of AG indicators,
		 * the values must be listed in the indicator order declared
		 * in INDICATOR_UPDATE_RSP.
		 * +CIND: <signal>,<service>,<call>,
		 *        <callsetup>,<callheld>,<roam>
		 */
		snprintf(buf, 64, AT_CMD("+CIND: %d,%d,%d,%d,%d,%d,0"),
			 handle->battery, handle->signal, handle->service,
			 handle->telephony->call, handle->telephony->callsetup,
			 handle->telephony->callheld);
		err = hfp_send(handle, buf);
	}

	if (err < 0)
		return err;

	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+BIA command to change the subset of indicators that shall be
 * sent by the AG.
 */
static int indicator_activation(struct hfp_slc_handle *handle, const char *cmd)
{
	char *ptr;
	int idx = BATTERY_IND_INDEX;

	/* AT+BIA=[[<indrep 1>][,[<indrep 2>][,...[,[<indrep n>]]]]]
	 * According to the spec:
	 * - The indicator state can be omitted and the current reporting
	 *   states of the indicator shall not change.
	 *     Ex: AT+BIA=,1,,0
	 *         Only the 2nd and 4th indicators may be affected.
	 * - HF can provide fewer indicators than AG and states not provided
	 *   shall not change.
	 *     Ex: CRAS supports 7 indicators and gets AT+BIA=1,0,1
	 *         Only the first three indicators may be affected.
	 * - Call, Call Setup and Held Call are mandatory and should be always
	 *   on no matter what state HF set.
	 */
	ptr = strchr(cmd, '=');
	while (ptr && idx < INDICATOR_IND_MAX) {
		if (idx != CALL_IND_INDEX && idx != CALLSETUP_IND_INDEX &&
		    idx != CALLHELD_IND_INDEX) {
			if (*(ptr + 1) == '1')
				handle->ind_event_reports[idx] = 1;
			else if (*(ptr + 1) == '0')
				handle->ind_event_reports[idx] = 0;
		}
		ptr = strchr(ptr + 1, ',');
		idx++;
	}
	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+BIND command to report, query and activate Generic Status Indicators.
 * It is sent by the HF if both AG and HF support the HF indicator feature.
 */
static int indicator_support(struct hfp_slc_handle *handle, const char *cmd)
{
	char *tokens, *key;
	int err, cmd_len;

	cmd_len = strlen(cmd);
	if (cmd_len < 8)
		goto error_out;

	if (cmd[7] == '=') {
		/* AT+BIND=? (Read AG supported indicators) */
		if (cmd_len > 8 && cmd[8] == '?') {
			/* +BIND: (<a>,<b>,<c>,...,<n>) (Response to AT+BIND=?)
			 * <a> ... <n>: 0-65535, entered as decimal unsigned
			 * integer values without leading zeros, referencing an
			 * HF indicator assigned number.
			 * 1 is for Enhanced Driver Status.
			 * 2 is for Battery Level.
			 * For the list of HF indicator assigned number, you can
			 * check the  Bluetooth SIG Assigned Numbers web page.
			 */
			BTLOG(btlog, BT_HFP_HF_INDICATOR, 1, 0);
			/* "2" is for HF Battery Level that we support. We don't
			 * support "1" but this is a workaround for Pixel Buds 2
			 * which expects this exact combination for battery
			 * reporting (HFP 1.7 standard) to work. This workaround
			 * is fine since we don't enable Safety Drive with
			 * +BIND: 1,1 (b/172680041).
			 */
			err = hfp_send(handle, AT_CMD("+BIND: (1,2)"));
			if (err < 0)
				return err;
		}
		/* AT+BIND=<a>,<b>,...,<n>(List HF supported indicators) */
		else {
			tokens = strdup(cmd);
			strtok(tokens, "=");
			key = strtok(NULL, ",");
			while (key != NULL) {
				if (atoi(key) == 2)
					handle->hf_supports_battery_indicator |=
						CRAS_HFP_BATTERY_INDICATOR_HFP;
				key = strtok(NULL, ",");
			}
			free(tokens);
		}
	}
	/* AT+BIND? (Read AG enabled/disabled status of indicators) */
	else if (cmd[7] == '?') {
		/* +BIND: <a>,<state> (Unsolicited or Response to AT+BIND?)
		 * This response enables the AG to notify the HF which HF
		 * indicators are supported and their state, enabled or
		 * disabled.
		 * <a>: 1 or 2, referencing an HF indicator assigned number.
		 * <state>: 0-1, entered as integer values, where
		 * 0 = disabled, no value changes shall be sent for this
		 * indicator
		 * 1 = enabled, value changes may be sent for this indicator
		 */

		/* We don't support Enhanced Driver Status, so explicitly
		 * disable it (b/172680041).
		 */
		err = hfp_send(handle, AT_CMD("+BIND: 1,0"));
		if (err < 0)
			return err;

		BTLOG(btlog, BT_HFP_HF_INDICATOR, 0, 0);

		err = hfp_send(handle, AT_CMD("+BIND: 2,1"));
		if (err < 0)
			return err;

		err = hfp_send(handle, AT_CMD("OK"));
		if (err)
			return err;
		/*
		 * Consider the Service Level Connection to be fully initialized
		 * and thereby established, after successfully responded with OK
		 */
		initialize_slc_handle(NULL, (void *)handle);
		return 0;
	} else {
		goto error_out;
	}
	/* This OK reply is required after both +BIND AT commands. It also
	 * covers the AT+BIND= <a>,<b>,...,<n> case.
	 */
	return hfp_send(handle, AT_CMD("OK"));

error_out:
	syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
	return hfp_send(handle, AT_CMD("ERROR"));
}

/* AT+BIEV command reports updated values of enabled HF indicators to the AG.
 */
static int indicator_state_change(struct hfp_slc_handle *handle,
				  const char *cmd)
{
	char *tokens, *key, *val;
	int level;
	/* AT+BIEV= <assigned number>,<value> (Update value of indicator)
	 * CRAS only supports battery level, which is with assigned number 2.
	 * Battery level should range from 0 to 100 defined by the spec.
	 */
	tokens = strdup(cmd);
	strtok(tokens, "=");
	key = strtok(NULL, ",");
	if (!key)
		goto error_out;

	if (atoi(key) == 2) {
		val = strtok(NULL, ",");
		if (!val)
			goto error_out;
		level = atoi(val);
		if (level >= 0 && level <= 100) {
			cras_server_metrics_hfp_battery_report(
				CRAS_HFP_BATTERY_INDICATOR_HFP);
			if (handle->hf_battery != level) {
				handle->hf_battery = level;
				cras_observer_notify_bt_battery_changed(
					cras_bt_device_address(handle->device),
					(uint32_t)(level));
			}
		} else {
			syslog(LOG_ERR,
			       "Get invalid battery status from cmd:%s", cmd);
		}
	} else {
		goto error_out;
	}

	free(tokens);
	return hfp_send(handle, AT_CMD("OK"));

error_out:
	syslog(LOG_WARNING, "%s: invalid command: '%s'", __func__, cmd);
	free(tokens);
	return hfp_send(handle, AT_CMD("ERROR"));
}

/* AT+VGM and AT+VGS command reports the current mic and speaker gain
 * level respectively. Optional support per spec 4.28.
 */
static int signal_gain_setting(struct hfp_slc_handle *handle, const char *cmd)
{
	int gain;

	if (strlen(cmd) < 8) {
		syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
		return hfp_send(handle, AT_CMD("ERROR"));
	}

	/* Map 0 to the smallest non-zero scale 6/100, and 15 to
	 * 100/100 full. */
	if (cmd[5] == 'S') {
		gain = atoi(&cmd[7]);
		if (gain < 0 || gain > 15) {
			syslog(LOG_ERR,
			       "signal_gain_setting: gain %d is not between 0 and 15",
			       gain);
			return hfp_send(handle, AT_CMD("ERROR"));
		}
		BTLOG(btlog, BT_HFP_UPDATE_SPEAKER_GAIN, gain, 0);
		cras_bt_device_update_hardware_volume(handle->device,
						      (gain + 1) * 100 / 16);
	}

	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+CNUM command to query the subscriber number. Mandatory support
 * per spec 4.30.
 */
static int subscriber_number(struct hfp_slc_handle *handle, const char *buf)
{
	return hfp_send(handle, AT_CMD("OK"));
}

/* AT+BRSF command notifies the HF(Hands-free device) supported features
 * and retrieves the AG(Audio gateway) supported features. Mandatory
 * support per spec 4.2.
 */
static int supported_features(struct hfp_slc_handle *handle, const char *cmd)
{
	int err;
	char response[128];
	char *tokens, *features;

	if (strlen(cmd) < 9) {
		syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
		return hfp_send(handle, AT_CMD("ERROR"));
	}

	tokens = strdup(cmd);
	strtok(tokens, "=");
	features = strtok(NULL, ",");
	if (!features)
		goto error_out;

	handle->hf_supported_features = atoi(features);
	BTLOG(btlog, BT_HFP_SUPPORTED_FEATURES, 0,
	      handle->hf_supported_features);
	free(tokens);

	/* AT+BRSF=<feature> command received, ignore the HF supported feature
	 * for now. Respond with +BRSF:<feature> to notify mandatory supported
	 * features in AG(audio gateway).
	 */
	BTLOG(btlog, BT_HFP_SUPPORTED_FEATURES, 1,
	      handle->ag_supported_features);
	snprintf(response, 128, AT_CMD("+BRSF: %u"),
		 handle->ag_supported_features);
	err = hfp_send(handle, response);
	if (err < 0)
		return err;

	return hfp_send(handle, AT_CMD("OK"));

error_out:
	free(tokens);
	syslog(LOG_ERR, "%s: malformed command: '%s'", __func__, cmd);
	return hfp_send(handle, AT_CMD("ERROR"));
}

int hfp_event_speaker_gain(struct hfp_slc_handle *handle, int gain)
{
	char command[128];

	/* Normailize gain value to 0-15 */
	gain = gain * 15 / 100;
	BTLOG(btlog, BT_HFP_SET_SPEAKER_GAIN, gain, 0);
	snprintf(command, 128, AT_CMD("+VGS=%d"), gain);

	return hfp_send(handle, command);
}

/* AT+CHUP command to terminate current call. Mandatory support
 * per spec 4.15.
 */
static int terminate_call(struct hfp_slc_handle *handle, const char *cmd)
{
	int rc;
	rc = hfp_send(handle, AT_CMD("OK"));
	if (rc)
		return rc;

	return cras_telephony_event_terminate_call();
}

/* AT+XEVENT is defined by Android to support vendor specific features.
 * Currently, the only known supported case for CrOS is the battery event sent
 * by some Plantronics headsets.
 */
static int vendor_specific_features(struct hfp_slc_handle *handle,
				    const char *cmd)
{
	char *tokens, *event, *level_str, *num_of_level_str;
	int level, num_of_level;

	tokens = strdup(cmd);
	strtok(tokens, "=");
	event = strtok(NULL, ",");
	if (!event)
		goto error_out;

	/* AT+XEVENT=BATTERY,Level,NumberOfLevel,MinutesOfTalkTime,IsCharging
	 * Level: The charge level with a zero-based integer.
	 * NumberOfLevel: How many charging levels there are.
	 * MinuteOfTalkTime: The estimated number of talk minutes remaining.
	 * IsCharging: A 0 or 1 value.
	 *
	 * We only support the battery level and thus only care about the first
	 * 3 arguments.
	 */
	if (!strncmp(event, "BATTERY", 7)) {
		level_str = strtok(NULL, ",");
		num_of_level_str = strtok(NULL, ",");
		if (!level_str || !num_of_level_str)
			goto error_out;

		level = atoi(level_str);
		num_of_level = atoi(num_of_level_str);
		if (level < 0 || num_of_level <= 1 || level >= num_of_level)
			goto error_out;

		level = (int64_t)level * 100 / (num_of_level - 1);
		if (handle->hf_battery != level) {
			handle->hf_supports_battery_indicator |=
				CRAS_HFP_BATTERY_INDICATOR_PLANTRONICS;
			cras_server_metrics_hfp_battery_report(
				CRAS_HFP_BATTERY_INDICATOR_PLANTRONICS);
			handle->hf_battery = level;
			cras_observer_notify_bt_battery_changed(
				cras_bt_device_address(handle->device),
				(uint32_t)(level));
		}
	}

	free(tokens);
	/* For Plantronic headsets, it is required to reply "OK" for the first
	 * AT+XEVENT=USER-AGENT... command to tell the headset our support of
	 * the xevent protocol. Otherwise, all following events including
	 * BATTERY won't be sent.
	 */
	return hfp_send(handle, AT_CMD("OK"));

error_out:
	syslog(LOG_ERR, "%s: malformed vendor specific command: '%s'", __func__,
	       cmd);
	free(tokens);
	return hfp_send(handle, AT_CMD("ERROR"));
}

/* AT commands to support in order to conform HFP specification.
 *
 * An initialized service level connection is the pre-condition for all
 * call related procedures. Note that for the call related commands,
 * we are good to just respond with a meaningless "OK".
 *
 * The procedure to establish a service level connection is described below:
 *
 * 1. HF notifies AG about its own supported features and AG responds
 * with its supported feature.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                     AT+BRSF=<HF supported feature> -->
 *                 <-- +BRSF:<AG supported feature>
 *                 <-- OK
 *
 * 2. HF retrieves the information about the indicators supported in AG.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                     AT+CIND=? -->
 *                 <-- +CIND:...
 *                 <-- OK
 *
 * 3. The HF requests the current status of the indicators in AG.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                     AT+CIND -->
 *                 <-- +CIND:...
 *                 <-- OK
 *
 * 4. HF requests enabling indicator status update in the AG.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                     AT+CMER= -->
 *                 <-- OK
 */
static struct at_command at_commands[] = {
	{ "ATA", answer_call },
	{ "ATD", dial_number },
	{ "AT+BAC", available_codecs },
	{ "AT+BCC", bluetooth_codec_connection },
	{ "AT+BCS", bluetooth_codec_selection },
	{ "AT+BIA", indicator_activation },
	{ "AT+BIEV", indicator_state_change },
	{ "AT+BIND", indicator_support },
	{ "AT+BLDN", last_dialed_number },
	{ "AT+BRSF", supported_features },
	{ "AT+CCWA", call_waiting_notify },
	{ "AT+CHUP", terminate_call },
	{ "AT+CIND", report_indicators },
	{ "AT+CKPD", key_press },
	{ "AT+CLCC", list_current_calls },
	{ "AT+CLIP", cli_notification },
	{ "AT+CMEE", extended_errors },
	{ "AT+CMER", event_reporting },
	{ "AT+CNUM", subscriber_number },
	{ "AT+COPS", operator_selection },
	{ "AT+IPHONEACCEV", apple_accessory_state_change },
	{ "AT+VG", signal_gain_setting },
	{ "AT+VTS", dtmf_tone },
	{ "AT+XAPL", apple_supported_features },
	{ "AT+XEVENT", vendor_specific_features },
	{ "AT+CHLD", call_hold },
	{ 0 }
};

static int handle_at_command(struct hfp_slc_handle *slc_handle, const char *cmd)
{
	struct at_command *atc;

	for (atc = at_commands; atc->cmd; atc++)
		if (!strncmp(cmd, atc->cmd, strlen(atc->cmd)))
			return atc->callback(slc_handle, cmd);

	syslog(LOG_DEBUG, "AT command %s not supported", cmd);
	return hfp_send(slc_handle, AT_CMD("ERROR"));
}

int handle_at_command_for_test(struct hfp_slc_handle *slc_handle,
			       const char *cmd)
{
	return handle_at_command(slc_handle, cmd);
}

static int process_at_commands(struct hfp_slc_handle *handle)
{
	ssize_t bytes_read;
	int err;

	bytes_read =
		read(handle->rfcomm_fd, &handle->buf[handle->buf_write_idx],
		     SLC_BUF_SIZE_BYTES - handle->buf_write_idx - 1);
	if (bytes_read < 0)
		return bytes_read;

	handle->buf_write_idx += bytes_read;
	handle->buf[handle->buf_write_idx] = '\0';

	while (handle->buf_read_idx != handle->buf_write_idx) {
		char *end_char;
		end_char = strchr(&handle->buf[handle->buf_read_idx], '\r');
		if (end_char == NULL)
			break;

		*end_char = '\0';
		err = handle_at_command(handle,
					&handle->buf[handle->buf_read_idx]);
		if (err < 0)
			return 0;

		/* Shift the read index */
		handle->buf_read_idx = 1 + end_char - handle->buf;
		if (handle->buf_read_idx == handle->buf_write_idx) {
			handle->buf_read_idx = 0;
			handle->buf_write_idx = 0;
		}
	}

	/* Handle the case when buffer is full and no command found. */
	if (handle->buf_write_idx == SLC_BUF_SIZE_BYTES - 1) {
		if (handle->buf_read_idx) {
			memmove(handle->buf, &handle->buf[handle->buf_read_idx],
				handle->buf_write_idx - handle->buf_read_idx);
			handle->buf_write_idx -= handle->buf_read_idx;
			handle->buf_read_idx = 0;
		} else {
			syslog(LOG_ERR,
			       "Parse SLC command error, clean up buffer");
			handle->buf_write_idx = 0;
		}
	}
	return bytes_read;
}

static void slc_watch_callback(void *arg, int revents)
{
	struct hfp_slc_handle *handle = (struct hfp_slc_handle *)arg;
	int err;

	err = process_at_commands(handle);
	if (err < 0) {
		syslog(LOG_ERR, "Error reading slc command %s",
		       strerror(errno));
		cras_system_rm_select_fd(handle->rfcomm_fd);
		handle->disconnect_cb(handle);
	}
	return;
}

/* Exported interfaces */

struct hfp_slc_handle *hfp_slc_create(int fd, int is_hsp,
				      int ag_supported_features,
				      struct cras_bt_device *device,
				      hfp_slc_init_cb init_cb,
				      hfp_slc_disconnect_cb disconnect_cb)
{
	struct hfp_slc_handle *handle;
	int i;

	if (!disconnect_cb)
		return NULL;

	handle = (struct hfp_slc_handle *)calloc(1, sizeof(*handle));
	if (!handle)
		return NULL;

	handle->rfcomm_fd = fd;
	handle->is_hsp = is_hsp;
	handle->ag_supported_features = ag_supported_features;
	handle->hf_supported_features = 0;
	handle->device = device;
	handle->init_cb = init_cb;
	handle->disconnect_cb = disconnect_cb;
	handle->cli_active = 0;
	handle->battery = 5;
	handle->signal = 5;
	handle->service = 1;
	handle->ind_event_reports[CRAS_INDICATOR_ENABLE_INDEX] = 0;
	for (i = BATTERY_IND_INDEX; i < INDICATOR_IND_MAX; i++)
		handle->ind_event_reports[i] = 1;
	handle->telephony = cras_telephony_get();
	handle->preferred_codec = HFP_CODEC_ID_CVSD;
	handle->selected_codec = HFP_CODEC_UNUSED;
	handle->hf_supports_battery_indicator = CRAS_HFP_BATTERY_INDICATOR_NONE;
	handle->hf_battery = -1;
	cras_system_add_select_fd(handle->rfcomm_fd, slc_watch_callback, handle,
				  POLLIN | POLLERR | POLLHUP);

	return handle;
}

void hfp_slc_destroy(struct hfp_slc_handle *slc_handle)
{
	cras_system_rm_select_fd(slc_handle->rfcomm_fd);
	if (slc_handle->timer)
		cras_tm_cancel_timer(cras_system_state_get_tm(),
				     slc_handle->timer);
	close(slc_handle->rfcomm_fd);
	free(slc_handle);
}

int hfp_slc_is_hsp(struct hfp_slc_handle *handle)
{
	return handle->is_hsp;
}

int hfp_slc_get_selected_codec(struct hfp_slc_handle *handle)
{
	/* If codec negotiation is not supported on HF, or the negotiation
	 * process never completed. Fallback to the preffered codec. */
	if (handle->selected_codec == HFP_CODEC_UNUSED)
		return handle->preferred_codec;
	else
		return handle->selected_codec;
}

int hfp_slc_codec_connection_setup(struct hfp_slc_handle *handle)
{
	/* The time we wait for codec selection response. */
	static struct timespec timeout = { 0, 100000000 };
	struct pollfd poll_fd;
	int rc = 0;
	struct timespec ts = timeout;

	/*
	 * Codec negotiation is not required, if either AG or HF doesn't support
	 * it or it has been done once.
	 */
	if (!hfp_slc_get_hf_codec_negotiation_supported(handle) ||
	    !hfp_slc_get_ag_codec_negotiation_supported(handle) ||
	    handle->selected_codec == handle->preferred_codec)
		return 0;

redo_codec_conn:
	select_preferred_codec(handle);

	poll_fd.fd = handle->rfcomm_fd;
	poll_fd.events = POLLIN;

	ts = timeout;
	while (rc <= 0) {
		rc = cras_poll(&poll_fd, 1, &ts, NULL);
		if (rc == -ETIMEDOUT) {
			/*
	 		 * Catch the case that the first initial codec
			 * negotiation timeout. At this point we're not sure
			 * if HF is good with the preferred codec from AG.
			 * Fallback to CVSD doesn't help because very likely
			 * HF won't reply that either. The best thing we can
			 * do is just leave a warning log.
	 		 */
			if (handle->selected_codec == HFP_CODEC_UNUSED) {
				syslog(LOG_WARNING,
				       "Proceed using codec %d without HF reply",
				       handle->preferred_codec);
			}
			return rc;
		}
	}

	if (rc > 0) {
		do {
			usleep(CODEC_CONN_SLEEP_TIME_US);
			rc = process_at_commands(handle);
		} while (rc == -EAGAIN);

		if (rc <= 0)
			return rc;
		if (handle->selected_codec != handle->preferred_codec)
			goto redo_codec_conn;
	}

	return 0;
}

int hfp_set_call_status(struct hfp_slc_handle *handle, int call)
{
	int old_call = handle->telephony->call;

	if (old_call == call)
		return 0;

	handle->telephony->call = call;
	return hfp_event_update_call(handle);
}

/* Procedure to setup a call when AG sees incoming call.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                                                     <-- Incoming call
 *                 <-- +CIEV: (callsetup = 1)
 *                 <-- RING (ALERT)
 */
int hfp_event_incoming_call(struct hfp_slc_handle *handle, const char *number,
			    int type)
{
	int rc;

	if (handle->is_hsp)
		return 0;

	if (handle->cli_active) {
		rc = hfp_send_calling_line_identification(handle, number, type);
		if (rc)
			return rc;
	}

	if (handle->telephony->call)
		return 0;
	else
		return hfp_send(handle, AT_CMD("RING"));
}

int hfp_event_update_call(struct hfp_slc_handle *handle)
{
	return hfp_send_ind_event_report(handle, CALL_IND_INDEX,
					 handle->telephony->call);
}

int hfp_event_update_callsetup(struct hfp_slc_handle *handle)
{
	return hfp_send_ind_event_report(handle, CALLSETUP_IND_INDEX,
					 handle->telephony->callsetup);
}

int hfp_event_update_callheld(struct hfp_slc_handle *handle)
{
	return hfp_send_ind_event_report(handle, CALLHELD_IND_INDEX,
					 handle->telephony->callheld);
}

int hfp_event_set_battery(struct hfp_slc_handle *handle, int level)
{
	handle->battery = level;
	return hfp_send_ind_event_report(handle, BATTERY_IND_INDEX, level);
}

int hfp_event_set_signal(struct hfp_slc_handle *handle, int level)
{
	handle->signal = level;
	return hfp_send_ind_event_report(handle, SIGNAL_IND_INDEX, level);
}

int hfp_event_set_service(struct hfp_slc_handle *handle, int avail)
{
	/* Convert to 0 or 1.
	 * Since the value must be either 1 or 0. (service presence or not) */
	handle->service = !!avail;
	return hfp_send_ind_event_report(handle, SERVICE_IND_INDEX, avail);
}

int hfp_slc_get_ag_codec_negotiation_supported(struct hfp_slc_handle *handle)
{
	return handle->ag_supported_features & AG_CODEC_NEGOTIATION;
}

int hfp_slc_get_hf_codec_negotiation_supported(struct hfp_slc_handle *handle)
{
	return handle->hf_supported_features & HF_CODEC_NEGOTIATION;
}

int hfp_slc_get_hf_hf_indicators_supported(struct hfp_slc_handle *handle)
{
	return handle->hf_supported_features & HF_HF_INDICATORS;
}

bool hfp_slc_get_wideband_speech_supported(struct hfp_slc_handle *handle)
{
	return hfp_slc_get_ag_codec_negotiation_supported(handle) &&
	       hfp_slc_get_hf_codec_negotiation_supported(handle) &&
	       handle->hf_codec_supported[HFP_CODEC_ID_MSBC];
}

int hfp_slc_get_hf_supports_battery_indicator(struct hfp_slc_handle *handle)
{
	return handle->hf_supports_battery_indicator;
}
