# CHPP Release Notes

A summary of notable changes is provided in the form of release notes. Dates are provided as yyyy-mm-dd. Note that this is not meant to be a detailed changelog; for a detailed change list, please refer to git commits.

### 2020-03-04 (4c668b3)

Initial release of CHPP.

- CHPP transport and app layers
- Loopback testing service

### 2020-07-28 (7cebe57)

This release enables service integration with WWAN / WiFi / GNSS devices based on the CHRE PAL API.

- New functionality

  - Reset and reset-ack implementation to allow either peer to initialize the other (e.g., upon boot)
  - Discovery service to provide a list of services
  - Discovery client to match clients with discovered services
  - Standard WWAN service based on the CHRE PAL API
  - Standard WiFi service based on the CHRE PAL API
  - Standard GNSS service based on the CHRE PAL API
  - Standard WWAN client based on the CHRE PAL API

- Updates and bug fixes to existing layers, including

  - Better logging to assist verification and debugging
  - Error replies are sent over the wire for transport layer errors
  - Over-the-wire preamble has been corrected (byte shifting error)

- API and integration changes

  - App layer header now includes an error code
  - App layer message type now occupies only the least significant nibble (LSN). The most significant nibble (MSN) is reserved
  - chppPlatformLinkSend() now returns an error code instead of a boolean
  - Added initialization, deinitialization, and reset functionality for the link layer (see link.h)
  - Condition variables functionality needs to be supported alongside other platform functionality (see chpp/platform/)
  - Name changes for the logging APIs

### 2020-08-07 (0b41306)

This release contains bug fixes as well as the loopback client.

- New functionality

  - Loopback client to run and verify a loopback test using a provided data buffer

- Cleanup and bug fixes

  - Corrected sequence number handling
  - Updated log messages
  - More accurate casting into enums

### 2020-08-27 (8ab5c23)

This release contains additional clients, a virtual link layer for testing (e.g., using loopback), and several important bug fixes.

- New functionality

  - Basic implementation of the standard WiFi client based on the CHRE PAL API
  - Basic implementation of the standard GNSS client based on the CHRE PAL API
  - Virtual link layer that connects CHPP with itself on Linux to enable testing, including reset, discovery, and loopback

- Cleanup and bug fixes

  - Client implementation cleanup
  - Client-side handling of close responses
  - Reset / reset-ack handshaking mechanism fixed
  - Loopback client fixed
  - Enhanced log messages
  - Service command #s are now sequential

- API and integration changes

  - Platform-specific time functionality (platform_time.h)

### 2020-10-01 (95829e3)

This release updates client functionality using the parser, adds a transport-layer loopback mechanism for testing and debugging, and includes several important bug fixes.

- New functionality

  - Parser for CHPP -> CHRE Data Structure Decoding
  - Completed client functionality using parser-generated functions
  - Transport-layer-loopback client and service. The Transport-layer loopback ignores app layer functionality and state, as well as checksums and sequence numbers

- Cleanup and bug fixes

  - Improved compiler compatibility for MSVC, as well as when enabling additional compiler warning flags
  - Fixed handling of fragmented datagrams
  - Corrected MTU calculation
  - Corrected loopback assert
  - Slimmer OOM logging
  - Parser code and header files were relocated

### 2021-02-08 (f1d249c)

In addition to enhancements and bug fixes, this release enables error and reset handling and several other features.

- New functionality

  - ARQ implementation. Note that it is necessary to either implement chppNotifierTimedWait() or a single-threaded workaround as described in QUICKSTART.md to detect timeouts
  - Checksum support via IEEE CRC-32. A sample implementation is provided, but it is expected that most devices have optimized implementations available or may prefer implementations with alternate optimization criteria
  - Timesync functionality and timestamp offset correction for WiFi and WWAN measurements
  - Reset handling throughout the transport layer, clients, and services, opening and recovering state as needed
  - WiFi RTT client and service support
  - Multiple loopback client support
  - Transport-layer-loopback client validates response and returns the result
  - Support for pseudo-opening services at the client, so they appear always available
  - Correct responses generated at clients when async requests fail at services

- Cleanup and bug fixes

  - Client and service fixes including length and bound checks, missing implementations
  - Parser fixes including empty pointers set to null, compatibility with processors lacking unaligned access support
  - Stability fixes throughout CHPP and tests
  - Improved compiler compatibility for C99+ and pre-C99 systems (even though CHPP does not officially support pre-C99)
  - Updated documentation and logging

### 2021-03-25 (f908420)

This release updates the built-in timesync and checksum functionality and addresses bugs and compatibility issues.

- Updated functionality

  - Timesync is redesigned to become non-blocking
  - Outgoing checksums are enabled by default
  - An updated sample CRC32 implementation is provided (It is still expected that devices that have existing, optimized implementations use their own)
  - Client deinitialization and reset support

- Cleanup and bug fixes

  - Logging updates, including reset reasoning, avoiding %s for compatibility
  - Stability fixes and cleanup throughout CHPP and tests, including the reopening flow, permanent_failure state, and a memory leak
  - Testing improvements

### 2021-05-24 (c9bfae3)

This release enables better identification of end-of-packets as well as addressing bugs and compatibility issues.

- Updated functionality

  - Rx timeout detection
  - Rx MTU enforcement
  - Support for Rx end-of-packet notifications from the link layer (optional, platform dependent)

- Cleanup and bug fixes

  - Reset functionality cleanup, including updating the functionality of CHPP_TRANSPORT_MAX_RETX and CHPP_TRANSPORT_MAX_RESET to reflect their intent accurately
  - Pseudo-open clients remain pseudo-open after open failures
  - Fixed reopen failures after transport reset
  - Added missing WiFi ranging service response
  - Memory allocation and initialization improvements
  - Mutex handling improvements
  - Compatibility fixes
  - Testing improvements

### 2021-06-17 (this)

This release adds request timeout support at the client and addresses several bugs and compatibility issues throughout CHPP, including fixing open-state tracking at the service. Note that with the corrected open-state tracking, it is essential for services to correctly implement the close() PAL API so that it disables any ongoing requests and returns to a clean state.

- Updated functionality

  - Client request timeout support

- Cleanup and bug fixes

  - Service open-state tracking
  - Memory handling fixes
  - Added GNSS passive location listener service response
  - Enforced error code requirements on service responses
  - Fixed ARQ handling of duplicate packets
  - Client request/response state refactoring
  - Client registration cleanup
  - Reset handling fixes
  - Testing improvements
