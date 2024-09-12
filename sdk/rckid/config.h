#pragma once

/** UART transceiver does not support HW ack for messages, instead receiver is expected to send ACK message to the sender. If not received within this interval, the message is considered undelivered.
 
    NOTE the unit is microseconds!
 */
#define UART_TX_TIMEOUT_US 200000

// backend specific configuration, which may override the general configuration above
#include "backend_config.h"

