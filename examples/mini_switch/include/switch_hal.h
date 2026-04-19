#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos_config.h"

/* ── STM32 Ethernet HAL Interface ───────────────────────── */

#if !defined(_WIN32)
    #if defined(STM32F4xx)
        #include "stm32f4xx_hal.h"
        #include "stm32f4xx_hal_eth.h"
    #elif defined(STM32F7xx)
        #include "stm32f7xx_hal.h"
        #include "stm32f7xx_hal_eth.h"
    #elif defined(STM32H7xx)
        #include "stm32h7xx_hal.h"
        #include "stm32h7xx_hal_eth.h"
    #elif defined(STM32L4xx)
        #include "stm32l4xx_hal.h"
        #include "stm32l4xx_hal_eth.h"
    #else
        #error "Unsupported STM32 family for Ethernet"
    #endif
#else
    // Windows: no STM32 HAL available, provide stubs
    #define ETH_MAC_BASE (0x40028000UL)
    // DMA_BUFFER_ALIGNMENT is defined in freertos_config.h
#endif

/* ── Register Access Macros ─────────────────────────────── */

/**
 * @brief Volatile register pointer with proper casting
 */
#define REG_PTR(base, offset) \
    ((volatile uint32_t *)((uintptr_t)(base) + (uintptr_t)(offset)))

/**
 * @brief Atomic register read
 */
#define REG_READ(reg) \
    (*((volatile uint32_t *)(reg)))

/**
 * @brief Atomic register write
 */
#define REG_WRITE(reg, value) \
    do { \
        MEMORY_BARRIER(); \
        (*((volatile uint32_t *)(reg))) = (uint32_t)(value); \
        MEMORY_BARRIER(); \
    } while(0U)

/**
 * @brief Atomic read-modify-write with critical section protection
 */
#define REG_MODIFY(reg, mask, set) \
    do { \
        TASK_CRITICAL_ENTER(); \
        const uint32_t temp = REG_READ(reg); \
        REG_WRITE(reg, (temp & ~(mask)) | (set)); \
        TASK_CRITICAL_EXIT(); \
    } while(0U)

/* ── Ethernet Peripheral Base Addresses ─────────────────── */

#if defined(STM32F4xx) || defined(STM32F7xx)
    #define ETH_MAC_BASE         (0x40028000UL)
    #define ETH_MMC_BASE         (0x40028100UL)
    #define ETH_DMA_BASE         (0x40029000UL)
    #define ETH_PTP_BASE         (0x40028700UL)
#elif defined(STM32H7xx)
    #define ETH_MAC_BASE         (0x40028000UL)
    #define ETH_MTL_BASE         (0x40028B00UL)
    #define ETH_DMA_BASE         (0x40029000UL)
    #define ETH_DMA_BASE_EXT     (0x4002A000UL)
#elif defined(STM32L4xx)
    #define ETH_DMA_BASE         (0x40028200UL)
    #define ETH_DMA_BASE_EXT     (0x40028300UL)
#endif

/* ── DMA Buffer Configuration ───────────────────────────── */

/**
 * @brief DMA descriptor structure (STM32 Ethernet)
 */
typedef struct {
    volatile uint32_t status;          /**< Status */
    uint32_t control;                  /**< Control and length */
    uint32_t buffer_addr;              /**< Buffer address */
    uint32_t desc_next;                /**< Next descriptor address */
    uint32_t reserved1;                /**< Reserved */
    uint32_t timestamp_low;            /**< Timestamp low */
    uint32_t timestamp_high;           /**< Timestamp high */
    uint32_t reserved2;                /**< Reserved */
} PACKED_STRUCT
#ifdef _MSC_VER
__declspec(align(32))
#else
__attribute__((aligned(DMA_BUFFER_ALIGNMENT)))
#endif
EthDmaDesc_t;

/**
 * @brief DMA buffer management structure
 */
typedef struct {
    uint8_t *data;                     /**< Buffer data pointer */
    uint32_t size;                     /**< Buffer size */
    EthDmaDesc_t *tx_desc;             /**< TX descriptor */
    EthDmaDesc_t *rx_desc;             /**< RX descriptor */
    uint32_t tx_index;                 /**< Current TX descriptor index */
    uint32_t rx_index;                 /**< Current RX descriptor index */
} EthDmaBuffers_t;

/* ── HAL Wrapper Functions ───────────────────────────────── */

/**
 * @brief Initialize Ethernet peripheral
 * @param[in] mac_addr MAC address for the interface
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_Init(const uint8_t *mac_addr);

/**
 * @brief Start Ethernet DMA transmission and reception
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_Start(void);

/**
 * @brief Stop Ethernet DMA
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_Stop(void);

/**
 * @brief Send Ethernet frame
 * @param[in] data Frame data pointer
 * @param[in] len Frame length in bytes
 * @param[in] timeout_ms Timeout in milliseconds
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_SendFrame(const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Receive Ethernet frame
 * @param[out] data Buffer for received data
 * @param[in,out] len Buffer size on input, received length on output
 * @param[in] timeout_ms Timeout in milliseconds
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_ReceiveFrame(uint8_t *data, uint16_t *len, uint32_t timeout_ms);

/**
 * @brief Get link status
 * @param[out] link_up True if link is up
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_GetLinkStatus(bool *link_up);

/**
 * @brief Configure MAC address filtering
 * @param[in] mac_addr MAC address to add to filter
 * @param[in] enable Enable/disable filtering
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_SetMacFilter(const uint8_t *mac_addr, bool enable);

/**
 * @brief Enable/disable promiscuous mode
 * @param[in] enable Enable promiscuous mode
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_SetPromiscuousMode(bool enable);

/**
 * @brief Ethernet interrupt handler (to be called from ISR)
 */
void SwitchHal_IRQHandler(void);

/**
 * @brief Get packet transmission statistics
 * @param[out] tx_packets Transmitted packet count
 * @param[out] tx_errors Transmission error count
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_GetTxStats(uint32_t *tx_packets, uint32_t *tx_errors);

/**
 * @brief Get packet reception statistics
 * @param[out] rx_packets Received packet count
 * @param[out] rx_errors Reception error count
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_GetRxStats(uint32_t *rx_packets, uint32_t *rx_errors);

/* ── Interrupt Priority Configuration ───────────────────── */

/**
 * @brief Configure Ethernet interrupt priority
 * @note Must be called before enabling interrupts
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_ConfigureInterrupts(void);

/* ── PHY Interface ─────────────────────────────────────── */

/**
 * @brief Initialize PHY chip
 * @param[in] phy_addr PHY address (0-31)
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_InitPhy(uint8_t phy_addr);

/**
 * @brief Read PHY register
 * @param[in] phy_addr PHY address
 * @param[in] reg_addr Register address
 * @param[out] value Register value
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_ReadPhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value);

/**
 * @brief Write PHY register
 * @param[in] phy_addr PHY address
 * @param[in] reg_addr Register address
 * @param[in] value Register value
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_WritePhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t value);

/**
 * @brief Get PHY link status and speed
 * @param[in] phy_addr PHY address
 * @param[out] link_up True if link is up
 * @param[out] speed_mb Link speed in Mbps
 * @param[out] duplex True if full duplex
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_GetPhyLinkStatus(uint8_t phy_addr, bool *link_up, 
                                         uint32_t *speed_mb, bool *duplex);

/* ── Debug and Diagnostics ───────────────────────────────── */

/**
 * @brief Dump Ethernet registers for debugging
 */
void SwitchHal_DumpRegisters(void);

/**
 * @brief Run Ethernet self-test
 * @param[out] test_result True if test passed
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchHal_RunSelfTest(bool *test_result);

/* ── Compile-time Assertions ───────────────────────────── */

static_assert(sizeof(EthDmaDesc_t) == 32U, "EthDmaDesc_t size mismatch");
static_assert(DMA_BUFFER_ALIGNMENT >= 32U, "DMA alignment too small");
