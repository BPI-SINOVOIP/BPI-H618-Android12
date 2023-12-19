/*
 * Copyright © 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "wayland-client.h"
#include "wayland-server.h"
#include "wayland-util.h"
#include "test-runner.h"

/* Ensure the connection doesn't fail due to lack of XDG_RUNTIME_DIR. */
static const char *
require_xdg_runtime_dir(void)
{
	char *val = getenv("XDG_RUNTIME_DIR");
	assert(val && "set $XDG_RUNTIME_DIR to run this test");

	return val;
}

struct compositor {
	struct wl_display *display;
	struct wl_event_loop *loop;
	int message;
	struct wl_client *client;
};

struct client {
	struct wl_display *display;
	struct wl_callback *cb;
        int message;
};

struct message {
	enum wl_protocol_logger_type type;
	const char *class;
	int opcode;
	const char *message_name;
	int args_count;
} messages[] = {
	{
		.type = WL_PROTOCOL_LOGGER_REQUEST,
		.class = "wl_display",
		.opcode = 0,
		.message_name = "sync",
		.args_count = 1,
	},
	{
		.type = WL_PROTOCOL_LOGGER_EVENT,
		.class = "wl_callback",
		.opcode = 0,
		.message_name = "done",
		.args_count = 1,
	},
	{
		.type = WL_PROTOCOL_LOGGER_EVENT,
		.class = "wl_display",
		.opcode = 1,
		.message_name = "delete_id",
		.args_count = 1,
	},
};

struct client_message {
	enum wl_protocol_logger_client_type type;
	const char *class;
	int opcode;
	const char *message_name;
	int args_count;
} client_messages[] = {
	{
		.type = WL_PROTOCOL_LOGGER_CLIENT_REQUEST,
		.class = "wl_display",
		.opcode = 0,
		.message_name = "sync",
		.args_count = 1,
	},
	{
		.type = WL_PROTOCOL_LOGGER_CLIENT_EVENT,
		.class = "wl_display",
		.opcode = 1,
		.message_name = "delete_id",
		.args_count = 1,
	},
	{
		.type = WL_PROTOCOL_LOGGER_CLIENT_EVENT,
		.class = "wl_callback",
		.opcode = 0,
		.message_name = "done",
		.args_count = 1,
	},
};

static void
logger_func(void *user_data, enum wl_protocol_logger_type type,
	    const struct wl_protocol_logger_message *message)
{
	struct compositor *c = user_data;
	struct message *msg = &messages[c->message++];

	assert(msg->type == type);
	assert(strcmp(msg->class, wl_resource_get_class(message->resource)) == 0);
	assert(msg->opcode == message->message_opcode);
	assert(strcmp(msg->message_name, message->message->name) == 0);
	assert(msg->args_count == message->arguments_count);

	c->client = wl_resource_get_client(message->resource);
}

static void
client_logger_func(void *user_data, enum wl_protocol_logger_client_type type,
		const struct wl_protocol_logger_client_message *message)
{
	struct client *c = user_data;
	struct client_message *msg = &client_messages[c->message++];

	assert(msg->type == type);
	assert(strcmp(msg->class, wl_proxy_get_class(message->proxy)) == 0);
	assert(msg->opcode == message->message_opcode);
	assert(strcmp(msg->message_name, message->message->name) == 0);
	assert(msg->args_count == message->arguments_count);
}

// A slightly simplified version of  get_next_argument() from src/connection.c
static const char*
get_next_argument_type(const char *signature, char* type)
{
	for (; *signature; ++signature) {
		assert(strchr("iufsonah?", *signature) != NULL);
		switch (*signature) {
		case 'i':
		case 'u':
		case 'f':
		case 's':
		case 'o':
		case 'n':
		case 'a':
		case 'h':
			*type = *signature;
			return signature + 1;
		case '?':
			break;

		}
	}
	*type = 0;
	return signature;
}

// This duplicates what the internal wl_closure_print function does, and can be
// used as a starting point for a client or server that wants to log messages.
static void
client_log_to_stderr_demo(void *user_data,
			  enum wl_protocol_logger_client_type type,
			  const struct wl_protocol_logger_client_message *message) {
	int i;
	char arg_type;
	const char *signature = message->message->signature;
	const union wl_argument* args = message->arguments;
	struct wl_proxy* arg_proxy;
	const char* arg_class;
	struct timespec tp;
	unsigned int time;

	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);

	// Note: server logger will be given message->resource, and should
	// use wl_resource_get_class and wl_resolurce_get_id.
	fprintf(stderr, "[%10.3f] %s%s@%u.%s(",
		time / 1000.0,
		(type == WL_PROTOCOL_LOGGER_CLIENT_REQUEST) ? " -> " : "",
		wl_proxy_get_class(message->proxy), wl_proxy_get_id(message->proxy),
		message->message->name);

	for (i = 0; i < message->arguments_count; i++) {
		signature = get_next_argument_type(signature, &arg_type);
		if (i > 0)
			fprintf(stderr, ", ");

		switch (arg_type) {
		case 'u':
			fprintf(stderr, "%u", args[i].u);
			break;
		case 'i':
			fprintf(stderr, "%d", args[i].i);
			break;
		case 'f':
			fprintf(stderr, "%f", wl_fixed_to_double(args[i].f));
			break;
		case 's':
			if (args[i].s)
				fprintf(stderr, "\"%s\"", args[i].s);
			else
				fprintf(stderr, "nil");
			break;
		case 'o':
			if (args[i].o) {
				// Note: server logger should instead use
				// wl_resource_from_object, and then
				// wl_resource_get_class and wl_resource_get_id.
				arg_proxy = wl_proxy_from_object(args[i].o);
				arg_class = wl_proxy_get_class(arg_proxy);

				fprintf(stderr, "%s@%u",
					arg_class ? arg_class : "[unknown]",
					wl_proxy_get_id(arg_proxy));
			} else {
				fprintf(stderr, "nil");
			}
			break;
		case 'n':
			fprintf(stderr, "new id %s@",
				  (message->message->types[i]) ?
				   message->message->types[i]->name :
				    "[unknown]");
			if (args[i].n != 0)
				fprintf(stderr, "%u", args[i].n);
			else
				fprintf(stderr, "nil");
			break;
		case 'a':
			fprintf(stderr, "array");
			break;
		case 'h':
			fprintf(stderr, "fd %d", args[i].h);
			break;
		}
	}

	fprintf(stderr, ")\n");
}

static void
callback_done(void *data, struct wl_callback *cb, uint32_t time)
{
	wl_callback_destroy(cb);
}

static const struct wl_callback_listener callback_listener = {
	callback_done,
};

TEST(logger)
{
	test_set_timeout(1);

	const char *socket;
	struct compositor compositor = { 0 };
	struct client client = { 0 };
	struct wl_protocol_logger *logger;
	struct wl_protocol_logger_client *logger_client;
	struct wl_protocol_logger_client *logger_client_demo;

	require_xdg_runtime_dir();

	compositor.display = wl_display_create();
	compositor.loop = wl_display_get_event_loop(compositor.display);
	socket = wl_display_add_socket_auto(compositor.display);

	logger = wl_display_add_protocol_logger(compositor.display,
						logger_func, &compositor);

	client.display = wl_display_connect(socket);
	logger_client = wl_display_add_protocol_logger_client(
		client.display, client_logger_func, &client);
	logger_client_demo = wl_display_add_protocol_logger_client(
		client.display, client_log_to_stderr_demo, &client);
	client.cb = wl_display_sync(client.display);
	wl_callback_add_listener(client.cb, &callback_listener, NULL);
	wl_display_flush(client.display);

	while (compositor.message < 3) {
		wl_event_loop_dispatch(compositor.loop, -1);
		wl_display_flush_clients(compositor.display);
	}

	wl_display_dispatch(client.display);
	wl_display_disconnect(client.display);

	wl_protocol_logger_client_destroy(logger_client);
	wl_protocol_logger_client_destroy(logger_client_demo);
	wl_client_destroy(compositor.client);
	wl_protocol_logger_destroy(logger);
	wl_display_destroy(compositor.display);
}
