#pragma once

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// =============================================================================
// SEMANTIC TYPE ALIASES
// =============================================================================
// These aliases provide domain-specific meaning to primitive types,
// making the code more self-documenting and preventing type confusion.
// =============================================================================

// Network-related types
typedef uint16_t PortId;          // Port identifier (semantic alias)
typedef uint16_t VlanId;          // VLAN identifier (semantic alias)
typedef uint8_t  MacAddress[6];   // MAC address (semantic alias)

// Buffer and data types
typedef size_t   BufferLength;    // Buffer length in bytes (semantic alias)
typedef size_t   RingSize;        // Ring buffer size (semantic alias)
typedef uint8_t  Byte;            // Single byte (semantic alias)

// Counting and indexing types
typedef size_t   EntryCount;      // Number of entries (semantic alias)
typedef size_t   PacketCount;     // Number of packets (semantic alias)
typedef uint32_t SequenceNumber;  // Sequence number (semantic alias)

// Time-related types
typedef time_t   Timestamp;       // Unix timestamp (semantic alias)
typedef uint64_t TimestampMs;     // Timestamp in milliseconds (semantic alias)

// Socket and file descriptor types
typedef int      SocketFd;        // Socket file descriptor (semantic alias)
typedef int      InterfaceIndex;  // Network interface index (semantic alias)

// Atomic types
typedef uint32_t AtomicCounter;   // Atomic counter (semantic alias)

// String and identifier types
typedef char     InterfaceName[16]; // Network interface name (semantic alias)
