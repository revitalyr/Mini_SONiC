/**
 * @file constants.h
 * @brief Centralized constants and string literals for Mini SONiC
 * 
 * This file contains all hardcoded string literals, numeric constants,
 * and configuration values used throughout the project.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * SWITCH NAMES
 * ============================================================================= */

#define SWITCH_NAME_CORE "Core Switch"
#define SWITCH_NAME_AGGREGATION "Aggregation Switch"
#define SWITCH_NAME_EDGE "Edge Switch"
#define SWITCH_NAME_ACCESS "Access Switch"

/* =============================================================================
 * PACKET TYPES
 * ============================================================================= */

#define PACKET_TYPE_L2 "L2"
#define PACKET_TYPE_L3 "L3"
#define PACKET_TYPE_ARP "ARP"

/* =============================================================================
 * JSON KEYS
 * ============================================================================= */

#define JSON_KEY_COUNTERS "counters"
#define JSON_KEY_GAUGES "gauges"
#define JSON_KEY_LATENCY "latency"
#define JSON_KEY_COUNT "count"
#define JSON_KEY_MEAN_NS "mean_ns"
#define JSON_KEY_MIN_NS "min_ns"
#define JSON_KEY_MAX_NS "max_ns"

/* =============================================================================
 * CPU FEATURES
 * ============================================================================= */

#define CPU_FEATURE_SSE2 "sse2"
#define CPU_FEATURE_SSE3 "sse3"
#define CPU_FEATURE_SSE4_1 "sse4_1"
#define CPU_FEATURE_SSE4_2 "sse4_2"
#define CPU_FEATURE_AVX "avx"
#define CPU_FEATURE_AVX2 "avx2"
#define CPU_FEATURE_AVX512 "avx512"

/* =============================================================================
 * NETWORKING CONSTANTS
 * ============================================================================= */

#define DEFAULT_MAC_ADDRESS "00:00:00:00:00:00"
#define BROADCAST_MAC_ADDRESS "FF:FF:FF:FF:FF:FF"
#define DEFAULT_IP_ADDRESS "0.0.0.0"
#define BROADCAST_IP_ADDRESS "255.255.255.255"

/* =============================================================================
 * TIME CONSTANTS (milliseconds)
 * ============================================================================= */

#define MAC_AGING_TIME_MS 300000U  /* 5 minutes */
#define ARP_AGING_TIME_MS 60000U   /* 1 minute */
#define MAINTENANCE_INTERVAL_MS 10000U  /* 10 seconds */

/* =============================================================================
 * BUFFER SIZES
 * ============================================================================= */

#define MAX_SWITCH_NAME_LEN 64
#define MAX_PACKET_SIZE 1518
#define MAX_INTERFACE_NAME_LEN 32

/* =============================================================================
 * LOG LEVELS
 * ============================================================================= */

#define LOG_LEVEL_ERROR "ERROR"
#define LOG_LEVEL_WARN "WARN"
#define LOG_LEVEL_INFO "INFO"
#define LOG_LEVEL_DEBUG "DEBUG"

/* =============================================================================
 * MODULE NAMES
 * ============================================================================= */

#define MODULE_NAME_APP "APP"
#define MODULE_NAME_DATAPLANE "DATAPLANET"
#define MODULE_NAME_NETWORKING "NETWORKING"
#define MODULE_NAME_SAI "SAI"
#define MODULE_NAME_L2L3 "L2L3"
#define MODULE_NAME_UTILS "UTILS"

/* =============================================================================
 * LOGGING MESSAGES
 * ============================================================================= */

#define LOG_MSG_INITIALIZING "Initializing"
#define LOG_MSG_STARTED "Started"
#define LOG_MSG_STOPPED "Stopped"
#define LOG_MSG_ERROR "Error"
#define LOG_MSG_WARNING "Warning"
#define LOG_MSG_INFO "Info"
#define LOG_MSG_DEBUG "Debug"

#ifdef __cplusplus
}
#endif

#endif /* CONSTANTS_H */
