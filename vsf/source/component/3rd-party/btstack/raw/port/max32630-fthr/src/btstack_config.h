// Maxim Max32630FTHR Port
//
#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

#include <stdint.h>

// Port related features
#define HAVE_BTSTACK_STDIN
#define HAVE_EMBEDDED_TIME_MS
#define HAVE_INIT_SCRIPT

// BTstack features that can be enabled
#define ENABLE_BLE
#define ENABLE_CLASSIC
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_DATA_CHANNELS
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_INFO
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS
#define ENABLE_PRINTF_HEXDUMP
#define ENABLE_SCO_OVER_HCI

// BTstack configuration. buffers, sizes, ...
#define HCI_ACL_PAYLOAD_SIZE 1021
#define MAX_NR_AVDTP_CONNECTIONS 1
#define MAX_NR_AVDTP_STREAM_ENDPOINTS 1
#define MAX_NR_AVRCP_CONNECTIONS 1
#define MAX_NR_BNEP_CHANNELS 1
#define MAX_NR_BNEP_SERVICES 1
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES  2
#define MAX_NR_GATT_CLIENTS 1
#define MAX_NR_HCI_CONNECTIONS 1
#define MAX_NR_HFP_CONNECTIONS 1
#define MAX_NR_L2CAP_CHANNELS  3
#define MAX_NR_L2CAP_SERVICES  3
#define MAX_NR_RFCOMM_CHANNELS 1
#define MAX_NR_RFCOMM_MULTIPLEXERS 1
#define MAX_NR_RFCOMM_SERVICES 1
#define MAX_NR_SERVICE_RECORD_ITEMS 4
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 1

// Link Key DB and LE Device DB using TLV on top of Flash Sector interface
#define NVM_NUM_DEVICE_DB_ENTRIES 16
#define NVM_NUM_LINK_KEYS 16

#endif