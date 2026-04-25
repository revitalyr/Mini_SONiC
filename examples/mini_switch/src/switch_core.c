#include "freertos_config.h"
#include "switch_core.h"
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

/* ── Private Constants ─────────────────────────────────── */

#define MAC_AGEING_TIME_MS        (300000U)  /* 5 minutes */
#define ARP_AGEING_TIME_MS        (60000U)   /* 1 minute */
#define MAINTENANCE_INTERVAL_MS    (10000U)   /* 10 seconds */
#define BROADCAST_MAC_ADDR         {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define IPV4_BROADCAST_ADDR       (0xFFFFFFFFU)

/* ── Private Function Prototypes ───────────────────────── */

static SwitchStatus_t Core_LearnMacAddress(SwitchContext_t *ctx, 
                                          const uint8_t *mac_addr, 
                                          uint8_t port_id, uint16_t vlan_id);
static SwitchStatus_t Core_LookupMacAddress(SwitchContext_t *ctx, 
                                           const uint8_t *mac_addr, 
                                           uint8_t *port_id);
static SwitchStatus_t Core_ProcessArpPacket(SwitchContext_t *ctx, 
                                          const NetPacket_t *packet);
static SwitchStatus_t Core_ProcessIpPacket(SwitchContext_t *ctx, 
                                         const NetPacket_t *packet);
static SwitchStatus_t Core_ForwardPacket(SwitchContext_t *ctx, 
                                        const NetPacket_t *packet, 
                                        uint8_t ingress_port);
static SwitchStatus_t Core_FloodPacket(SwitchContext_t *ctx, 
                                      const NetPacket_t *packet, 
                                      uint8_t exclude_port);
static bool Core_IsBroadcastMac(const uint8_t *mac_addr);
static bool Core_IsMulticastMac(const uint8_t *mac_addr);
static bool Core_IsValidMac(const uint8_t *mac_addr);
static void Core_MaintenanceCallback(TimerHandle_t xTimer);
static uint32_t Core_GetCurrentTimeMs(void);

/* ── Public Functions ─────────────────────────────────── */

SwitchStatus_t SwitchCore_Init(SwitchContext_t **ctx)
{
    if (ctx == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Allocate context */
    static SwitchContext_t s_switch_context;
    SwitchContext_t *context = &s_switch_context;

    /* Clear context */
    memset(context, 0U, sizeof(SwitchContext_t));

    /* Initialize FreeRTOS objects */
    context->rx_queue = xQueueCreateStatic(PACKET_QUEUE_DEPTH, sizeof(NetPacket_t),
                                         context->rx_queue_storage, 
                                         &context->rx_queue_struct);
    if (context->rx_queue == (QueueHandle_t)0) {
        return SWITCH_STATUS_NO_MEMORY;
    }

    context->tx_queue = xQueueCreateStatic(PACKET_QUEUE_DEPTH, sizeof(NetPacket_t),
                                         context->tx_queue_storage, 
                                         &context->tx_queue_struct);
    if (context->tx_queue == (QueueHandle_t)0) {
        return SWITCH_STATUS_NO_MEMORY;
    }

    context->mac_mutex = xSemaphoreCreateMutexStatic(&context->mac_mutex_struct);
    if (context->mac_mutex == (SemaphoreHandle_t)0) {
        return SWITCH_STATUS_NO_MEMORY;
    }

    context->arp_mutex = xSemaphoreCreateMutexStatic(&context->arp_mutex_struct);
    if (context->arp_mutex == (SemaphoreHandle_t)0) {
        return SWITCH_STATUS_NO_MEMORY;
    }

    /* Initialize configuration */
    context->learning_enabled = true;
    context->forwarding_enabled = true;
    context->mac_aging_time_ms = MAC_AGEING_TIME_MS;
    context->arp_aging_time_ms = ARP_AGEING_TIME_MS;
    context->system_start_time_ms = Core_GetCurrentTimeMs();

    /* Create maintenance timer */
    context->maintenance_timer = xTimerCreateStatic("SwitchMnt",
                                                  pdMS_TO_TICKS(MAINTENANCE_INTERVAL_MS),
                                                  pdTRUE,
                                                  context,
                                                  Core_MaintenanceCallback,
                                                  &context->maintenance_timer_struct);
    if (context->maintenance_timer == (TimerHandle_t)0) {
        return SWITCH_STATUS_NO_MEMORY;
    }

    /* Start maintenance timer */
    if (xTimerStart(context->maintenance_timer, 0U) != pdPASS) {
        return SWITCH_STATUS_ERROR;
    }

    *ctx = context;
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_Deinit(SwitchContext_t *ctx)
{
    if (ctx == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Stop timer */
    if (ctx->maintenance_timer != NULL) {
        xTimerStop(ctx->maintenance_timer, 0U);
        xTimerDelete(ctx->maintenance_timer, 0U);
    }

    /* Delete queues */
    if (ctx->rx_queue != NULL) {
        vQueueDelete(ctx->rx_queue);
    }
    if (ctx->tx_queue != NULL) {
        vQueueDelete(ctx->tx_queue);
    }

    /* Delete mutexes */
    if (ctx->mac_mutex != NULL) {
        vSemaphoreDelete(ctx->mac_mutex);
    }
    if (ctx->arp_mutex != NULL) {
        vSemaphoreDelete(ctx->arp_mutex);
    }

    /* Clear context */
    memset(ctx, 0U, sizeof(SwitchContext_t));

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_ProcessPacket(SwitchContext_t *ctx, const NetPacket_t *packet)
{
    if (ctx == NULL || packet == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (packet->len < sizeof(EthHeader_t)) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Update statistics */
    ctx->total_rx_packets++;

    /* Parse Ethernet header */
    const EthHeader_t *eth_hdr = (const EthHeader_t *)packet->data;
    
    /* Validate MAC addresses */
    if (!Core_IsValidMac(eth_hdr->src_addr) || !Core_IsValidMac(eth_hdr->dest_addr)) {
        ctx->total_rx_errors++;
        return SWITCH_STATUS_ERROR;
    }

    /* Learn source MAC address */
    if (ctx->learning_enabled) {
        Core_LearnMacAddress(ctx, eth_hdr->src_addr, packet->port_id, 0U);
    }

    /* Process based on EtherType */
    uint16_t ethertype = ntohs(eth_hdr->ethertype);
    
    switch (ethertype) {
        case ETH_TYPE_ARP:
            return Core_ProcessArpPacket(ctx, packet);
            
        case ETH_TYPE_IP:
            return Core_ProcessIpPacket(ctx, packet);
            
        case ETH_TYPE_VLAN:
            /* Handle VLAN tagged packets */
            /* Implementation would parse VLAN tag and process inner packet */
            break;
            
        default:
            /* Unknown EtherType - forward if enabled */
            break;
    }

    /* Forward packet */
    if (ctx->forwarding_enabled) {
        return Core_ForwardPacket(ctx, packet, packet->port_id);
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_AddPort(SwitchContext_t *ctx, uint8_t port_id, 
                                  const uint8_t *mac_addr)
{
    if (ctx == NULL || mac_addr == NULL || port_id >= SWITCH_MAX_PORTS) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    /* Check if port already exists */
    for (uint8_t i = 0U; i < ctx->port_count; i++) {
        if (ctx->ports[i].port_id == port_id) {
            return SWITCH_STATUS_ERROR; /* Port already exists */
        }
    }

    if (ctx->port_count >= SWITCH_MAX_PORTS) {
        return SWITCH_STATUS_ERROR; /* Maximum ports reached */
    }

    /* Add new port */
    SwitchPort_t *port = &ctx->ports[ctx->port_count];
    port->port_id = port_id;
    port->enabled = true;
    port->vlan_id = 1U; /* Default VLAN */
    port->rx_packets = 0U;
    port->tx_packets = 0U;
    port->rx_errors = 0U;
    port->tx_errors = 0U;

    ctx->port_count++;

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_GetMacEntry(SwitchContext_t *ctx, const uint8_t *mac_addr, 
                                     uint8_t *port_id)
{
    if (ctx == NULL || mac_addr == NULL || port_id == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    return Core_LookupMacAddress(ctx, mac_addr, port_id);
}

SwitchStatus_t SwitchCore_AddMacEntry(SwitchContext_t *ctx, const uint8_t *mac_addr, 
                                    uint8_t port_id, uint16_t vlan_id)
{
    if (ctx == NULL || mac_addr == NULL || port_id >= SWITCH_MAX_PORTS) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    return Core_LearnMacAddress(ctx, mac_addr, port_id, vlan_id);
}

SwitchStatus_t SwitchCore_GetArpEntry(SwitchContext_t *ctx, uint32_t ip_addr, 
                                     uint8_t *mac_addr)
{
    if (ctx == NULL || mac_addr == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (xSemaphoreTake(ctx->arp_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return SWITCH_STATUS_TIMEOUT;
    }

    SwitchStatus_t status = SWITCH_STATUS_NOT_FOUND;
    uint32_t current_time = Core_GetCurrentTimeMs();

    for (uint16_t i = 0U; i < ctx->arp_table_size; i++) {
        const ArpEntry_t *entry = &ctx->arp_table[i];
        
        if (entry->valid && (entry->ip_addr == ip_addr)) {
            /* Check if entry is still valid */
            if ((current_time - entry->last_seen_ms) < ctx->arp_aging_time_ms) {
                memcpy(mac_addr, entry->mac_addr, 6U);
                status = SWITCH_STATUS_OK;
            }
            break;
        }
    }

    xSemaphoreGive(ctx->arp_mutex);
    return status;
}

SwitchStatus_t SwitchCore_AddArpEntry(SwitchContext_t *ctx, uint32_t ip_addr, 
                                     const uint8_t *mac_addr)
{
    if (ctx == NULL || mac_addr == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (xSemaphoreTake(ctx->arp_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return SWITCH_STATUS_TIMEOUT;
    }

    /* Look for existing entry */
    bool found = false;
    uint32_t current_time = Core_GetCurrentTimeMs();

    for (uint16_t i = 0U; i < ctx->arp_table_size; i++) {
        ArpEntry_t *entry = &ctx->arp_table[i];
        
        if (entry->valid && (entry->ip_addr == ip_addr)) {
            /* Update existing entry */
            memcpy(entry->mac_addr, mac_addr, 6U);
            entry->last_seen_ms = current_time;
            found = true;
            break;
        }
    }

    /* Add new entry if not found */
    if (!found && ctx->arp_table_size < ARP_TABLE_SIZE) {
        ArpEntry_t *entry = &ctx->arp_table[ctx->arp_table_size];
        entry->ip_addr = ip_addr;
        memcpy(entry->mac_addr, mac_addr, 6U);
        entry->last_seen_ms = current_time;
        entry->valid = true;
        ctx->arp_table_size++;
    }

    xSemaphoreGive(ctx->arp_mutex);
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_SendArpRequest(SwitchContext_t *ctx, uint32_t target_ip,
                                        uint32_t source_ip, uint8_t port_id)
{
    if (ctx == NULL || port_id >= ctx->port_count) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    NetPacket_t packet;
    memset(&packet, 0U, sizeof(NetPacket_t));
    packet.port_id = port_id;
    packet.timestamp_ms = Core_GetCurrentTimeMs();

    /* Build Ethernet header */
    EthHeader_t *eth_hdr = (EthHeader_t *)packet.data;
    memset(eth_hdr->dest_addr, 0xFF, 6U); /* Broadcast */
    memcpy(eth_hdr->src_addr, ctx->switch_mac_addr, 6U);
    eth_hdr->ethertype = htons(ETH_TYPE_ARP);

    /* Build ARP packet */
    ArpPacket_t *arp = (ArpPacket_t *)(packet.data + sizeof(EthHeader_t));
    arp->htype = htons(ARP_HTYPE_ETHER);
    arp->ptype = htons(ARP_PTYPE_IP);
    arp->hlen = 6U;
    arp->plen = 4U;
    arp->oper = htons(ARP_OPER_REQUEST);
    memcpy(arp->sha, ctx->switch_mac_addr, 6U);
    arp->spa = source_ip;
    memset(arp->tha, 0U, 6U);
    arp->tpa = target_ip;

    packet.len = sizeof(EthHeader_t) + sizeof(ArpPacket_t);

    /* Send packet */
    if (xQueueSend(ctx->tx_queue, &packet, pdMS_TO_TICKS(PACKET_TIMEOUT_MS)) != pdTRUE) {
        return SWITCH_STATUS_BUFFER_FULL;
    }

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_FlushMacTable(SwitchContext_t *ctx)
{
    if (ctx == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    if (xSemaphoreTake(ctx->mac_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return SWITCH_STATUS_TIMEOUT;
    }

    ctx->mac_table_size = 0U;
    memset(ctx->mac_table, 0U, sizeof(ctx->mac_table));

    xSemaphoreGive(ctx->mac_mutex);
    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_GetGlobalStats(SwitchContext_t *ctx,
                                        uint32_t *total_rx_packets,
                                        uint32_t *total_tx_packets,
                                        uint32_t *mac_table_entries,
                                        uint32_t *arp_cache_entries)
{
    if (ctx == NULL || total_rx_packets == NULL || total_tx_packets == NULL ||
        mac_table_entries == NULL || arp_cache_entries == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    *total_rx_packets = ctx->total_rx_packets;
    *total_tx_packets = ctx->total_tx_packets;
    *mac_table_entries = ctx->mac_table_size;
    *arp_cache_entries = ctx->arp_table_size;

    return SWITCH_STATUS_OK;
}

SwitchStatus_t SwitchCore_PeriodicMaintenance(SwitchContext_t *ctx)
{
    if (ctx == NULL) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    uint32_t current_time = Core_GetCurrentTimeMs();

    /* Age out MAC entries */
    if (xSemaphoreTake(ctx->mac_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint16_t new_size = 0U;
        
        for (uint16_t i = 0U; i < ctx->mac_table_size; i++) {
            MacEntry_t *entry = &ctx->mac_table[i];
            
            if (entry->valid && 
                ((current_time - entry->last_seen_ms) < ctx->mac_aging_time_ms)) {
                /* Keep valid entry */
                if (i != new_size) {
                    ctx->mac_table[new_size] = *entry;
                }
                new_size++;
            }
        }
        
        ctx->mac_table_size = new_size;
        xSemaphoreGive(ctx->mac_mutex);
    }

    /* Age out ARP entries */
    if (xSemaphoreTake(ctx->arp_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint16_t new_size = 0U;
        
        for (uint16_t i = 0U; i < ctx->arp_table_size; i++) {
            ArpEntry_t *entry = &ctx->arp_table[i];
            
            if (entry->valid && 
                ((current_time - entry->last_seen_ms) < ctx->arp_aging_time_ms)) {
                /* Keep valid entry */
                if (i != new_size) {
                    ctx->arp_table[new_size] = *entry;
                }
                new_size++;
            }
        }
        
        ctx->arp_table_size = new_size;
        xSemaphoreGive(ctx->arp_mutex);
    }

    return SWITCH_STATUS_OK;
}

/* ── Private Functions ───────────────────────────────── */

static SwitchStatus_t Core_LearnMacAddress(SwitchContext_t *ctx, 
                                          const uint8_t *mac_addr, 
                                          uint8_t port_id, uint16_t vlan_id)
{
    /* Skip broadcast and multicast addresses */
    if (Core_IsBroadcastMac(mac_addr) || Core_IsMulticastMac(mac_addr)) {
        return SWITCH_STATUS_OK;
    }

    if (xSemaphoreTake(ctx->mac_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return SWITCH_STATUS_TIMEOUT;
    }

    uint32_t current_time = Core_GetCurrentTimeMs();
    bool found = false;

    /* Look for existing entry */
    for (uint16_t i = 0U; i < ctx->mac_table_size; i++) {
        MacEntry_t *entry = &ctx->mac_table[i];
        
        if (entry->valid && (memcmp(entry->mac_addr, mac_addr, 6U) == 0)) {
            /* Update existing entry */
            entry->port_id = port_id;
            entry->vlan_id = vlan_id;
            entry->last_seen_ms = current_time;
            found = true;
            break;
        }
    }

    /* Add new entry if not found and space available */
    if (!found && ctx->mac_table_size < MAC_TABLE_SIZE) {
        MacEntry_t *entry = &ctx->mac_table[ctx->mac_table_size];
        memcpy(entry->mac_addr, mac_addr, 6U);
        entry->port_id = port_id;
        entry->vlan_id = vlan_id;
        entry->last_seen_ms = current_time;
        entry->valid = true;
        ctx->mac_table_size++;
    }

    xSemaphoreGive(ctx->mac_mutex);
    return SWITCH_STATUS_OK;
}

static SwitchStatus_t Core_LookupMacAddress(SwitchContext_t *ctx, 
                                           const uint8_t *mac_addr, 
                                           uint8_t *port_id)
{
    if (xSemaphoreTake(ctx->mac_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return SWITCH_STATUS_TIMEOUT;
    }

    SwitchStatus_t status = SWITCH_STATUS_NOT_FOUND;
    uint32_t current_time = Core_GetCurrentTimeMs();

    for (uint16_t i = 0U; i < ctx->mac_table_size; i++) {
        const MacEntry_t *entry = &ctx->mac_table[i];
        
        if (entry->valid && 
            (memcmp(entry->mac_addr, mac_addr, 6U) == 0) &&
            ((current_time - entry->last_seen_ms) < ctx->mac_aging_time_ms)) {
            
            *port_id = entry->port_id;
            status = SWITCH_STATUS_OK;
            break;
        }
    }

    xSemaphoreGive(ctx->mac_mutex);
    return status;
}

static SwitchStatus_t Core_ProcessArpPacket(SwitchContext_t *ctx, 
                                          const NetPacket_t *packet)
{
    if (packet->len < (sizeof(EthHeader_t) + sizeof(ArpPacket_t))) {
        return SWITCH_STATUS_INVALID_PARAM;
    }

    const ArpPacket_t *arp = (const ArpPacket_t *)(packet->data + sizeof(EthHeader_t));
    uint16_t oper = ntohs(arp->oper);

    if (oper == ARP_OPER_REQUEST) {
        /* Learn sender's ARP entry */
        SwitchCore_AddArpEntry(ctx, arp->spa, arp->sha);

        /* Send ARP reply if target is our IP */
        /* Implementation would check if arp->tpa matches our IP */
        /* For now, we'll just forward the request */
    } else if (oper == ARP_OPER_REPLY) {
        /* Learn from ARP reply */
        SwitchCore_AddArpEntry(ctx, arp->spa, arp->sha);
    }

    return SWITCH_STATUS_OK;
}

static SwitchStatus_t Core_ProcessIpPacket(SwitchContext_t *ctx, 
                                         const NetPacket_t *packet)
{
    /* Basic IP packet processing */
    /* Implementation would parse IP header and handle ICMP, etc. */
    return Core_ForwardPacket(ctx, packet, packet->port_id);
}

static SwitchStatus_t Core_ForwardPacket(SwitchContext_t *ctx, 
                                        const NetPacket_t *packet, 
                                        uint8_t ingress_port)
{
    const EthHeader_t *eth_hdr = (const EthHeader_t *)packet->data;

    /* Don't forward broadcast back to same port */
    if (Core_IsBroadcastMac(eth_hdr->dest_addr)) {
        return Core_FloodPacket(ctx, packet, ingress_port);
    }

    /* Lookup destination MAC */
    uint8_t egress_port;
    SwitchStatus_t status = Core_LookupMacAddress(ctx, eth_hdr->dest_addr, &egress_port);

    if (status == SWITCH_STATUS_OK) {
        /* Forward to specific port */
        if (egress_port != ingress_port) {
            NetPacket_t forward_packet = *packet;
            forward_packet.port_id = egress_port;

            if (xQueueSend(ctx->tx_queue, &forward_packet, 0U) != pdTRUE) {
                ctx->forwarding_discards++;
            }
        }
    } else {
        /* Flood to all ports except ingress */
        Core_FloodPacket(ctx, packet, ingress_port);
    }

    return SWITCH_STATUS_OK;
}

static SwitchStatus_t Core_FloodPacket(SwitchContext_t *ctx, 
                                      const NetPacket_t *packet, 
                                      uint8_t exclude_port)
{
    for (uint8_t i = 0U; i < ctx->port_count; i++) {
        if ((ctx->ports[i].enabled) && (i != exclude_port)) {
            NetPacket_t flood_packet = *packet;
            flood_packet.port_id = i;

            if (xQueueSend(ctx->tx_queue, &flood_packet, 0U) != pdTRUE) {
                ctx->forwarding_discards++;
            }
        }
    }

    return SWITCH_STATUS_OK;
}

static bool Core_IsBroadcastMac(const uint8_t *mac_addr)
{
    static const uint8_t broadcast_addr[6U] = BROADCAST_MAC_ADDR;
    return memcmp(mac_addr, broadcast_addr, 6U) == 0U;
}

static bool Core_IsMulticastMac(const uint8_t *mac_addr)
{
    return (mac_addr[0] & 0x01U) != 0U;
}

static bool Core_IsValidMac(const uint8_t *mac_addr)
{
    /* Check for all zeros or broadcast */
    if (Core_IsBroadcastMac(mac_addr)) {
        return false;
    }

    /* Check for all zeros */
    bool all_zeros = true;
    for (uint8_t i = 0U; i < 6U; i++) {
        if (mac_addr[i] != 0U) {
            all_zeros = false;
            break;
        }
    }

    return !all_zeros;
}

static void Core_MaintenanceCallback(TimerHandle_t xTimer)
{
    SwitchContext_t *ctx = (SwitchContext_t *)pvTimerGetTimerID(xTimer);
    if (ctx != NULL) {
        SwitchCore_PeriodicMaintenance(ctx);
    }
}

static uint32_t Core_GetCurrentTimeMs(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}
