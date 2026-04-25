/**
 * @file orchagent.c
 * @brief SONiC OrchAgent Implementation
 * 
 * OrchAgent is the central orchestration daemon in SONiC architecture.
 * It subscribes to Redis database changes and translates them to SAI calls.
 * 
 * Key Responsibilities:
 * - Subscribe to APP_DB changes via Redis pub/sub
 * - Translate configuration changes to SAI API calls
 * - Coordinate different orchestrators (FdbOrch, RouteOrch, etc.)
 * - Manage component lifecycle and error handling
 */

#include "sonic_architecture.h"
#include "sonic_types.h"
#include "cross_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <hiredis/hiredis.h>
#else
// Windows stubs for hiredis types
#ifndef HIREDIS_STUBS_H
#define HIREDIS_STUBS_H
#define REDIS_REPLY_ARRAY 1
#define REDIS_REPLY_STRING 2
typedef struct redisReply {
    int type;
    size_t elements;
    struct redisReply **element;
    char *str;
    int integer;
} redisReply;
typedef struct redisContext {
    int err;
    char errstr[128];
    int fd;
} redisContext;
static inline redisContext* redisConnect(const char *ip, int port) {
    (void)ip; (void)port;
    return NULL;
}
static inline void redisFree(redisContext *c) { (void)c; }
static inline redisReply* redisCommand(redisContext *c, const char *fmt, ...) {
    (void)c; (void)fmt;
    return NULL;
}
static inline void freeReplyObject(redisReply *r) { (void)r; }
static inline void redisSetTimeout(redisContext *c, int sec, int us) { (void)c; (void)sec; (void)us; }
#endif
#endif

// =============================================================================
// REDIS COMMUNICATION
// =============================================================================

/**
 * @brief Redis context for database operations
 */
typedef struct {
    redisContext *connection;        ///< Redis connection
    redisContext *subscriptions[REDIS_DB_MAX]; ///< Subscription contexts
    char *db_names[REDIS_DB_MAX]; ///< Database names
} orch_redis_ctx;

/**
 * @brief Initialize Redis connection
 */
static int redis_init(orch_redis_ctx *ctx, const char *host, int port) {
    ctx->connection = redisConnect(host, port);
    if (ctx->connection == NULL || ctx->connection->err) {
        sonic_log(LOG_ERROR, "Failed to connect to Redis: %s", 
                   ctx->connection ? ctx->connection->errstr : "Unknown error");
        return -1;
    }
    
    // Initialize database names
    ctx->db_names[REDIS_DB_APP] = "APP_DB";
    ctx->db_names[REDIS_DB_ASIC] = "ASIC_DB";
    ctx->db_names[REDIS_DB_CONFIG] = "CONFIG_DB";
    ctx->db_names[REDIS_DB_COUNTERS] = "COUNTERS_DB";
    ctx->db_names[REDIS_DB_STATE] = "STATE_DB";
    
    sonic_log(LOG_INFO, "Connected to Redis at %s:%d", host, port);
    return 0;
}

/**
 * @brief Subscribe to Redis database changes
 */
static int redis_subscribe(orch_redis_ctx *ctx, redis_database_t db, const char *pattern) {
    redisReply *reply;
    
    reply = redisCommand(ctx->connection, "SUBSCRIBE %s", pattern);
    if (reply == NULL) {
        sonic_log(LOG_ERROR, "Failed to subscribe to %s", pattern);
        return -1;
    }
    
    freeReplyObject(reply);
    sonic_log(LOG_INFO, "Subscribed to %s in %s", pattern, ctx->db_names[db]);
    return 0;
}

// =============================================================================
// ORCHESTRATOR MANAGEMENT
// =============================================================================

/**
 * @brief OrchAgent context with all sub-orchestrators
 */
struct orchagent_context {
    orch_redis_ctx redis_ctx;     ///< Redis communication context
    fdb_orch_t fdb_orch;        ///< FDB orchestrator
    route_orch_t route_orch;      ///< Route orchestrator
    thread_t event_loop_thread;    ///< Event processing thread
    volatile bool running;           ///< Running status
    uint64_t packets_processed;     ///< Packets processed counter
    uint64_t packets_forwarded;     ///< Packets forwarded counter
};

static struct orchagent_context g_orchagent;

/**
 * @brief Process Redis message and route to appropriate orchestrator
 */
// Stub function declarations for missing orchestrator functions
static int fdb_orch_process_message(fdb_orch_t *orch, const char *payload) { (void)orch; (void)payload; return 0; }
static int route_orch_process_message(route_orch_t *orch, const char *payload) { (void)orch; (void)payload; return 0; }
static int neighbor_orch_process_message(const char *payload) { (void)payload; return 0; }
static void fdb_orch_cleanup(fdb_orch_t *orch) { (void)orch; }
static void route_orch_cleanup(route_orch_t *orch) { (void)orch; }
static void fdb_orch_get_counters(fdb_orch_t *orch, sonic_counters_t *counters) { (void)orch; (void)counters; }
static void route_orch_get_counters(route_orch_t *orch, sonic_counters_t *counters) { (void)orch; (void)counters; }

static int process_redis_message(redisReply *reply) {
    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        return -1;
    }
    
    // Parse message: [message_type, channel, payload]
    if (reply->elements < 3) {
        return -1;
    }
    
    char *channel = reply->element[1]->str;
    char *payload = reply->element[2]->str;
    
    sonic_log(LOG_DEBUG, "Received message on channel %s: %s", channel, payload);
    
    // Route to appropriate orchestrator based on channel
    if (strstr(channel, "FDB_TABLE")) {
        // Handle FDB changes
        return fdb_orch_process_message(&g_orchagent.fdb_orch, payload);
    } else if (strstr(channel, "ROUTE_TABLE")) {
        // Handle route changes
        return route_orch_process_message(&g_orchagent.route_orch, payload);
    } else if (strstr(channel, "NEIGH_TABLE")) {
        // Handle neighbor changes
        return neighbor_orch_process_message(payload);
    }
    
    return 0;
}

/**
 * @brief Main event loop for Redis subscription processing
 */
static THREAD_RETURN_TYPE THREAD_CALL event_loop_thread(void *arg) {
    struct orchagent_context *ctx = (struct orchagent_context*)arg;
    redisReply *reply;
    
    sonic_log(LOG_INFO, "OrchAgent event loop started");
    
    while (ctx->running) {
        // Wait for Redis messages with timeout
        redisSetTimeout(ctx->redis_ctx.connection, 1, 0);
        
        reply = redisCommand(ctx->redis_ctx.connection, "GETMESSAGE");
        if (reply != NULL) {
            process_redis_message(reply);
            freeReplyObject(reply);
        }
        
        SLEEP_MS(10); // 10ms sleep to prevent busy waiting
    }
    
    sonic_log(LOG_INFO, "OrchAgent event loop stopped");
    return (THREAD_RETURN_TYPE)0;
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

/**
 * @brief Initialize OrchAgent with all sub-orchestrators
 */
int orchagent_init(orchagent_t *orch) {
    struct orchagent_context *ctx = &g_orchagent;
    
    // Initialize Redis connection
    if (redis_init(&ctx->redis_ctx, "localhost", 6379) != 0) {
        return -1;
    }
    
    // Initialize sub-orchestrators
    if (fdb_orch_init(&ctx->fdb_orch) != 0) {
        sonic_log(LOG_ERROR, "Failed to initialize FDB orchestrator");
        return -1;
    }
    
    if (route_orch_init(&ctx->route_orch) != 0) {
        sonic_log(LOG_ERROR, "Failed to initialize Route orchestrator");
        return -1;
    }
    
    // Subscribe to database changes
    redis_subscribe(&ctx->redis_ctx, REDIS_DB_APP, "FDB_TABLE*");
    redis_subscribe(&ctx->redis_ctx, REDIS_DB_APP, "ROUTE_TABLE*");
    redis_subscribe(&ctx->redis_ctx, REDIS_DB_APP, "NEIGH_TABLE*");
    
    ctx->running = true;
    
    // Set parent references
    ctx->fdb_orch.parent = orch;
    ctx->route_orch.parent = orch;
    
    sonic_log(LOG_INFO, "OrchAgent initialized successfully");
    return 0;
}

/**
 * @brief Start OrchAgent main event loop
 */
int orchagent_run(orchagent_t *orch) {
    struct orchagent_context *ctx = &g_orchagent;
    
    // Start event processing thread
    if (!THREAD_CREATE(ctx->event_loop_thread, event_loop_thread, ctx)) {
        sonic_log(LOG_ERROR, "Failed to create event loop thread");
        return -1;
    }
    
    sonic_log(LOG_INFO, "OrchAgent started");
    return 0;
}

/**
 * @brief Stop OrchAgent and cleanup resources
 */
int orchagent_stop(orchagent_t *orch) {
    struct orchagent_context *ctx = &g_orchagent;
    
    ctx->running = false;
    
    // Wait for event loop thread to finish
    THREAD_JOIN(ctx->event_loop_thread);
    
    // Cleanup sub-orchestrators
    fdb_orch_cleanup(&ctx->fdb_orch);
    route_orch_cleanup(&ctx->route_orch);
    
    // Cleanup Redis connection
    if (ctx->redis_ctx.connection) {
        redisFree(ctx->redis_ctx.connection);
    }
    
    sonic_log(LOG_INFO, "OrchAgent stopped");
    return 0;
}

/**
 * @brief Subscribe to specific database changes
 */
int orchagent_subscribe_to_db(orchagent_t *orch, redis_database_t db) {
    struct orchagent_context *ctx = &g_orchagent;
    
    switch (db) {
        case REDIS_DB_APP:
            redis_subscribe(&ctx->redis_ctx, db, "APP_DB*");
            break;
        case REDIS_DB_CONFIG:
            redis_subscribe(&ctx->redis_ctx, db, "CONFIG_DB*");
            break;
        case REDIS_DB_STATE:
            redis_subscribe(&ctx->redis_ctx, db, "STATE_DB*");
            break;
        default:
            sonic_log(LOG_WARN, "Unsupported database subscription: %d", db);
            return -1;
    }
    
    return 0;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

// sonic_log is defined in sonic_types.c

/**
 * @brief Get OrchAgent status
 */
bool orchagent_is_running(void) {
    return g_orchagent.running;
}

/**
 * @brief Get performance counters
 */
void orchagent_get_counters(sonic_counters_t *counters) {
    // Collect counters from all sub-orchestrators
    fdb_orch_get_counters(&g_orchagent.fdb_orch, counters);
    route_orch_get_counters(&g_orchagent.route_orch, counters);
    
    // Add OrchAgent specific counters
    counters->packets_received += g_orchagent.packets_processed;
    counters->packets_sent += g_orchagent.packets_forwarded;
}
