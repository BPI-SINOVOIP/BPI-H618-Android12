
#ifndef CEC_H_
#define CEC_H_

#include <sys/ioctl.h>

/* cec_msg tx/rx_status field */
#define CEC_TX_STATUS_OK            (1 << 0)
#define CEC_TX_STATUS_NACK          (1 << 1)
#define CEC_TX_STATUS_ERROR         (1 << 2)
#define CEC_TX_STATUS_MAX_RETRIES   (1 << 3)

#define CEC_RX_STATUS_OK            (1 << 0)
#define CEC_RX_STATUS_TIMEOUT       (1 << 1)
#define CEC_RX_STATUS_FEATURE_ABORT (1 << 2)

typedef struct cec_msg {
    uint32_t len;       /* length in bytes of this message             */
    uint32_t timeout;   /* the timeout (in ms) for waiting for a reply */
    uint32_t sequence;  /* seq num for reply tracing                   */
    uint32_t tx_status; /* for sending cec message result              */
    uint32_t rx_status; /* for receiving cec message result            */
    uint8_t  msg[32];
} cec_msg_t;

/* CEC config ioctl */
#define CEC_S_PHYS_ADDR             _IOW('c', 1, uint16_t)
#define CEC_G_PHYS_ADDR             _IOR('c', 2, uint16_t)
#define CEC_S_LOG_ADDR              _IOW('c', 3, uint8_t)
#define CEC_G_LOG_ADDR              _IOR('c', 4, uint8_t)

/* CEC transmit/receive ioctl */
#define CEC_TRANSMIT                _IOWR('c', 5, struct cec_msg)
#define CEC_RECEIVE                 _IOWR('c', 6, struct cec_msg)

#endif
