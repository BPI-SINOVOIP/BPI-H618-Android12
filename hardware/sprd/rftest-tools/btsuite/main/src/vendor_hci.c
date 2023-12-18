/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <assert.h>
#include <cutils/properties.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

#include "bt_types.h"

#include "vendor_h4.h"
#include "vendor_hci.h"
#include "vendor_utils.h"
#include "hcidefs.h"
#include "vendor_hcif.h"

#include "buffer_allocator.h"

#include "osi/include/eager_reader.h"
#include "osi/include/osi.h"
#include "osi/include/log.h"
#include "osi/include/reactor.h"
#include "osi/include/thread.h"
#include "osi/include/future.h"
#include "osi/include/list.h"
#include "osi/include/fixed_queue.h"

static thread_t *thread; // We own this
const allocator_t *buffer_allocator;
static int command_credits = 1;
static list_t *commands_pending_response;
static pthread_mutex_t commands_pending_response_lock;
static fixed_queue_t *command_queue;

#define INBOUND_PACKET_TYPE_COUNT 3
#define PACKET_TYPE_TO_INBOUND_INDEX(type) ((type) - 2)
#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)

#define PREAMBLE_BUFFER_SIZE 4 // max preamble size, ACL
#define RETRIEVE_ACL_LENGTH(preamble) ((((preamble)[3]) << 8) | (preamble)[2])

static const uint8_t preamble_sizes[] = {
  HCI_COMMAND_PREAMBLE_SIZE,
  HCI_ACL_PREAMBLE_SIZE,
  HCI_SCO_PREAMBLE_SIZE,
  HCI_EVENT_PREAMBLE_SIZE
};

static const uint16_t outbound_event_types[] =
{
  MSG_HC_TO_STACK_HCI_ERR,
  MSG_HC_TO_STACK_HCI_ACL,
  MSG_HC_TO_STACK_HCI_SCO,
  MSG_HC_TO_STACK_HCI_EVT
};

typedef enum {
  BRAND_NEW,
  PREAMBLE,
  BODY,
  IGNORE,
  FINISHED
} receive_state_t;

typedef struct {
  receive_state_t state;
  uint16_t bytes_remaining;
  uint8_t preamble[PREAMBLE_BUFFER_SIZE];
  uint16_t index;
  BT_HDR *buffer;
} packet_receive_data_t;

typedef struct {
  uint16_t opcode;
  future_t *complete_future;
  command_complete_cb complete_callback;
  command_status_cb status_callback;
  void *context;
  BT_HDR *command;
} waiting_command_t;

static packet_receive_data_t incoming_packets[INBOUND_PACKET_TYPE_COUNT];

static waiting_command_t *get_waiting_command(command_opcode_t opcode) {
  pthread_mutex_lock(&commands_pending_response_lock);

  for (const list_node_t *node = list_begin(commands_pending_response);
      node != list_end(commands_pending_response);
      node = list_next(node)) {
    waiting_command_t *wait_entry = list_node(node);

    if (!wait_entry || wait_entry->opcode != opcode)
      continue;

    list_remove(commands_pending_response, wait_entry);

    pthread_mutex_unlock(&commands_pending_response_lock);
    return wait_entry;
  }

  pthread_mutex_unlock(&commands_pending_response_lock);
  return NULL;
}

static uint8_t filter_incoming_event(BT_HDR *packet) {
  waiting_command_t *wait_entry = NULL;
  uint8_t *stream = packet->data;
  uint8_t event_code;
  command_opcode_t opcode;

  STREAM_TO_UINT8(event_code, stream);
  STREAM_SKIP_UINT8(stream); // Skip the parameter total length field

  if (event_code == HCI_COMMAND_COMPLETE_EVT) {
    STREAM_TO_UINT8(command_credits, stream);
    STREAM_TO_UINT16(opcode, stream);

    wait_entry = get_waiting_command(opcode);
    if (!wait_entry)
      LOG_WARN("%s command complete event with no matching command. opcode: 0x%x.", __func__, opcode);
    else if (wait_entry->complete_callback) {
      wait_entry->complete_callback(packet, wait_entry->context);
      BTD("complete");
    }
    else if (wait_entry->complete_future)
      future_ready(wait_entry->complete_future, packet);

    goto intercepted;
  } else if (event_code == HCI_COMMAND_STATUS_EVT) {
    uint8_t status;
    STREAM_TO_UINT8(status, stream);
    STREAM_TO_UINT8(command_credits, stream);
    STREAM_TO_UINT16(opcode, stream);

    // If a command generates a command status event, it won't be getting a command complete event

    wait_entry = get_waiting_command(opcode);
    if (!wait_entry)
      LOG_WARN("%s command status event with no matching command. opcode: 0x%x", __func__, opcode);
    else if (wait_entry->status_callback)
      wait_entry->status_callback(status, wait_entry->command, wait_entry->context);

    goto intercepted;
  }

  return false;
intercepted:;
  //non_repeating_timer_restart_if(command_response_timer, !list_is_empty(commands_pending_response));

  if (wait_entry) {
    // If it has a callback, it's responsible for freeing the packet
    if (event_code == HCI_COMMAND_STATUS_EVT || (!wait_entry->complete_callback && !wait_entry->complete_future))
      buffer_allocator->free(packet);

    // If it has a callback, it's responsible for freeing the command
    if (event_code == HCI_COMMAND_COMPLETE_EVT || !wait_entry->status_callback)
      buffer_allocator->free(wait_entry->command);

    osi_free(wait_entry);
  } else {
    buffer_allocator->free(packet);
  }

  return true;
}

void transmit_command(
    BT_HDR *command,
    command_complete_cb complete_callback,
    command_status_cb status_callback,
    void *context) {
  waiting_command_t *wait_entry = osi_calloc(sizeof(waiting_command_t));
  if (!wait_entry) {
    LOG_ERROR("%s couldn't allocate space for wait entry.", __func__);
    return;
  }

  uint8_t *stream = command->data + command->offset;
  STREAM_TO_UINT16(wait_entry->opcode, stream);
  wait_entry->complete_callback = complete_callback;
  wait_entry->status_callback = status_callback;
  wait_entry->command = command;
  wait_entry->context = context;

  BTD("Send Cmd: 0x%04X", wait_entry->opcode);

  // Store the command message type in the event field
  // in case the upper layer didn't already
  command->event = MSG_STACK_TO_HC_HCI_CMD;

  fixed_queue_enqueue(command_queue, wait_entry);
}
static void hal_says_data_ready(serial_data_type_t type) {
  packet_receive_data_t *incoming = &incoming_packets[PACKET_TYPE_TO_INBOUND_INDEX(type)];

  uint8_t byte;
  while (read_data(type, &byte, 1, false) != 0) {
    switch (incoming->state) {
      case BRAND_NEW:
        // Initialize and prepare to jump to the preamble reading state
        incoming->bytes_remaining = preamble_sizes[PACKET_TYPE_TO_INDEX(type)];
        memset(incoming->preamble, 0, PREAMBLE_BUFFER_SIZE);
        incoming->index = 0;
        incoming->state = PREAMBLE;
        // INTENTIONAL FALLTHROUGH
      case PREAMBLE:
        incoming->preamble[incoming->index] = byte;
        incoming->index++;
        incoming->bytes_remaining--;

        if (incoming->bytes_remaining == 0) {
          // For event and sco preambles, the last byte we read is the length
          incoming->bytes_remaining = (type == DATA_TYPE_ACL) ? RETRIEVE_ACL_LENGTH(incoming->preamble) : byte;

          size_t buffer_size = BT_HDR_SIZE + incoming->index + incoming->bytes_remaining;
          incoming->buffer = (BT_HDR *)buffer_allocator->alloc(buffer_size);

          if (!incoming->buffer) {
            LOG_ERROR("%s error getting buffer for incoming packet of type %d and size %zd", __func__, type, buffer_size);
            // Can't read any more of this current packet, so jump out
            incoming->state = incoming->bytes_remaining == 0 ? BRAND_NEW : IGNORE;
            break;
          }

          // Initialize the buffer
          incoming->buffer->offset = 0;
          incoming->buffer->layer_specific = 0;
          incoming->buffer->event = outbound_event_types[PACKET_TYPE_TO_INDEX(type)];
          memcpy(incoming->buffer->data, incoming->preamble, incoming->index);

          incoming->state = incoming->bytes_remaining > 0 ? BODY : FINISHED;
        }

        break;
      case BODY:
        incoming->buffer->data[incoming->index] = byte;
        incoming->index++;
        incoming->bytes_remaining--;

        size_t bytes_read = read_data(type, (incoming->buffer->data + incoming->index), incoming->bytes_remaining, false);
        incoming->index += bytes_read;
        incoming->bytes_remaining -= bytes_read;

        incoming->state = incoming->bytes_remaining == 0 ? FINISHED : incoming->state;
        break;
      case IGNORE:
        incoming->bytes_remaining--;
        if (incoming->bytes_remaining == 0) {
          incoming->state = BRAND_NEW;
          // Don't forget to let the hal know we finished the packet we were ignoring.
          // Otherwise we'll get out of sync with hals that embed extra information
          // in the uart stream (like H4). #badnewsbears
          packet_finished(type);
          return;
        }

        break;
      case FINISHED:
        LOG_ERROR("%s the state machine should not have been left in the finished state.", __func__);
        break;
    }

    if (incoming->state == FINISHED) {
      incoming->buffer->len = incoming->index;

      //btsnoop->capture(incoming->buffer, true);

      if (type != DATA_TYPE_EVENT) {
        //packet_fragmenter->reassemble_and_dispatch(incoming->buffer);
      } else if (!filter_incoming_event(incoming->buffer)) {
            BT_HDR  *stream = (incoming->buffer);
            uint8_t *stream_evt = incoming->buffer->data;
            uint8_t event_code;
            STREAM_TO_UINT8(event_code, stream_evt);
            btu_hcif_process_event (event_code, (BT_HDR*)stream);
      }

      // We don't control the buffer anymore
      incoming->buffer = NULL;
      incoming->state = BRAND_NEW;
      packet_finished(type);

      // We return after a packet is finished for two reasons:
      // 1. The type of the next packet could be different.
      // 2. We don't want to hog cpu time.
      return;
    }
  }
}

static const hci_h4_callbacks_t h4_callbacks = {
  hal_says_data_ready
};

static serial_data_type_t event_to_data_type(uint16_t event) {
  if (event == MSG_STACK_TO_HC_HCI_ACL)
    return DATA_TYPE_ACL;
  else if (event == MSG_STACK_TO_HC_HCI_SCO)
    return DATA_TYPE_SCO;
  else if (event == MSG_STACK_TO_HC_HCI_CMD)
    return DATA_TYPE_COMMAND;
  else
    LOG_ERROR("%s invalid event type, could not translate 0x%x", __func__, event);

  return 0;
}

static void transmit_fragment(BT_HDR *packet, bool send_transmit_finished) {
  uint16_t event = packet->event & MSG_EVT_MASK;
  serial_data_type_t type = event_to_data_type(event);

  transmit_data(type, packet->data + packet->offset, packet->len);

  if (event != MSG_STACK_TO_HC_HCI_CMD && send_transmit_finished)
    buffer_allocator->free(packet);
}

static void fragmenter_transmit_finished(BT_HDR *packet, bool all_fragments_sent) {
  if (all_fragments_sent) {
    buffer_allocator->free(packet);
  } else {
    // This is kind of a weird case, since we're dispatching a partially sent packet
    // up to a higher layer.
    // TODO(zachoverflow): rework upper layer so this isn't necessary.
    //data_dispatcher_dispatch(interface.event_dispatcher, packet->event & MSG_EVT_MASK, packet);
  }
}

static void fragment_and_dispatch(BT_HDR *packet) {
  assert(packet != NULL);

  uint16_t event = packet->event & MSG_EVT_MASK;
//  uint8_t *stream = packet->data + packet->offset;

  // We only fragment ACL packets
  if (event != MSG_STACK_TO_HC_HCI_ACL) {
    transmit_fragment(packet, true);
    return;
  }

#if 0
  uint16_t max_data_size =
    SUB_EVENT(packet->event) == LOCAL_BR_EDR_CONTROLLER_ID ?
      controller->get_acl_data_size_classic() :
      controller->get_acl_data_size_ble();

  uint16_t max_packet_size = max_data_size + HCI_ACL_PREAMBLE_SIZE;
  uint16_t remaining_length = packet->len;

  uint16_t continuation_handle;
  STREAM_TO_UINT16(continuation_handle, stream);
  continuation_handle = APPLY_CONTINUATION_FLAG(continuation_handle);

  while (remaining_length > max_packet_size) {
    // Make sure we use the right ACL packet size
    stream = packet->data + packet->offset;
    STREAM_SKIP_UINT16(stream);
    UINT16_TO_STREAM(stream, max_data_size);

    packet->len = max_packet_size;
    transmit_fragment(packet, false);

    packet->offset += max_data_size;
    remaining_length -= max_data_size;
    packet->len = remaining_length;

    // Write the ACL header for the next fragment
    stream = packet->data + packet->offset;
    UINT16_TO_STREAM(stream, continuation_handle);
    UINT16_TO_STREAM(stream, remaining_length - HCI_ACL_PREAMBLE_SIZE);

    // Apparently L2CAP can set layer_specific to a max number of segments to transmit
    if (packet->layer_specific) {
      packet->layer_specific--;

      if (packet->layer_specific == 0) {
        packet->event = MSG_HC_TO_STACK_L2C_SEG_XMIT;
        fragmenter_transmit_finished(packet, false);
        return;
      }
    }
  }
#endif
  fragmenter_transmit_finished(packet, true);
}

static void event_command_ready(fixed_queue_t *queue, UNUSED_ATTR void *context) {
  if (command_credits > 0) {
    waiting_command_t *wait_entry = fixed_queue_dequeue(queue);
    command_credits--;

    // Move it to the list of commands awaiting response
    pthread_mutex_lock(&commands_pending_response_lock);
    list_append(commands_pending_response, wait_entry);
    pthread_mutex_unlock(&commands_pending_response_lock);

    // Send it off
    lmp_assert();
    fragment_and_dispatch(wait_entry->command);
    lmp_deassert();

    //non_repeating_timer_restart_if(command_response_timer, !list_is_empty(commands_pending_response));
  }
}

int hci_start_up(void) {
    thread = thread_new("hci_thread");
    if (!thread) {
        BTD("%s unable to create thread.", __func__);
        goto error;
    }

  command_credits = 1;
  pthread_mutex_init(&commands_pending_response_lock, NULL);


    command_queue = fixed_queue_new(SIZE_MAX);
    if (!command_queue) {
        LOG_ERROR("%s unable to create pending command queue.", __func__);
        goto error;
    }


    commands_pending_response = list_new(NULL);
    if (!commands_pending_response) {
        LOG_ERROR("%s unable to create list for commands pending response.", __func__);
        goto error;
    }

    buffer_allocator = buffer_allocator_get_interface();


    fixed_queue_register_dequeue(command_queue, thread_get_reactor(thread), event_command_ready, NULL);


    h4_start_up(&h4_callbacks, thread);

    return 0;

error:;

    return -1;
}

void hci_shut_down(void) {
    BTD("%s+++", __func__);

    h4_shut_down();

    fixed_queue_free(command_queue, osi_free);
    list_free(commands_pending_response);

    pthread_mutex_destroy(&commands_pending_response_lock);

    BTD("%s thread_free thread", __func__);
    thread_free(thread);
    thread = NULL;

    BTD("%s---", __func__);
}

