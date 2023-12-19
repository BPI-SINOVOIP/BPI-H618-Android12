/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <syslog.h>

#include "cras_iodev_list.h"
#include "cras_messages.h"
#include "cras_observer.h"
#include "cras_rclient.h"
#include "cras_rclient_util.h"
#include "cras_rstream.h"
#include "cras_types.h"
#include "cras_util.h"

/* Declarations of cras_rclient operators for cras_unified_rclient. */
static const struct cras_rclient_ops cras_unified_rclient_ops = {
	.handle_message_from_client = rclient_handle_message_from_client,
	.send_message_to_client = rclient_send_message_to_client,
	.destroy = rclient_destroy,
};

/*
 * Exported Functions.
 */

/* Creates a client structure and sends a message back informing the client that
 * the connection has succeeded. */
struct cras_rclient *cras_unified_rclient_create(int fd, size_t id)
{
	int supported_directions =
		cras_stream_direction_mask(CRAS_STREAM_OUTPUT) |
		cras_stream_direction_mask(CRAS_STREAM_INPUT);
	return rclient_generic_create(fd, id, &cras_unified_rclient_ops,
				      supported_directions);
}
