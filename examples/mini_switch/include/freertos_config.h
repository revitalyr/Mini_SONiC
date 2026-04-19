#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "cross_platform.h" // For PACKED_STRUCT macros

#ifdef __linux__
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#elif defined(PLATFORM_WINDOWS) && !defined(INC_FREERTOS_H) && !defined(_FREERTOS_SIM_TYPES_DEFINED)
// Types are provided by cross_platform.h; only define missing logic here
#define tskIDLE_PRIORITY 0
#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR() 0
#define taskEXIT_CRITICAL_FROM_ISR(x) (void)(x)
#define portMAX_DELAY (0xFFFFFFFFUL)
#endif

/* ── Application Configuration ───────────────────────────────── */

/** @brief Maximum number of switch ports */
#define SWITCH_MAX_PORTS        (8U)

/** @brief Network buffer size for Ethernet frames */
#define NET_BUFFER_SIZE        (1518U)  /* Standard Ethernet MTU + headers */

/** @brief MAC address table size */
#define MAC_TABLE_SIZE         (256U)

/** @brief ARP cache table size */
#define ARP_TABLE_SIZE         (64U)

/** @brief VLAN table size */
#define VLAN_TABLE_SIZE        (32U)

/** @brief Packet queue depth */
#define PACKET_QUEUE_DEPTH     (16U)

/** @brief Task stack sizes in words */
#define RX_TASK_STACK_SIZE     (256U)
#define WORKER_TASK_STACK_SIZE (384U)
#define CLI_TASK_STACK_SIZE    (192U)

/** @brief Task priorities */
#define TASK_PRIORITY_IDLE     (tskIDLE_PRIORITY + 0U)
#define TASK_PRIORITY_LOW      (tskIDLE_PRIORITY + 1U)
#define TASK_PRIORITY_NORMAL   (tskIDLE_PRIORITY + 2U)
#define TASK_PRIORITY_HIGH     (tskIDLE_PRIORITY + 3U)

/** @brief Tick rates */
#define CLI_POLL_PERIOD_MS     (50U)
#define VISUAL_UPDATE_PERIOD_MS (5000U)
#define PACKET_TIMEOUT_MS      (100U)

/* ── Hardware Configuration ───────────────────────────────── */

/** @brief Ethernet peripheral base addresses (STM32F4xx example) */
#define ETH_BASE_ADDR          (0x40028000UL)

/** @brief DMA buffer alignment requirement */
#define DMA_BUFFER_ALIGNMENT   (32U)

/** @brief Maximum interrupt priority for FreeRTOS API calls */
#define MAX_SYSCALL_INTERRUPT_PRIORITY (5U)

/* ── Type Definitions ─────────────────────────────────────── */

/**
 * @brief Network packet buffer structure
 */
typedef struct {
    uint8_t data[NET_BUFFER_SIZE];  /**< Packet data */
    uint16_t len;                    /**< Packet length in bytes */
    uint8_t port_id;                 /**< Source port identifier */
    uint32_t timestamp_ms;            /**< Timestamp in milliseconds */
} NetPacket_t;

/**
 * @brief MAC address entry
 */
typedef struct {
    uint8_t mac_addr[6U];           /**< MAC address */
    uint8_t port_id;                 /**< Associated port */
    uint16_t vlan_id;                /**< VLAN ID */
    uint32_t last_seen_ms;           /**< Last activity timestamp */
    bool valid;                      /**< Entry validity flag */
} MacEntry_t;

/**
 * @brief ARP cache entry
 */
typedef struct {
    uint32_t ip_addr;                /**< IPv4 address (network order) */
    uint8_t mac_addr[6U];           /**< MAC address */
    uint32_t last_seen_ms;           /**< Last activity timestamp */
    bool valid;                      /**< Entry validity flag */
} ArpEntry_t;

/**
 * @brief Switch port configuration
 */
typedef struct {
    uint8_t port_id;                 /**< Port identifier */
    bool enabled;                     /**< Port enable status */
    uint16_t vlan_id;                /**< Default VLAN ID */
    uint32_t rx_packets;             /**< RX packet counter */
    uint32_t tx_packets;             /**< TX packet counter */
    uint32_t rx_errors;              /**< RX error counter */
    uint32_t tx_errors;              /**< TX error counter */
} SwitchPort_t;

/**
 * @brief Ethernet frame header
 */
START_PACKED_STRUCT
typedef struct {
    uint8_t dest_addr[6U];           /**< Destination MAC */
    uint8_t src_addr[6U];            /**< Source MAC */
    uint16_t ethertype;              /**< EtherType (host order) */
} EthHeader_t PACKED_STRUCT;
END_PACKED_STRUCT

/**
 * @brief VLAN tag structure
 */
START_PACKED_STRUCT
typedef struct {
    uint16_t tpid;                   /**< Tag Protocol Identifier (0x8100) */
    uint16_t tci;                    /**< Tag Control Information */
} VlanTag_t PACKED_STRUCT;
END_PACKED_STRUCT

/**
 * @brief ARP packet structure
 */
START_PACKED_STRUCT
typedef struct {
    uint16_t htype;                  /**< Hardware type */
    uint16_t ptype;                  /**< Protocol type */
    uint8_t hlen;                    /**< Hardware address length */
    uint8_t plen;                    /**< Protocol address length */
    uint16_t oper;                   /**< Operation */
    uint8_t sha[6U];                 /**< Sender hardware address */
    uint32_t spa;                    /**< Sender protocol address */
    uint8_t tha[6U];                 /**< Target hardware address */
    uint32_t tpa;                    /**< Target protocol address */
} ArpPacket_t PACKED_STRUCT;
END_PACKED_STRUCT

/* ── EtherType Constants ───────────────────────────────────── */

#define ETH_TYPE_ARP            (0x0806U)
#define ETH_TYPE_IP             (0x0800U)
#define ETH_TYPE_VLAN           (0x8100U)
#define ETH_TYPE_IPV6           (0x86DDU)

/* ── ARP Constants ───────────────────────────────────────── */

#define ARP_HTYPE_ETHER         (1U)
#define ARP_PTYPE_IP            (0x0800U)
#define ARP_OPER_REQUEST        (1U)
#define ARP_OPER_REPLY          (2U)

/* ── Error Codes ───────────────────────────────────────── */

typedef enum {
    SWITCH_STATUS_OK = 0U,
    SWITCH_STATUS_ERROR = 1U,
    SWITCH_STATUS_INVALID_PARAM = 2U,
    SWITCH_STATUS_NO_MEMORY = 3U,
    SWITCH_STATUS_TIMEOUT = 4U,
    SWITCH_STATUS_BUFFER_FULL = 5U,
    SWITCH_STATUS_NOT_FOUND = 6U
} SwitchStatus_t;

/* ── Compile-time Assertions ─────────────────────────────── */

#ifdef _MSC_VER
// Windows MSVC has different packed struct alignment, skip size assertions
static_assert(sizeof(NetPacket_t) <= 2048U, "NetPacket_t too large");
#else
static_assert(sizeof(NetPacket_t) <= 2048U, "NetPacket_t too large");
static_assert(sizeof(MacEntry_t) == 16U, "MacEntry_t size mismatch");
static_assert(sizeof(ArpEntry_t) == 14U, "ArpEntry_t size mismatch");
static_assert(sizeof(EthHeader_t) == 14U, "EthHeader_t size mismatch");
static_assert(sizeof(VlanTag_t) == 4U, "VlanTag_t size mismatch");
static_assert(sizeof(ArpPacket_t) == 28U, "ArpPacket_t size mismatch");
#endif

/* ── Memory Barriers ─────────────────────────────────────── */
// MEMORY_BARRIER and related macros are defined in cross_platform.h

/* ── Cache Maintenance (Cortex-M7 only) ───────────────────── */

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    #define CACHE_CLEAN_ADDR(addr, size) \
        SCB_CleanDCache_by_Addr((uint32_t *)(addr), (size_t)(size))
    #define CACHE_INVALIDATE_ADDR(addr, size) \
        SCB_InvalidateDCache_by_Addr((uint32_t *)(addr), (size_t)(size))
#else
    #define CACHE_CLEAN_ADDR(addr, size)    ((void)0U)
    #define CACHE_INVALIDATE_ADDR(addr, size) ((void)0U)
#endif

/* ── Critical Section Macros ───────────────────────────── */

#define TASK_CRITICAL_ENTER()    taskENTER_CRITICAL()
#define TASK_CRITICAL_EXIT()     taskEXIT_CRITICAL()

#define ISR_CRITICAL_ENTER()     \
    UBaseType_t uxSavedStatus = taskENTER_CRITICAL_FROM_ISR()

#define ISR_CRITICAL_EXIT(uxSavedStatus) \
    taskEXIT_CRITICAL_FROM_ISR(uxSavedStatus)

/* ── Alignment Macros ───────────────────────────────────── */

#define ALIGN_UP(size, align)    (((size) + (align) - 1U) & ~((align) - 1U))
#define ALIGN_DOWN(size, align)  ((size) & ~((align) - 1U))
#define IS_ALIGNED(ptr, align)   (((uintptr_t)(ptr) & ((align) - 1U)) == 0U)
