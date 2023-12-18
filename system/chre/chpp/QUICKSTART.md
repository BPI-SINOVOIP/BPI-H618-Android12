# CHPP Integration

This guide focuses on integrating and porting CHPP to your desired platform. For implementation details please refer to the included README.md instead. The minimum typical steps are provided.

## CHPP Transport Layer Integration

### 1. Platform-specific functionality

Implement the platform-specific functionality utilized by CHPP. These can be found in chpp/platform and include:

1. Memory allocation and deallocation
1. Thread notification mechanisms and thread signalling
1. Mutexes (or other resource sharing mechanism)
1. Condition variables
1. Logging (including defining CHPP_LOGx, CHPP_LOG_OOM, PRIuSIZE)
1. CRC32 (a half-byte algorithm is provided in platform/shared, but it is recommended to use a platform-optimized / hardware algorithm when available)

Sample Linux implementations are provided.

### 1. Link-Layer APIs

The APIs that are needed to tie CHPP to the serial port are as follows. Details are provided in link.h.

1. void chppPlatformLinkInit(\*params)
1. void chppPlatformLinkDeinit(\*params)
1. void chppPlatformLinkReset(\*params)
1. enum ChppLinkErrorCode chppPlatformLinkSend(\*params, \*buf, len)
1. Depending on implementation, void chppLinkSendDoneCb(\*params)
1. void chppPlatformLinkDoWork(\*params)
1. bool chppRxDataCb(\*context, \*buf, len)
1. Optionally, chppRxPacketCompleteCb(\*context)

In addition, the system must implement and initialize the platform-specific linkParams data structure as part of platform_link.h

### 1. Initialization

In order to initialize CHPP, it is necessary to

1. Allocate the transportContext and appContext structs that hold the state for each instance of the application and transport layers (in any order)
1. Call the layers’ initialization functions, chppTransportInit and chppAppInit (in any order)
1. Initialize the platform-specific linkParams struct (part of the transport struct)
1. Call chppWorkThreadStart to start the main thread for CHPP's Transport Layer

### 1. Testing

Several unit tests are provided in transport_test.c. In addition, loopback functionality is already implemented in CHPP, and can be used for testing. For details on crafting a loopback datagram, please refer to README.md and the transport layer unit tests (transport_test.c).

### 1. Termination

In order to terminate CHPP's main transport layer thread, it is necessary to

1. Call chppWorkThreadStop() to stop the main worker thread.
1. Call the layers’ deinitialization functions, chppTransportDeinit and chppAppDeinit (in any order)
1. Deallocate the transportContext, appContext, and the platform-specific linkParams structs

### 1. Single-threaded systems

If the system does not support multi-threading, the chppWorkThreadHandleSignal method can be used to directly handle signals without using chppWorkThreadStart.

Note that the system MUST replicate the high-level behavior of chppWorkThreadStart exactly in this case. More details in the documentation of chppWorkThreadHandleSignal.

For such systems, chppTransportGetTimeUntilNextDoWorkNs() can be used to replicate the functionality of chppNotifierTimedWait(). chppTransportGetTimeUntilNextDoWorkNs() returns the time until chppTransportDoWork() must be called again.

## CHPP Services Integration

CHPP provides several predefined services (including Loopback Test, Service Discovery), as well as three standard services that follow the CHRE PAL API to simplify integration and testing. CHPP allows for custom services as well, as described in README.md. The standard services included in CHPP are

1. WWAN
1. WiFi
1. GNSS

In order to integrate the standard services using the CHRE PAL API, please refer to each service's interface as provided in include/chre/pal/
