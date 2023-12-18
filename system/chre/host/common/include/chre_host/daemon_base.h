/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef CHRE_DAEMON_H_
#define CHRE_DAEMON_H_

#include <atomic>
#include <cstdint>
#include <map>
#include <queue>
#include <string>

#include "chre_host/host_protocol_host.h"
#include "chre_host/log_message_parser_base.h"
#include "chre_host/socket_server.h"

namespace android {
namespace chre {

class ChreDaemonBase {
 public:
  ChreDaemonBase() : mChreShutdownRequested(false) {}
  virtual ~ChreDaemonBase() {}

  /**
   * Initialize the CHRE daemon. We're expected to fail here and not start
   * the daemon if we don't get all the resources we're hoping to.
   * Any resources claimed by this function should be released in the
   * destructor
   *
   * @return true on successful initialization
   */
  virtual bool init() = 0;

  /**
   * Start the CHRE Daemon. This method must be called after @ref init() has
   * been called.
   */
  virtual void run() = 0;

  /**
   * Send a message to CHRE
   *
   * @param clientId The client ID that this message originates from.
   * @param data The data to pass down.
   * @param length The size of the data to send.
   * @return true if successful, false otherwise.
   */
  bool sendMessageToChre(uint16_t clientId, void *data, size_t dataLen);

  /**
   * Function to query if a graceful shutdown of CHRE was requested
   *
   * @return true if a graceful shutdown was requested
   */
  bool wasShutdownRequested() const {
    return mChreShutdownRequested;
  }

  /**
   * Loads the supplied file into the provided buffer.
   *
   * @param filename The name of the file to load.
   * @param buffer The buffer to load into.
   * @return true if successful, false otherwise.
   */
  static bool readFileContents(const char *filename,
                               std::vector<uint8_t> *buffer);

 protected:
  //! The host ID to use when preloading nanoapps. This is used before the
  //! server is started and is sufficiently high enough so as to not collide
  //! with any clients after the server starts.
  static constexpr uint16_t kHostClientIdDaemon = UINT16_MAX;

  void setShutdownRequested(bool request) {
    mChreShutdownRequested = request;
  }

  /**
   * Attempts to load all preloaded nanoapps from a config file. The config file
   * is expected to be valid JSON with the following structure:
   *
   * { "nanoapps": [
   *     "/path/to/nanoapp_1",
   *     "/path/to/nanoapp_2"
   * ]}
   *
   * The napp_header and so files will both be loaded. All errors are logged.
   *
   * TODO: This is SLPI specific right now, and needs to be revisited to
   * implement platform specific loading.
   */
  void loadPreloadedNanoapps();

  /**
   * Loads a preloaded nanoapp given a filename to load from. Allows the
   * transaction to complete before the nanoapp starts so the server can start
   * serving requests as soon as possible.
   *
   * @param directory The directory to load the nanoapp from.
   * @param name The filename of the nanoapp to load.
   * @param transactionId The transaction ID to use when loading the app.
   */
  virtual void loadPreloadedNanoapp(const std::string &directory,
                                    const std::string &name,
                                    uint32_t transactionId);

  /**
   * Sends a preloaded nanoapp filename / metadata to CHRE.
   *
   * @param header The nanoapp header binary blob.
   * @param nanoappName The filename of the nanoapp to be loaded.
   * @param transactionId The transaction ID to use when loading the app.
   * @return true if successful, false otherwise.
   */
  bool loadNanoapp(const std::vector<uint8_t> &header,
                   const std::string &nanoappName, uint32_t transactionId);

  /**
   * Loads a nanoapp by sending the nanoapp filename to the CHRE framework. This
   * method will return after sending the request so no guarantee is made that
   * the nanoapp is loaded until after the response is received.
   *
   * @param appId The ID of the nanoapp to load.
   * @param appVersion The version of the nanoapp to load.
   * @param appTargetApiVersion The version of the CHRE API that the app
   * targets.
   * @param appBinaryName The name of the binary as stored in the filesystem.
   * This will be used to load the nanoapp into CHRE.
   * @param transactionId The transaction ID to use when loading.
   * @return true if a request was successfully sent, false otherwise.
   */
  bool sendNanoappLoad(uint64_t appId, uint32_t appVersion,
                       uint32_t appTargetApiVersion,
                       const std::string &appBinaryName,
                       uint32_t transactionId);

  /**
   * Send a time sync message to CHRE
   *
   * @param logOnError If true, logs an error message on failure.
   *
   * @return true if the time sync message was successfully sent to CHRE.
   */
  bool sendTimeSync(bool logOnError);

  /**
   * Sends a time sync message to CHRE, retrying a specified time until success.
   *
   * @param maxNumRetries The number of times to retry sending the message
   *
   * @return true if the time sync message was successfully sent to CHRE.
   */
  bool sendTimeSyncWithRetry(size_t numRetries, useconds_t retryDelayUs,
                             bool logOnError);

  /**
   * Interface to a callback that is called when the Daemon receives a message.
   *
   * @param message A buffer containing the message
   * @param messageLen size of the message buffer in bytes
   */
  void onMessageReceived(const unsigned char *message, size_t messageLen);

  /**
   * Handles a message that is directed towards the daemon.
   *
   * @param message The message sent to the daemon.
   */
  virtual void handleDaemonMessage(const uint8_t *message);

  /**
   * Platform-specific method to actually do the message sending requested by
   * sendMessageToChre.
   */
  virtual bool doSendMessage(void *data, size_t dataLen) = 0;

  /**
   * Enables or disables LPMA (low power microphone access).
   */
  virtual void configureLpma(bool enabled) = 0;

  /**
   * @return logger used by the underlying platform.
   */
  virtual ChreLogMessageParserBase *getLogger() = 0;

  //! Server used to communicate with daemon clients
  SocketServer mServer;

 private:
  //! Set to true when we request a graceful shutdown of CHRE
  std::atomic<bool> mChreShutdownRequested;

  //! Contains a set of transaction IDs used to load the preloaded nanoapps.
  //! The IDs are stored in the order they are sent.
  std::queue<uint32_t> mPreloadedNanoappPendingTransactionIds;

  /**
   * Computes and returns the clock drift between the system clock
   * and the processor timer registers
   *
   * @return offset in nanoseconds
   */
  virtual int64_t getTimeOffset(bool *success) = 0;
};

}  // namespace chre
}  // namespace android

#endif  // CHRE_DAEMON_H
