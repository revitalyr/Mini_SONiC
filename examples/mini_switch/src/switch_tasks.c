#include "switch_core.h"
#include "switch_hal.h"
#include <string.h>
#include <stdio.h>

/* ── Task Handles and Stack Memory ───────────────────────── */

static TaskHandle_t s_rx_task_handle = NULL;
static TaskHandle_t s_worker_task_handle = NULL;
static TaskHandle_t s_cli_task_handle = NULL;
static TaskHandle_t s_tx_task_handle = NULL;

/* Static task stack allocation */
static StackType_t s_rx_task_stack[RX_TASK_STACK_SIZE];
static StaticTask_t s_rx_task_tcb;

static StackType_t s_worker_task_stack[WORKER_TASK_STACK_SIZE];
static StaticTask_t s_worker_task_tcb;

static StackType_t s_cli_task_stack[CLI_TASK_STACK_SIZE];
static StaticTask_t s_cli_task_tcb;

static StackType_t s_tx_task_stack[WORKER_TASK_STACK_SIZE];
static StaticTask_t s_tx_task_tcb;

/* ── Global Switch Context ───────────────────────────────── */

static SwitchContext_t *g_switch_ctx = NULL;

/* ── CLI Command Buffer ─────────────────────────────────── */

static char s_cli_buffer[128U];
static uint8_t s_cli_buffer_pos = 0U;

/* ── Private Function Prototypes ───────────────────────── */

static void RxTask(void *pvParameters);
static void WorkerTask(void *pvParameters);
static void TxTask(void *pvParameters);
static void CliTask(void *pvParameters);
static void ProcessCliCommand(const char *command);
static void PrintMacTable(void);
static void PrintArpCache(void);
static void PrintStats(void);
static void PrintHelp(void);
static SwitchStatus_t InitializeSwitch(void);
static void DeinitializeSwitch(void);

/* ── Public Functions ─────────────────────────────────── */

SwitchStatus_t SwitchTasks_Initialize(void)
{
    /* Initialize switch core */
    SwitchStatus_t status = InitializeSwitch();
    if (status != SWITCH_STATUS_OK) {
        return status;
    }

    /* Create RX task */
    s_rx_task_handle = xTaskCreateStatic(RxTask,
                                         "SwitchRX",
                                         RX_TASK_STACK_SIZE,
                                         NULL,
                                         TASK_PRIORITY_HIGH,
                                         s_rx_task_stack,
                                         &s_rx_task_tcb);
    if (s_rx_task_handle == NULL) {
        DeinitializeSwitch();
        return SWITCH_STATUS_NO_MEMORY;
    }

    /* Create Worker task */
    s_worker_task_handle = xTaskCreateStatic(WorkerTask,
                                            "SwitchWorker",
                                            WORKER_TASK_STACK_SIZE,
                                            NULL,
                                            TASK_PRIORITY_NORMAL,
                                            s_worker_task_stack,
                                            &s_worker_task_tcb);
    if (s_worker_task_handle == NULL) {
        vTaskDelete(s_rx_task_handle);
        DeinitializeSwitch();
        return SWITCH_STATUS_NO_MEMORY;
    }

    /* Create TX task */
    s_tx_task_handle = xTaskCreateStatic(TxTask,
                                         "SwitchTX",
                                         WORKER_TASK_STACK_SIZE,
                                         NULL,
                                         TASK_PRIORITY_HIGH,
                                         s_tx_task_stack,
                                         &s_tx_task_tcb);
    if (s_tx_task_handle == NULL) {
        vTaskDelete(s_rx_task_handle);
        vTaskDelete(s_worker_task_handle);
        DeinitializeSwitch();
        return SWITCH_STATUS_NO_MEMORY;
    }

    /* Create CLI task */
    s_cli_task_handle = xTaskCreateStatic(CliTask,
                                         "SwitchCLI",
                                         CLI_TASK_STACK_SIZE,
                                         NULL,
                                         TASK_PRIORITY_LOW,
                                         s_cli_task_stack,
                                         &s_cli_task_tcb);
    if (s_cli_task_handle == NULL) {
        vTaskDelete(s_rx_task_handle);
        vTaskDelete(s_worker_task_handle);
        vTaskDelete(s_tx_task_handle);
        DeinitializeSwitch();
        return SWITCH_STATUS_NO_MEMORY;
    }

    return SWITCH_STATUS_OK;
}

void SwitchTasks_Shutdown(void)
{
    /* Delete all tasks */
    if (s_rx_task_handle != NULL) {
        vTaskDelete(s_rx_task_handle);
        s_rx_task_handle = NULL;
    }

    if (s_worker_task_handle != NULL) {
        vTaskDelete(s_worker_task_handle);
        s_worker_task_handle = NULL;
    }

    if (s_tx_task_handle != NULL) {
        vTaskDelete(s_tx_task_handle);
        s_tx_task_handle = NULL;
    }

    if (s_cli_task_handle != NULL) {
        vTaskDelete(s_cli_task_handle);
        s_cli_task_handle = NULL;
    }

    /* Deinitialize switch */
    DeinitializeSwitch();
}

/* ── Task Implementations ───────────────────────────────── */

static void RxTask(void *pvParameters)
{
    (void)pvParameters;

    NetPacket_t packet;
    SwitchStatus_t status;

    printf("Switch RX task started\n");

    for (;;) {
        /* Receive packet from HAL */
        uint16_t packet_len = NET_BUFFER_SIZE;
        status = SwitchHal_ReceiveFrame(packet.data, &packet_len, portMAX_DELAY);

        if (status == SWITCH_STATUS_OK) {
            packet.len = packet_len;
            packet.port_id = 0U; /* Single port for now */
            packet.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

            /* Send to worker queue */
            if (xQueueSend(g_switch_ctx->rx_queue, &packet, 0U) != pdTRUE) {
                /* Queue full - drop packet */
                g_switch_ctx->total_rx_errors++;
            }
        } else if (status == SWITCH_STATUS_TIMEOUT) {
            /* Timeout is normal - continue */
            continue;
        } else {
            /* Error */
            g_switch_ctx->total_rx_errors++;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void WorkerTask(void *pvParameters)
{
    (void)pvParameters;

    NetPacket_t packet;

    printf("Switch Worker task started\n");

    for (;;) {
        /* Wait for packet from RX queue */
        if (xQueueReceive(g_switch_ctx->rx_queue, &packet, portMAX_DELAY) == pdTRUE) {
            /* Process packet */
            SwitchStatus_t status = SwitchCore_ProcessPacket(g_switch_ctx, &packet);
            
            if (status != SWITCH_STATUS_OK) {
                g_switch_ctx->learning_discards++;
            }
        }
    }
}

static void TxTask(void *pvParameters)
{
    (void)pvParameters;

    NetPacket_t packet;

    printf("Switch TX task started\n");

    for (;;) {
        /* Wait for packet from TX queue */
        if (xQueueReceive(g_switch_ctx->tx_queue, &packet, portMAX_DELAY) == pdTRUE) {
            /* Send packet via HAL */
            SwitchStatus_t status = SwitchHal_SendFrame(packet.data, packet.len, 
                                                     PACKET_TIMEOUT_MS);
            
            if (status == SWITCH_STATUS_OK) {
                g_switch_ctx->total_tx_packets++;
            } else {
                g_switch_ctx->total_tx_errors++;
            }
        }
    }
}

static void CliTask(void *pvParameters)
{
    (void)pvParameters;

    printf("Switch CLI task started\n");
    printf("Type 'help' for available commands\n");

    for (;;) {
        printf("switch> ");
        fflush(stdout);

        /* Wait for user input */
        if (fgets(s_cli_buffer, sizeof(s_cli_buffer), stdin) != NULL) {
            /* Remove newline */
            size_t len = strlen(s_cli_buffer);
            if (len > 0U && s_cli_buffer[len - 1] == '\n') {
                s_cli_buffer[len - 1] = '\0';
                len--;
            }

            /* Process command */
            if (len > 0U) {
                ProcessCliCommand(s_cli_buffer);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CLI_POLL_PERIOD_MS));
    }
}

/* ── CLI Command Processing ───────────────────────────── */

static void ProcessCliCommand(const char *command)
{
    if (strncmp(command, "help", 4U) == 0) {
        PrintHelp();
    } else if (strncmp(command, "show mac", 8U) == 0) {
        PrintMacTable();
    } else if (strncmp(command, "show arp", 8U) == 0) {
        PrintArpCache();
    } else if (strncmp(command, "show stats", 10U) == 0) {
        PrintStats();
    } else if (strncmp(command, "clear mac", 9U) == 0) {
        SwitchStatus_t status = SwitchCore_FlushMacTable(g_switch_ctx);
        printf("MAC table cleared: %s\n", 
               (status == SWITCH_STATUS_OK) ? "OK" : "ERROR");
    } else if (strncmp(command, "learning on", 11U) == 0) {
        SwitchCore_SetLearningMode(g_switch_ctx, true);
        printf("MAC learning enabled\n");
    } else if (strncmp(command, "learning off", 12U) == 0) {
        SwitchCore_SetLearningMode(g_switch_ctx, false);
        printf("MAC learning disabled\n");
    } else if (strncmp(command, "forwarding on", 13U) == 0) {
        SwitchCore_SetForwardingMode(g_switch_ctx, true);
        printf("Packet forwarding enabled\n");
    } else if (strncmp(command, "forwarding off", 14U) == 0) {
        SwitchCore_SetForwardingMode(g_switch_ctx, false);
        printf("Packet forwarding disabled\n");
    } else {
        printf("Unknown command: %s\n", command);
        printf("Type 'help' for available commands\n");
    }
}

static void PrintMacTable(void)
{
    printf("\n=== MAC Address Table ===\n");
    printf("MAC Address         Port  VLAN  Age (s)\n");
    printf("------------------- ---- ---- ---------\n");

    /* This would need access to private MAC table data */
    /* For now, print summary */
    uint32_t mac_entries;
    SwitchCore_GetGlobalStats(g_switch_ctx, NULL, NULL, &mac_entries, NULL);
    printf("Total entries: %lu\n", (unsigned long)mac_entries);
    printf("========================\n\n");
}

static void PrintArpCache(void)
{
    printf("\n=== ARP Cache ===\n");
    printf("IP Address         MAC Address         Age (s)\n");
    printf("------------------ ------------------- ---------\n");

    /* This would need access to private ARP table data */
    /* For now, print summary */
    uint32_t arp_entries;
    SwitchCore_GetGlobalStats(g_switch_ctx, NULL, NULL, NULL, &arp_entries);
    printf("Total entries: %lu\n", (unsigned long)arp_entries);
    printf("===================\n\n");
}

static void PrintStats(void)
{
    uint32_t total_rx_packets, total_tx_packets;
    uint32_t mac_entries, arp_entries;

    SwitchStatus_t status = SwitchCore_GetGlobalStats(g_switch_ctx,
                                                    &total_rx_packets,
                                                    &total_tx_packets,
                                                    &mac_entries,
                                                    &arp_entries);
    if (status != SWITCH_STATUS_OK) {
        printf("Error getting statistics\n");
        return;
    }

    printf("\n=== Switch Statistics ===\n");
    printf("Total RX packets: %lu\n", (unsigned long)total_rx_packets);
    printf("Total TX packets: %lu\n", (unsigned long)total_tx_packets);
    printf("RX errors: %lu\n", (unsigned long)g_switch_ctx->total_rx_errors);
    printf("TX errors: %lu\n", (unsigned long)g_switch_ctx->total_tx_errors);
    printf("Learning discards: %lu\n", (unsigned long)g_switch_ctx->learning_discards);
    printf("Forwarding discards: %lu\n", (unsigned long)g_switch_ctx->forwarding_discards);
    printf("MAC table entries: %lu\n", (unsigned long)mac_entries);
    printf("ARP cache entries: %lu\n", (unsigned long)arp_entries);
    printf("========================\n\n");
}

static void PrintHelp(void)
{
    printf("\n=== Available Commands ===\n");
    printf("help              - Show this help\n");
    printf("show mac          - Show MAC address table\n");
    printf("show arp          - Show ARP cache\n");
    printf("show stats        - Show switch statistics\n");
    printf("clear mac         - Clear MAC address table\n");
    printf("learning on/off  - Enable/disable MAC learning\n");
    printf("forwarding on/off- Enable/disable packet forwarding\n");
    printf("========================\n\n");
}

/* ── Initialization Functions ───────────────────────────── */

static SwitchStatus_t InitializeSwitch(void)
{
    /* Initialize switch core */
    SwitchStatus_t status = SwitchCore_Init(&g_switch_ctx);
    if (status != SWITCH_STATUS_OK) {
        printf("Failed to initialize switch core: %d\n", status);
        return status;
    }

    /* Default MAC address for testing */
    uint8_t mac_addr[6U] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

    /* Initialize HAL */
    status = SwitchHal_Init(mac_addr);
    if (status != SWITCH_STATUS_OK) {
        printf("Failed to initialize switch HAL: %d\n", status);
        SwitchCore_Deinit(g_switch_ctx);
        return status;
    }

    /* Store MAC address in context */
    memcpy(g_switch_ctx->switch_mac_addr, mac_addr, 6U);

    /* Add port */
    status = SwitchCore_AddPort(g_switch_ctx, 0U, mac_addr);
    if (status != SWITCH_STATUS_OK) {
        printf("Failed to add port: %d\n", status);
        SwitchHal_Stop();
        SwitchCore_Deinit(g_switch_ctx);
        return status;
    }

    /* Start HAL */
    status = SwitchHal_Start();
    if (status != SWITCH_STATUS_OK) {
        printf("Failed to start switch HAL: %d\n", status);
        SwitchCore_Deinit(g_switch_ctx);
        return status;
    }

    printf("Switch initialized successfully\n");
    return SWITCH_STATUS_OK;
}

static void DeinitializeSwitch(void)
{
    if (g_switch_ctx != NULL) {
        SwitchHal_Stop();
        SwitchCore_Deinit(g_switch_ctx);
        g_switch_ctx = NULL;
    }
}
