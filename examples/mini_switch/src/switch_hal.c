#include "switch_hal.h"
#include "switch_core.h"
#include <string.h>

// Windows: skip STM32 HAL implementation
#ifdef _WIN32

// Stub implementations for Windows
SwitchStatus_t SwitchHal_Init(const uint8_t *mac_addr) {
    (void)mac_addr;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_Start(void) {
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_Stop(void) {
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_SendFrame(const uint8_t *data, uint16_t len, uint32_t timeout_ms) {
    (void)data; (void)len; (void)timeout_ms;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ReceiveFrame(uint8_t *data, uint16_t *len, uint32_t timeout_ms) {
    (void)data; (void)len; (void)timeout_ms;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_GetLinkStatus(bool *link_up) {
    *link_up = true;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_SetMacFilter(const uint8_t *mac_addr, bool enable) {
    (void)mac_addr; (void)enable;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_SetPromiscuousMode(bool enable) {
    (void)enable;
    return SWITCH_STATUS_OK;
}

void SwitchHal_IRQHandler(void) {
}

SwitchStatus_t SwitchHal_GetTxStats(uint32_t *tx_packets, uint32_t *tx_errors) {
    *tx_packets = 0;
    *tx_errors = 0;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_GetRxStats(uint32_t *rx_packets, uint32_t *rx_errors) {
    *rx_packets = 0;
    *rx_errors = 0;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ConfigureInterrupts(void) {
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_InitPhy(uint8_t phy_addr) {
    (void)phy_addr;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ReadPhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value) {
    (void)phy_addr; (void)reg_addr; (void)value;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_WritePhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t value) {
    (void)phy_addr; (void)reg_addr; (void)value;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_GetPhyLinkStatus(uint8_t phy_addr, bool *link_up, uint32_t *speed_mb, bool *duplex) {
    (void)phy_addr;
    *link_up = true;
    *speed_mb = 1000;
    *duplex = true;
    return SWITCH_STATUS_OK;
}

void SwitchHal_DumpRegisters(void) {
}

SwitchStatus_t SwitchHal_RunSelfTest(bool *test_result) {
    *test_result = true;
    return SWITCH_STATUS_OK;
}

#else // !_WIN32 - STM32 implementation

/* ── Private Defines ───────────────────────────────────── */

#define ETH_PHY_ADDRESS            (0x00U)
#define ETH_AUTONEGOTIATION_TIMEOUT (5000U)

/* ── Private Variables ─────────────────────────────────── */

static ETH_HandleTypeDef s_eth_handle;
static EthDmaBuffers_t s_dma_buffers;
static volatile bool s_link_status = false;
static volatile bool s_tx_busy = false;

/* ── DMA Buffer Allocation (aligned) ───────────────────── */

#if defined(__CORTEX_M) && (__CORTEX_M >= 4U)
    #define DMA_BUFFER_SECTION __attribute__((section(".ram_d2")))
#else
    #define DMA_BUFFER_SECTION
#endif

/* TX/RX buffers with proper alignment */
static uint8_t DMA_BUFFER_SECTION s_rx_buffers[PACKET_QUEUE_DEPTH][NET_BUFFER_SIZE] 
    __attribute__((aligned(DMA_BUFFER_ALIGNMENT)));

static uint8_t DMA_BUFFER_SECTION s_tx_buffers[PACKET_QUEUE_DEPTH][NET_BUFFER_SIZE] 
    __attribute__((aligned(DMA_BUFFER_ALIGNMENT)));

/* DMA descriptors with proper alignment */
static EthDmaDesc_t DMA_BUFFER_SECTION s_rx_descs[PACKET_QUEUE_DEPTH] 
    __attribute__((aligned(DMA_BUFFER_ALIGNMENT)));

static EthDmaDesc_t DMA_BUFFER_SECTION s_tx_descs[PACKET_QUEUE_DEPTH] 
    __attribute__((aligned(DMA_BUFFER_ALIGNMENT)));

/* ── Private Function Prototypes ─────────────────────────── */

static void Hal_ErrorHandler(const char *file, uint32_t line);
static SwitchStatus_t Hal_InitDmaBuffers(void);
static SwitchStatus_t Hal_ConfigureMacFilters(void);
static void Hal_TxCpltCallback(ETH_HandleTypeDef *heth);
static void Hal_RxCpltCallback(ETH_HandleTypeDef *heth);
static void Hal_ErrorCallback(ETH_HandleTypeDef *heth);

/* ── Public Functions ───────────────────────────────────── */

SwitchStatus_t SwitchHal_Init(const uint8_t *mac_addr)
{
    if (mac_addr == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Reset ETH peripheral */
    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_ETH_FORCE_RESET();
    __HAL_RCC_ETH_RELEASE_RESET();

    /* Initialize HAL handle */
    s_eth_handle.Instance = ETH;
    s_eth_handle.Init.MACAddr = (uint32_t *)mac_addr;
    s_eth_handle.Init.MediaInterface = HAL_ETH_MII_INTERFACE;
    s_eth_handle.Init.RxDesc = s_rx_descs;
    s_eth_handle.Init.TxDesc = s_tx_descs;
    s_eth_handle.Init.RxBuffLen = NET_BUFFER_SIZE;
    s_eth_handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
    s_eth_handle.Init.BroadcastAddrReject = DISABLE;
    s_eth_handle.Init.AddrFiltering = ENABLE;
    s_eth_handle.Init.DmaArbitration = ETH_DMAARBITRATION_ROUNDROBIN_RXTX;
    s_eth_handle.Init.DropTcpIpChecksumErrorFrame = ENABLE;
    s_eth_handle.Init.ReceiveAll = DISABLE;
    s_eth_handle.Init.PauseForward = DISABLE;
    s_eth_handle.Init.BackwardReceiveAddr = DISABLE;
    s_eth_handle.Init.DelayedChecksumMode = DISABLE;

    /* Initialize HAL ETH */
    if (HAL_ETH_Init(&s_eth_handle) != HAL_OK) {
        return SWITCH_STATUS_ERROR;
    }

    /* Initialize DMA buffers */
    SwitchStatus_t status = Hal_InitDmaBuffers();
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Configure MAC filters */
    status = Hal_ConfigureMacFilters();
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Initialize PHY */
    status = SwitchHal_InitPhy(ETH_PHY_ADDRESS);
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Store MAC address */
    memcpy(s_eth_handle.Init.MACAddr, mac_addr, 6U);

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_Start(void)
{
    /* Start ETH DMA */
    if (HAL_ETH_Start(&s_eth_handle) != HAL_OK) {
        return SWITCH_STATUS_ERROR;
    }

    /* Enable interrupts */
    SwitchStatus_t status = SwitchHal_ConfigureInterrupts();
    if (status != SWITCH_STATUS_OK) {
        HAL_ETH_Stop(&s_eth_handle);
        return status;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_Stop(void)
{
    /* Disable interrupts */
    HAL_NVIC_DisableIRQ(ETH_IRQn);

    /* Stop ETH DMA */
    if (HAL_ETH_Stop(&s_eth_handle) != HAL_OK) {
        return SWITCH_STATUS_ERROR;
    }

    s_tx_busy = false;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_SendFrame(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    if (data == NULL || len == 0U || len > NET_BUFFER_SIZE) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (s_tx_busy) {
        return SWITCH_STATUS_BUFFER_FULL;
    }

    /* Find free TX buffer */
    uint32_t tx_index = s_dma_buffers.tx_index;
    uint8_t *tx_buffer = s_tx_buffers[tx_index];

    /* Copy frame to TX buffer */
    memcpy(tx_buffer, data, len);
    CACHE_CLEAN_ADDR(tx_buffer, len);

    /* Prepare TX descriptor */
    EthDmaDesc_t *tx_desc = &s_tx_descs[tx_index];
    tx_desc->control = len & ETH_DMATXNDESCRF_TBS1;
    tx_desc->status = ETH_DMATXNDESCRF_OWN | ETH_DMATXNDESCRF_FS | ETH_DMATXNDESCRF_LS;

    /* Update index */
    s_dma_buffers.tx_index = (tx_index + 1U) % PACKET_QUEUE_DEPTH;
    s_tx_busy = true;

    /* Start transmission */
    __DSB();
    ETH->DMATPDR = 0U; /* Resume DMA transmission */

    /* Wait for completion with timeout */
    uint32_t start_time = xTaskGetTickCount();
    while (s_tx_busy) {
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(timeout_ms)) {
            return SWITCH_STATUS_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ReceiveFrame(uint8_t *data, uint16_t *len, uint32_t timeout_ms)
{
    if (data == NULL || len == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    uint32_t start_time = xTaskGetTickCount();
    uint32_t rx_index = s_dma_buffers.rx_index;

    while ((xTaskGetTickCount() - start_time) <= pdMS_TO_TICKS(timeout_ms)) {
        EthDmaDesc_t *rx_desc = &s_rx_descs[rx_index];

        /* Check if descriptor is owned by CPU */
        if ((rx_desc->status & ETH_DMARXDESC_OWN) == 0U) {
            /* Frame received */
            if ((rx_desc->status & ETH_DMARXDESC_ES) == 0U) {
                uint16_t frame_len = (rx_desc->status & ETH_DMARXDESC_FL) >> 16U;
                
                if (frame_len <= *len && frame_len > 0U) {
                    /* Invalidate cache */
                    CACHE_INVALIDATE_ADDR(s_rx_buffers[rx_index], frame_len);
                    
                    /* Copy frame */
                    memcpy(data, s_rx_buffers[rx_index], frame_len);
                    *len = frame_len;

                    /* Return descriptor to DMA */
                    rx_desc->status = ETH_DMARXDESC_OWN;
                    s_dma_buffers.rx_index = (rx_index + 1U) % PACKET_QUEUE_DEPTH;

                    return SWITCH_STATUS_OK;
                }
            } else {
                /* Error in frame */
                rx_desc->status = ETH_DMARXDESC_OWN;
                s_dma_buffers.rx_index = (rx_index + 1U) % PACKET_QUEUE_DEPTH;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return SWITCH_STATUS_TIMEOUT;
}

SwitchStatus_t SwitchHal_GetLinkStatus(bool *link_up)
{
    if (link_up == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    bool phy_link_up;
    uint32_t speed_mb;
    bool duplex;

    SwitchStatus_t status = SwitchHal_GetPhyLinkStatus(ETH_PHY_ADDRESS, &phy_link_up, 
                                                      &speed_mb, &duplex);
    if (status == SWITCH_STATUS_OK) {
        *link_up = phy_link_up;
        s_link_status = phy_link_up;
    }

    return status;
}

SwitchStatus_t SwitchHal_SetMacFilter(const uint8_t *mac_addr, bool enable)
{
    if (mac_addr == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (enable) {
        /* Add MAC address to filter */
        uint32_t mac_high = ((uint32_t)mac_addr[5] << 8U) | mac_addr[4];
        uint32_t mac_low = ((uint32_t)mac_addr[3] << 24U) | 
                         ((uint32_t)mac_addr[2] << 16U) |
                         ((uint32_t)mac_addr[1] << 8U) | mac_addr[0];

        ETH->MACA0HR = mac_high | ETH_MACA0HR_AE;
        ETH->MACA0LR = mac_low;
    } else {
        /* Disable MAC filter */
        ETH->MACA0HR = 0U;
        ETH->MACA0LR = 0U;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_SetPromiscuousMode(bool enable)
{
    if (enable) {
        ETH->MACFFR |= ETH_MACFFR_PM;
    } else {
        ETH->MACFFR &= ~ETH_MACFFR_PM;
    }

    return SWITCH_STATUS_OK;
}

void SwitchHal_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&s_eth_handle);
}

SwitchStatus_t SwitchHal_GetTxStats(uint32_t *tx_packets, uint32_t *tx_errors)
{
    if (tx_packets == NULL || tx_errors == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    *tx_packets = ETH->MMCTGFSCCR; /* Good frame count */
    *tx_errors = ETH->MMCTGFSCCR - ETH->MMCTGFCR; /* Failed frame count */

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_GetRxStats(uint32_t *rx_packets, uint32_t *rx_errors)
{
    if (rx_packets == NULL || rx_errors == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    *rx_packets = ETH->MMCRFCECR; /* Good frame count */
    *rx_errors = ETH->MMCRGUFCR; /* Undersize/good frame count */

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ConfigureInterrupts(void)
{
    /* Configure ETH interrupt priority */
    HAL_NVIC_SetPriority(ETH_IRQn, MAX_SYSCALL_INTERRUPT_PRIORITY, 0U);
    
    /* Enable ETH interrupt */
    HAL_NVIC_EnableIRQ(ETH_IRQn);

    /* Enable specific ETH interrupts */
    __HAL_ETH_ENABLE_IT(&s_eth_handle, ETH_IT_ALL);

    return SWITCH_STATUS_OK;
}

/* ── PHY Interface Functions ───────────────────────────── */

SwitchStatus_t SwitchHal_InitPhy(uint8_t phy_addr)
{
    uint32_t timeout = ETH_AUTONEGOTIATION_TIMEOUT;

    /* Reset PHY */
    SwitchStatus_t status = SwitchHal_WritePhyReg(phy_addr, PHY_BCR, PHY_RESET);
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Wait for reset completion */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Start auto-negotiation */
    status = SwitchHal_WritePhyReg(phy_addr, PHY_BCR, PHY_AUTONEGOTIATION);
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Wait for auto-negotiation completion */
    uint16_t phy_reg;
    do {
        status = SwitchHal_ReadPhyReg(phy_addr, PHY_BSR, &phy_reg);
        if (status != SWITCH_STATUS_OK) {
            return status;
        }

        if ((phy_reg & PHY_AUTONEGO_COMPLETE) != 0U) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        timeout -= 100U;

    } while (timeout > 0U);

    if (timeout == 0U) {
        return SWITCH_STATUS_TIMEOUT;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_ReadPhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value)
{
    if (value == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (HAL_ETH_ReadPHYRegister(&s_eth_handle, phy_addr, reg_addr, value) != HAL_OK) {
        return SWITCH_STATUS_ERROR;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_WritePhyReg(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    if (HAL_ETH_WritePHYRegister(&s_eth_handle, phy_addr, reg_addr, value) != HAL_OK) {
        return SWITCH_STATUS_ERROR;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchHal_GetPhyLinkStatus(uint8_t phy_addr, bool *link_up, 
                                         uint32_t *speed_mb, bool *duplex)
{
    if (link_up == NULL || speed_mb == NULL || duplex == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    uint16_t phy_reg;
    SwitchStatus_t status = SwitchHal_ReadPhyReg(phy_addr, PHY_BSR, &phy_reg);
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    *link_up = ((phy_reg & PHY_LINKED_STATUS) != 0U);

    if (*link_up) {
        /* Read PHY status register for speed and duplex */
        status = SwitchHal_ReadPhyReg(phy_addr, PHY_PHYSCSR, &phy_reg);
        if (status != SWITCH_STATUS_OK) {
            return status;
        }

        uint16_t speed_bits = phy_reg & PHY_SPEED_STATUS;
        switch (speed_bits) {
            case PHY_SPEED_STATUS_10:
                *speed_mb = 10U;
                break;
            case PHY_SPEED_STATUS_100:
                *speed_mb = 100U;
                break;
            case PHY_SPEED_STATUS_1000:
                *speed_mb = 1000U;
                break;
            default:
                *speed_mb = 0U;
                break;
        }

        *duplex = ((phy_reg & PHY_DUPLEX_STATUS) != 0U);
    } else {
        *speed_mb = 0U;
        *duplex = false;
    }

    return SWITCH_STATUS_OK;
}

/* ── Debug Functions ───────────────────────────────────── */

void SwitchHal_DumpRegisters(void)
{
    /* This would dump all ETH registers for debugging */
    /* Implementation depends on specific debugging requirements */
}

SwitchStatus_t SwitchHal_RunSelfTest(bool *test_result)
{
    if (test_result == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Basic self-test: check PHY communication */
    uint16_t phy_id1, phy_id2;
    SwitchStatus_t status = SwitchHal_ReadPhyReg(ETH_PHY_ADDRESS, PHY_ID1, &phy_id1);
    if (status != SWITCH_STATUS_OK) {
        *test_result = false;
        return status;
    }

    status = SwitchHal_ReadPhyReg(ETH_PHY_ADDRESS, PHY_ID2, &phy_id2);
    if (status != SWITCH_STATUS_OK) {
        *test_result = false;
        return status;
    }

    /* Check for valid PHY ID */
    *test_result = ((phy_id1 != 0xFFFFU) && (phy_id1 != 0x0000U) && 
                   (phy_id2 != 0xFFFFU) && (phy_id2 != 0x0000U));

    return SWITCH_STATUS_OK;
}

/* ── Private Functions ─────────────────────────────────── */

static SwitchStatus_t Hal_InitDmaBuffers(void)
{
    /* Initialize RX descriptors */
    for (uint32_t i = 0U; i < PACKET_QUEUE_DEPTH; i++) {
        s_rx_descs[i].status = ETH_DMARXDESC_OWN;
        s_rx_descs[i].control = NET_BUFFER_SIZE;
        s_rx_descs[i].buffer_addr = (uint32_t)s_rx_buffers[i];
        s_rx_descs[i].desc_next = (uint32_t)&s_rx_descs[(i + 1U) % PACKET_QUEUE_DEPTH];
    }

    /* Initialize TX descriptors */
    for (uint32_t i = 0U; i < PACKET_QUEUE_DEPTH; i++) {
        s_tx_descs[i].status = 0U;
        s_tx_descs[i].control = 0U;
        s_tx_descs[i].buffer_addr = (uint32_t)s_tx_buffers[i];
        s_tx_descs[i].desc_next = (uint32_t)&s_tx_descs[(i + 1U) % PACKET_QUEUE_DEPTH];
    }

    /* Initialize buffer management */
    s_dma_buffers.data = NULL;
    s_dma_buffers.size = 0U;
    s_dma_buffers.tx_desc = s_tx_descs;
    s_dma_buffers.rx_desc = s_rx_descs;
    s_dma_buffers.tx_index = 0U;
    s_dma_buffers.rx_index = 0U;

    /* Set DMA descriptor addresses */
    ETH->DMARDLAR = (uint32_t)s_rx_descs;
    ETH->DMATDLAR = (uint32_t)s_tx_descs;

    return SWITCH_STATUS_OK;
}

static SwitchStatus_t Hal_ConfigureMacFilters(void)
{
    /* Configure MAC frame filter */
    ETH->MACFFR = ETH_MACFFR_HPF | ETH_MACFFR_HMC;

    /* Configure perfect filter for unicast */
    ETH->MACHTHR = 0U;
    ETH->MACHTLR = 0U;

    return SWITCH_STATUS_OK;
}

static void Hal_ErrorHandler(const char *file, uint32_t line)
{
    /* Error handling implementation */
    /* Could log error, trigger reset, etc. */
    (void)file;
    (void)line;
    
    /* Disable interrupts and stop */
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Halt */
    }
}

/* ── HAL Callbacks ─────────────────────────────────────── */

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
    (void)heth;
    s_tx_busy = false;
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    (void)heth;
    /* RX complete - packet is ready for processing */
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
    (void)heth;
    /* Handle ETH errors */
}

#endif // _WIN32
