/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sysdeps.h"

#include "client/usb.h"

#include <stdint.h>
#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <libusb/libusb.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/thread_annotations.h>

#include "adb.h"
#include "adb_utils.h"
#include "fdevent/fdevent.h"
#include "transfer_id.h"
#include "transport.h"

using android::base::ScopedLockAssertion;
using android::base::StringPrintf;

// RAII wrappers for libusb.
struct ConfigDescriptorDeleter {
    void operator()(libusb_config_descriptor* desc) { libusb_free_config_descriptor(desc); }
};

using unique_config_descriptor = std::unique_ptr<libusb_config_descriptor, ConfigDescriptorDeleter>;

struct DeviceDeleter {
    void operator()(libusb_device* d) { libusb_unref_device(d); }
};

using unique_device = std::unique_ptr<libusb_device, DeviceDeleter>;

struct DeviceHandleDeleter {
    void operator()(libusb_device_handle* h) { libusb_close(h); }
};

using unique_device_handle = std::unique_ptr<libusb_device_handle, DeviceHandleDeleter>;

static void process_device(libusb_device* device_raw);

static std::string get_device_address(libusb_device* device) {
    return StringPrintf("usb:%d:%d", libusb_get_bus_number(device),
                        libusb_get_device_address(device));
}

#if defined(__linux__)
static std::string get_device_serial_path(libusb_device* device) {
    uint8_t ports[7];
    int port_count = libusb_get_port_numbers(device, ports, 7);
    if (port_count < 0) return "";

    std::string path =
            StringPrintf("/sys/bus/usb/devices/%d-%d", libusb_get_bus_number(device), ports[0]);
    for (int port = 1; port < port_count; ++port) {
        path += StringPrintf(".%d", ports[port]);
    }
    path += "/serial";
    return path;
}
#endif

static bool endpoint_is_output(uint8_t endpoint) {
    return (endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
}

static bool should_perform_zero_transfer(size_t write_length, uint16_t zero_mask) {
    return write_length != 0 && zero_mask != 0 && (write_length & zero_mask) == 0;
}

struct LibusbConnection : public Connection {
    struct ReadBlock {
        LibusbConnection* self = nullptr;
        libusb_transfer* transfer = nullptr;
        Block block;
        bool active = false;
    };

    struct WriteBlock {
        LibusbConnection* self;
        libusb_transfer* transfer;
        Block block;
        TransferId id;
    };

    explicit LibusbConnection(unique_device device)
        : device_(std::move(device)), device_address_(get_device_address(device_.get())) {}

    ~LibusbConnection() { Stop(); }

    void HandlePacket(amessage& msg, std::optional<Block> payload) {
        auto packet = std::make_unique<apacket>();
        packet->msg = msg;
        if (payload) {
            packet->payload = std::move(*payload);
        }
        read_callback_(this, std::move(packet));
    }

    void Cleanup(ReadBlock* read_block) REQUIRES(read_mutex_) {
        libusb_free_transfer(read_block->transfer);
        read_block->active = false;
        read_block->transfer = nullptr;
        if (terminating_) {
            destruction_cv_.notify_one();
        }
    }

    bool MaybeCleanup(ReadBlock* read_block) REQUIRES(read_mutex_) {
        if (read_block->transfer->status == LIBUSB_TRANSFER_CANCELLED) {
            CHECK(terminating_);
        }

        if (terminating_) {
            Cleanup(read_block);
            return true;
        }

        return false;
    }

    static void LIBUSB_CALL header_read_cb(libusb_transfer* transfer) {
        auto read_block = static_cast<ReadBlock*>(transfer->user_data);
        auto self = read_block->self;

        std::lock_guard<std::mutex> lock(self->read_mutex_);
        CHECK_EQ(read_block, &self->header_read_);
        if (self->MaybeCleanup(read_block)) {
            return;
        }

        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
            std::string msg = StringPrintf("usb read failed: status = %d", transfer->status);
            LOG(ERROR) << msg;
            self->OnError(msg);
            self->Cleanup(read_block);
            return;
        }

        if (transfer->actual_length != sizeof(amessage)) {
            std::string msg = StringPrintf("usb read: invalid length for header: %d",
                                           transfer->actual_length);
            LOG(ERROR) << msg;
            self->OnError(msg);
            self->Cleanup(read_block);
            return;
        }

        CHECK(!self->incoming_header_);
        amessage& amsg = self->incoming_header_.emplace();
        memcpy(&amsg, transfer->buffer, sizeof(amsg));

        if (amsg.data_length > MAX_PAYLOAD) {
            std::string msg =
                    StringPrintf("usb read: payload length too long: %d", amsg.data_length);
            LOG(ERROR) << msg;
            self->OnError(msg);
            self->Cleanup(&self->header_read_);
            return;
        } else if (amsg.data_length == 0) {
            self->HandlePacket(amsg, std::nullopt);
            self->incoming_header_.reset();
            self->SubmitRead(read_block, sizeof(amessage));
        } else {
            read_block->active = false;
            self->SubmitRead(&self->payload_read_, amsg.data_length);
        }
    }

    static void LIBUSB_CALL payload_read_cb(libusb_transfer* transfer) {
        auto read_block = static_cast<ReadBlock*>(transfer->user_data);
        auto self = read_block->self;
        std::lock_guard<std::mutex> lock(self->read_mutex_);

        if (self->MaybeCleanup(&self->payload_read_)) {
            return;
        }

        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
            std::string msg = StringPrintf("usb read failed: status = %d", transfer->status);
            LOG(ERROR) << msg;
            self->OnError(msg);
            self->Cleanup(&self->payload_read_);
            return;
        }

        if (transfer->actual_length != transfer->length) {
            std::string msg =
                    StringPrintf("usb read: unexpected length for payload: wanted %d, got %d",
                                 transfer->length, transfer->actual_length);
            LOG(ERROR) << msg;
            self->OnError(msg);
            self->Cleanup(&self->payload_read_);
            return;
        }

        CHECK(self->incoming_header_.has_value());
        self->HandlePacket(*self->incoming_header_, std::move(read_block->block));
        self->incoming_header_.reset();

        read_block->active = false;
        self->SubmitRead(&self->header_read_, sizeof(amessage));
    }

    static void LIBUSB_CALL write_cb(libusb_transfer* transfer) {
        auto write_block = static_cast<WriteBlock*>(transfer->user_data);
        auto self = write_block->self;

        bool succeeded = transfer->status == LIBUSB_TRANSFER_COMPLETED;

        {
            std::lock_guard<std::mutex> lock(self->write_mutex_);
            libusb_free_transfer(transfer);
            self->writes_.erase(write_block->id);

            if (self->terminating_ && self->writes_.empty()) {
                self->destruction_cv_.notify_one();
            }
        }

        if (!succeeded) {
            self->OnError("libusb write failed");
        }
    }

    bool DoTlsHandshake(RSA*, std::string*) final {
        LOG(FATAL) << "tls not supported";
        return false;
    }

    void CreateRead(ReadBlock* read, bool header) {
        read->self = this;
        read->transfer = libusb_alloc_transfer(0);
        if (!read->transfer) {
            LOG(FATAL) << "failed to allocate libusb_transfer for read";
        }
        libusb_fill_bulk_transfer(read->transfer, device_handle_.get(), read_endpoint_, nullptr, 0,
                                  header ? header_read_cb : payload_read_cb, read, 0);
    }

    void SubmitRead(ReadBlock* read, size_t length) {
        read->block.resize(length);
        read->transfer->buffer = reinterpret_cast<unsigned char*>(read->block.data());
        read->transfer->length = length;
        read->active = true;
        int rc = libusb_submit_transfer(read->transfer);
        if (rc != 0) {
            LOG(ERROR) << "libusb_submit_transfer failed: " << libusb_strerror(rc);
        }
    }

    void SubmitWrite(Block&& block) REQUIRES(write_mutex_) {
        // TODO: Reuse write blocks.
        auto write = std::make_unique<WriteBlock>();

        write->self = this;
        write->id = TransferId::write(next_write_id_++);
        write->block = std::move(block);
        write->transfer = libusb_alloc_transfer(0);
        if (!write->transfer) {
            LOG(FATAL) << "failed to allocate libusb_transfer for write";
        }

        libusb_fill_bulk_transfer(write->transfer, device_handle_.get(), write_endpoint_,
                                  reinterpret_cast<unsigned char*>(write->block.data()),
                                  write->block.size(), &write_cb, write.get(), 0);
        libusb_submit_transfer(write->transfer);
        writes_[write->id] = std::move(write);
    }

    bool Write(std::unique_ptr<apacket> packet) final {
        LOG(DEBUG) << "USB write: " << dump_header(&packet->msg);
        Block header;
        header.resize(sizeof(packet->msg));
        memcpy(header.data(), &packet->msg, sizeof(packet->msg));

        std::lock_guard<std::mutex> lock(write_mutex_);
        if (terminating_) {
            return false;
        }

        SubmitWrite(std::move(header));
        if (!packet->payload.empty()) {
            size_t payload_length = packet->payload.size();
            SubmitWrite(std::move(packet->payload));

            // If the payload is a multiple of the endpoint packet size, we
            // need an explicit zero-sized transfer.
            if (should_perform_zero_transfer(payload_length, zero_mask_)) {
                LOG(INFO) << "submitting zero transfer for payload length " << payload_length;
                Block empty;
                SubmitWrite(std::move(empty));
            }
        }

        return true;
    }

    std::optional<libusb_device_descriptor> GetDeviceDescriptor() {
        libusb_device_descriptor device_desc;
        int rc = libusb_get_device_descriptor(device_.get(), &device_desc);
        if (rc != 0) {
            LOG(WARNING) << "failed to get device descriptor for device at " << device_address_
                         << ": " << libusb_error_name(rc);
            return {};
        }
        return device_desc;
    }

    bool FindInterface(libusb_device_descriptor* device_desc) {
        if (device_desc->bDeviceClass != LIBUSB_CLASS_PER_INTERFACE) {
            // Assume that all Android devices have the device class set to per interface.
            // TODO: Is this assumption valid?
            LOG(VERBOSE) << "skipping device with incorrect class at " << device_address_;
            return false;
        }

        libusb_config_descriptor* config_raw;
        int rc = libusb_get_active_config_descriptor(device_.get(), &config_raw);
        if (rc != 0) {
            LOG(WARNING) << "failed to get active config descriptor for device at "
                         << device_address_ << ": " << libusb_error_name(rc);
            return false;
        }
        const unique_config_descriptor config(config_raw);

        // Use size_t for interface_num so <iostream>s don't mangle it.
        size_t interface_num;
        uint16_t zero_mask = 0;
        uint8_t bulk_in = 0, bulk_out = 0;
        size_t packet_size = 0;
        bool found_adb = false;

        for (interface_num = 0; interface_num < config->bNumInterfaces; ++interface_num) {
            const libusb_interface& interface = config->interface[interface_num];

            if (interface.num_altsetting == 0) {
                continue;
            }

            const libusb_interface_descriptor& interface_desc = interface.altsetting[0];
            if (!is_adb_interface(interface_desc.bInterfaceClass, interface_desc.bInterfaceSubClass,
                                  interface_desc.bInterfaceProtocol)) {
                LOG(VERBOSE) << "skipping non-adb interface at " << device_address_
                             << " (interface " << interface_num << ")";
                continue;
            }

            if (interface.num_altsetting != 1) {
                // Assume that interfaces with alternate settings aren't adb interfaces.
                // TODO: Is this assumption valid?
                LOG(WARNING) << "skipping interface with unexpected num_altsetting at "
                             << device_address_ << " (interface " << interface_num << ")";
                continue;
            }

            LOG(VERBOSE) << "found potential adb interface at " << device_address_ << " (interface "
                         << interface_num << ")";

            bool found_in = false;
            bool found_out = false;
            for (size_t endpoint_num = 0; endpoint_num < interface_desc.bNumEndpoints;
                 ++endpoint_num) {
                const auto& endpoint_desc = interface_desc.endpoint[endpoint_num];
                const uint8_t endpoint_addr = endpoint_desc.bEndpointAddress;
                const uint8_t endpoint_attr = endpoint_desc.bmAttributes;

                const uint8_t transfer_type = endpoint_attr & LIBUSB_TRANSFER_TYPE_MASK;

                if (transfer_type != LIBUSB_TRANSFER_TYPE_BULK) {
                    continue;
                }

                if (endpoint_is_output(endpoint_addr) && !found_out) {
                    found_out = true;
                    bulk_out = endpoint_addr;
                    zero_mask = endpoint_desc.wMaxPacketSize - 1;
                } else if (!endpoint_is_output(endpoint_addr) && !found_in) {
                    found_in = true;
                    bulk_in = endpoint_addr;
                }

                size_t endpoint_packet_size = endpoint_desc.wMaxPacketSize;
                CHECK(endpoint_packet_size != 0);
                if (packet_size == 0) {
                    packet_size = endpoint_packet_size;
                } else {
                    CHECK(packet_size == endpoint_packet_size);
                }
            }

            if (found_in && found_out) {
                found_adb = true;
                break;
            } else {
                LOG(VERBOSE) << "rejecting potential adb interface at " << device_address_
                             << "(interface " << interface_num << "): missing bulk endpoints "
                             << "(found_in = " << found_in << ", found_out = " << found_out << ")";
            }
        }

        if (!found_adb) {
            return false;
        }

        interface_num_ = interface_num;
        write_endpoint_ = bulk_out;
        read_endpoint_ = bulk_in;
        zero_mask_ = zero_mask;
        return true;
    }

    std::string GetSerial() {
        std::string serial;

        auto device_desc = GetDeviceDescriptor();

        serial.resize(255);
        int rc = libusb_get_string_descriptor_ascii(
                device_handle_.get(), device_desc->iSerialNumber,
                reinterpret_cast<unsigned char*>(&serial[0]), serial.length());
        if (rc == 0) {
            LOG(WARNING) << "received empty serial from device at " << device_address_;
            return {};
        } else if (rc < 0) {
            LOG(WARNING) << "failed to get serial from device at " << device_address_
                         << libusb_error_name(rc);
            return {};
        }
        serial.resize(rc);

        return serial;
    }

    bool OpenDevice(std::string* error) {
        if (device_handle_) {
            return true;
        }

        libusb_device_handle* handle_raw;
        int rc = libusb_open(device_.get(), &handle_raw);
        if (rc != 0) {
            std::string err = StringPrintf("failed to open device: %s", libusb_strerror(rc));
            LOG(ERROR) << err;

#if defined(__linux__)
            std::string device_serial;
            // libusb doesn't think we should be messing around with devices we don't have
            // write access to, but Linux at least lets us get the serial number anyway.
            if (!android::base::ReadFileToString(get_device_serial_path(device_.get()),
                                                 &device_serial)) {
                // We don't actually want to treat an unknown serial as an error because
                // devices aren't able to communicate a serial number in early bringup.
                // http://b/20883914
                serial_ = "<unknown>";
            } else {
                serial_ = android::base::Trim(device_serial);
            }
#else
            // On Mac OS and Windows, we're screwed. But I don't think this situation actually
            // happens on those OSes.
#endif

            if (error) {
                *error = std::move(err);
            }

            return false;
        }

        unique_device_handle handle(handle_raw);
        device_handle_ = std::move(handle);

        auto device_desc = GetDeviceDescriptor();
        if (!device_desc) {
            device_handle_.reset();
            return false;
        }

        if (!FindInterface(&device_desc.value())) {
            device_handle_.reset();
            return false;
        }

        serial_ = GetSerial();
        return true;
    }

    bool StartImpl(std::string* error) {
        if (!OpenDevice(error)) {
            return false;
        }

        LOG(DEBUG) << "successfully opened adb device at " << device_address_ << ", "
                   << StringPrintf("bulk_in = %#x, bulk_out = %#x", read_endpoint_,
                                   write_endpoint_);

        // WARNING: this isn't released via RAII.
        int rc = libusb_claim_interface(device_handle_.get(), interface_num_);
        if (rc != 0) {
            LOG(WARNING) << "failed to claim adb interface for device '" << serial_ << "'"
                         << libusb_error_name(rc);
            return false;
        }

        for (uint8_t endpoint : {read_endpoint_, write_endpoint_}) {
            rc = libusb_clear_halt(device_handle_.get(), endpoint);
            if (rc != 0) {
                LOG(WARNING) << "failed to clear halt on device '" << serial_ << "' endpoint 0x"
                             << std::hex << endpoint << ": " << libusb_error_name(rc);
                libusb_release_interface(device_handle_.get(), interface_num_);
                return false;
            }
        }

        LOG(INFO) << "registered new usb device '" << serial_ << "'";
        std::lock_guard lock(read_mutex_);
        CreateRead(&header_read_, true);
        CreateRead(&payload_read_, false);
        SubmitRead(&header_read_, sizeof(amessage));

        return true;
    }

    void OnError(const std::string& error) {
        std::call_once(error_flag_, [this, &error]() {
            if (error_callback_) {
                error_callback_(this, error);
            }
        });
    }

    virtual void Reset() override final {
        Stop();

        if (libusb_reset_device(device_handle_.get()) == 0) {
            libusb_device* device = libusb_ref_device(device_.get());
            fdevent_run_on_main_thread([device]() {
                process_device(device);
                libusb_unref_device(device);
            });
        }
    }

    virtual void Start() override final {
        std::string error;
        if (!StartImpl(&error)) {
            OnError(error);
            return;
        }
    }

    virtual void Stop() override final {
        // This is rather messy, because of the lifecyle of libusb_transfers.
        //
        // We can't call libusb_free_transfer for a submitted transfer, we have to cancel it
        // and free it in the callback. Complicating things more, it's possible for us to be in
        // the callback for a transfer as the destructor is being called, at which point cancelling
        // the transfer won't do anything (and it's possible that we'll submit the transfer again
        // in the callback).
        //
        // Resolve this by setting an atomic flag before we lock to cancel transfers, and take the
        // lock in the callbacks before checking the flag.

        if (terminating_) {
            return;
        }

        terminating_ = true;

        {
            std::unique_lock<std::mutex> lock(write_mutex_);
            ScopedLockAssertion assumed_locked(write_mutex_);

            if (!writes_.empty()) {
                for (auto& [id, write] : writes_) {
                    libusb_cancel_transfer(write->transfer);
                }

                destruction_cv_.wait(lock, [this]() {
                    ScopedLockAssertion assumed_locked(write_mutex_);
                    return writes_.empty();
                });
            }
        }

        {
            std::unique_lock<std::mutex> lock(read_mutex_);
            ScopedLockAssertion assumed_locked(read_mutex_);
            if (header_read_.transfer) {
                if (header_read_.active) {
                    libusb_cancel_transfer(header_read_.transfer);
                } else {
                    libusb_free_transfer(header_read_.transfer);
                }
            }

            if (payload_read_.transfer) {
                if (payload_read_.active) {
                    libusb_cancel_transfer(payload_read_.transfer);
                } else {
                    libusb_free_transfer(payload_read_.transfer);
                }
            }

            destruction_cv_.wait(lock, [this]() {
                ScopedLockAssertion assumed_locked(read_mutex_);
                return !header_read_.active && !payload_read_.active;
            });
        }

        if (device_handle_) {
            libusb_release_interface(device_handle_.get(), interface_num_);
        }

        OnError("requested stop");
    }

    static std::optional<std::shared_ptr<LibusbConnection>> Create(unique_device device) {
        auto connection = std::make_unique<LibusbConnection>(std::move(device));
        if (!connection) {
            LOG(FATAL) << "failed to construct LibusbConnection";
        }

        auto device_desc = connection->GetDeviceDescriptor();
        if (!device_desc) {
            return {};
        }

        if (!connection->FindInterface(&device_desc.value())) {
            return {};
        }

        if (!connection->OpenDevice(nullptr)) {
            return {};
        }

        return connection;
    }

    unique_device device_;
    unique_device_handle device_handle_;
    std::string device_address_;
    std::string serial_ = "<unknown>";

    uint32_t interface_num_;
    uint8_t write_endpoint_;
    uint8_t read_endpoint_;

    std::mutex read_mutex_;
    ReadBlock header_read_ GUARDED_BY(read_mutex_);
    ReadBlock payload_read_ GUARDED_BY(read_mutex_);
    std::optional<amessage> incoming_header_ GUARDED_BY(read_mutex_);
    IOVector incoming_payload_ GUARDED_BY(read_mutex_);

    std::mutex write_mutex_;
    std::unordered_map<TransferId, std::unique_ptr<WriteBlock>> writes_ GUARDED_BY(write_mutex_);
    std::atomic<size_t> next_write_id_ = 0;

    std::once_flag error_flag_;
    std::atomic<bool> terminating_ = false;
    std::condition_variable destruction_cv_;

    size_t zero_mask_ = 0;
};

static libusb_hotplug_callback_handle hotplug_handle;
static std::mutex usb_handles_mutex [[clang::no_destroy]];
static std::unordered_map<libusb_device*, std::weak_ptr<LibusbConnection>> usb_handles
        [[clang::no_destroy]] GUARDED_BY(usb_handles_mutex);
static std::atomic<int> connecting_devices(0);

static void process_device(libusb_device* device_raw) {
    std::string device_address = get_device_address(device_raw);
    LOG(INFO) << "device connected: " << device_address;

    unique_device device(libusb_ref_device(device_raw));
    auto connection_opt = LibusbConnection::Create(std::move(device));
    if (!connection_opt) {
        LOG(INFO) << "ignoring device: not an adb interface";
        return;
    }

    auto connection = *connection_opt;
    LOG(INFO) << "constructed LibusbConnection for device " << connection->serial_ << " ("
              << device_address << ")";

    register_usb_transport(connection, connection->serial_.c_str(), device_address.c_str(), true);
}

static void device_connected(libusb_device* device) {
#if defined(__linux__)
    // Android's host linux libusb uses netlink instead of udev for device hotplug notification,
    // which means we can get hotplug notifications before udev has updated ownership/perms on the
    // device. Since we're not going to be able to link against the system's libudev any time soon,
    // hack around this by inserting a sleep.
    libusb_ref_device(device);
    auto thread = std::thread([device]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        process_device(device);
        if (--connecting_devices == 0) {
            adb_notify_device_scan_complete();
        }
        libusb_unref_device(device);
    });
    thread.detach();
#else
    process_device(device);
#endif
}

static void device_disconnected(libusb_device* device) {
    usb_handles_mutex.lock();
    auto it = usb_handles.find(device);
    if (it != usb_handles.end()) {
        // We need to ensure that we don't destroy the LibusbConnection on this thread,
        // as we're in a context with internal libusb mutexes held.
        std::weak_ptr<LibusbConnection> connection_weak = it->second;
        usb_handles.erase(it);
        fdevent_run_on_main_thread([connection_weak]() {
            auto connection = connection_weak.lock();
            if (connection) {
                connection->Stop();
                LOG(INFO) << "libusb_hotplug: device disconnected: " << connection->serial_;
            } else {
                LOG(INFO) << "libusb_hotplug: device disconnected: (destroyed)";
            }
        });
    }
    usb_handles_mutex.unlock();
}

static auto& hotplug_queue = *new BlockingQueue<std::pair<libusb_hotplug_event, libusb_device*>>();
static void hotplug_thread() {
    LOG(INFO) << "libusb hotplug thread started";
    adb_thread_setname("libusb hotplug");
    while (true) {
        hotplug_queue.PopAll([](std::pair<libusb_hotplug_event, libusb_device*> pair) {
            libusb_hotplug_event event = pair.first;
            libusb_device* device = pair.second;
            if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
                device_connected(device);
            } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
                device_disconnected(device);
            }
        });
    }
}

static LIBUSB_CALL int hotplug_callback(libusb_context*, libusb_device* device,
                                        libusb_hotplug_event event, void*) {
    // We're called with the libusb lock taken. Call these on a separate thread outside of this
    // function so that the usb_handle mutex is always taken before the libusb mutex.
    static std::once_flag once;
    std::call_once(once, []() { std::thread(hotplug_thread).detach(); });

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        ++connecting_devices;
    }
    hotplug_queue.Push({event, device});
    return 0;
}

namespace libusb {

void usb_init() {
    LOG(DEBUG) << "initializing libusb...";
    int rc = libusb_init(nullptr);
    if (rc != 0) {
        LOG(FATAL) << "failed to initialize libusb: " << libusb_error_name(rc);
    }

    // Register the hotplug callback.
    rc = libusb_hotplug_register_callback(
            nullptr,
            static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                              LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
            LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            LIBUSB_CLASS_PER_INTERFACE, hotplug_callback, nullptr, &hotplug_handle);

    if (rc != LIBUSB_SUCCESS) {
        LOG(FATAL) << "failed to register libusb hotplug callback";
    }

    // Spawn a thread for libusb_handle_events.
    std::thread([]() {
        adb_thread_setname("libusb");
        while (true) {
            libusb_handle_events(nullptr);
        }
    }).detach();
}

}  // namespace libusb
