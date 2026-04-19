#pragma once

#ifdef _WIN32
    // Windows-specific headers
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <process.h>
    #pragma comment(lib, "ws2_32.lib")

    // Windows compatibility definitions
    #define sleep(x) Sleep((x) * 1000)
    #define usleep(x) Sleep((x) / 1000)
    typedef HANDLE pthread_t;
    typedef DWORD pthread_key_t;
    #define pthread_create(handle, attr, routine, arg) \
        ((*(handle) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(routine), (arg), 0, NULL)) == NULL ? -1 : 0)
    #define pthread_join(thread, result) WaitForSingleObject(thread, INFINITE)
    #define pthread_detach(thread) CloseHandle(thread)

#else
    // Unix/Linux headers
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <linux/if_packet.h>
    #include <linux/if_ether.h>
    #include <linux/if_arp.h>
    #include <arpa/inet.h>
    #include <pthread.h>
#endif

// Common headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#ifdef _WIN32
    #include <windows.h>
    #define atomic_uint volatile ULONG
    #define atomic_fetch_add(ptr, val) InterlockedAdd((volatile LONG*)(ptr), (val))
    #define atomic_fetch_sub(ptr, val) InterlockedAdd((volatile LONG*)(ptr), -(val))
    #define atomic_load(ptr) (*(ptr))
    #define atomic_store(ptr, val) (*(ptr) = (val))
#else
    #include <stdatomic.h>
#endif
#include <time.h>
#include <signal.h>

// Include semantic type aliases
#include "semantic_types.h"

// Include constants
#include "constants.h"

// Use only kernel headers to avoid conflicts (Linux only)
#ifndef _WIN32
#include <linux/if.h>
#endif

typedef struct {
    Byte data[BUFFER_SIZE];
    BufferLength len;
    PortId port;
    struct timespec timestamp;
} packet_t;

typedef struct {
    void *data[RING_SIZE];
    AtomicCounter head;
    AtomicCounter tail;
} ring_t;

typedef struct {
    ring_t rx_queue;
    ring_t tx_queue;
} packet_queues_t;

// Forward declarations
extern packet_queues_t global_queues;
extern volatile PacketCount global_rx_count;
extern volatile PacketCount global_tx_count;
