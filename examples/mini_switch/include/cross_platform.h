/**
 * @file cross_platform.h
 * @brief Cross-platform compatibility layer for SONiC Mini Switch
 * 
 * Provides unified APIs for:
 * - Threading (Windows threads / pthread)
 * - Synchronization (mutexes, conditions)
 * - Time functions
 * - Network types
 */

#ifndef CROSS_PLATFORM_H
#define CROSS_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// =============================================================================
// ENDIANNESS DETECTION
// =============================================================================

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ || \
    defined(__BIG_ENDIAN__) || \
    defined(_BIG_ENDIAN) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
    #define PLATFORM_IS_BIG_ENDIAN 1
#else
    #define PLATFORM_IS_BIG_ENDIAN 0
#endif

// Windows doesn't have ssize_t, define it
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

// =============================================================================
// PLATFORM DETECTION
// =============================================================================

#ifdef _WIN32
    #define PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX // Prevent min/max macros from windows.h
    #include <windows.h>
    #include <winsock2.h>
    #include <time.h>
    // Linux networking constants for compatibility
    #define AF_PACKET 17
    #define ETH_P_ALL 3
    #define ETH_P_8021Q 0x8100
    #include <ws2tcpip.h> // For sockaddr_in, etc.

    // Linux-specific networking structures and constants (stubs for compilation)
    #define IFNAMSIZ 16 // Common size on Linux

    // Dummy struct for ifreq
    struct ifreq {
        char ifr_name[IFNAMSIZ];
        union {
            struct sockaddr ifr_addr;
            struct sockaddr ifr_dstaddr;
            struct sockaddr ifr_broadaddr;
            struct sockaddr ifr_netmask;
            struct sockaddr ifr_hwaddr;
            short ifr_flags;
            int ifr_ifindex;
            int ifr_metric;
            int ifr_mtu;
            char ifr_slave[IFNAMSIZ];
            char ifr_newname[IFNAMSIZ];
            char *ifr_data;
        };
    };

    // Dummy struct for sockaddr_ll (Linux-specific link-layer socket address)
    struct sockaddr_ll {
        unsigned short sll_family;
        unsigned short sll_protocol;
        int sll_ifindex;
        unsigned short sll_hatype;
        unsigned char sll_pkttype;
        unsigned char sll_halen;
        unsigned char sll_addr[8];
    };

    // Dummy values for ioctl commands and flags
    #define SIOCGIFINDEX 0x8933 // Get interface index
    #define SIOCGIFFLAGS 0x8913 // Get interface flags
    #define SIOCSIFFLAGS 0x8914 // Set interface flags
    #define SIOCGIFHWADDR 0x8927 // Get hardware address
    #define IFF_PROMISC  0x100  // Promiscuous mode
    #define PACKET_HOST  0      // Packet type: to us
    #define ETH_P_ARP 0x0806    // ARP protocol

    #include <process.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #define PLATFORM_UNIX
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

// =============================================================================
// THREADS
// =============================================================================

#ifdef PLATFORM_WINDOWS
    typedef HANDLE thread_t;
    typedef unsigned int thread_ret_t;
    #define THREAD_CALL __stdcall
    #define THREAD_RETVAL(v) ((unsigned int)(uintptr_t)(v))
    #define THREAD_CREATE(thread, func, arg) \
        ((thread) = (HANDLE)_beginthreadex(NULL, 0, (unsigned int (__stdcall *)(void *))(func), (void*)(arg), 0, NULL), (thread) != NULL)
    #define THREAD_JOIN(thread) WaitForSingleObject((thread), INFINITE); CloseHandle(thread)
    #define THREAD_EXIT(ret) do { _endthreadex(THREAD_RETVAL(ret)); return THREAD_RETVAL(ret); } while(0)
    #define THREAD_RETURN_TYPE thread_ret_t
#else
    typedef pthread_t thread_t;
    typedef void* thread_ret_t;
    #define THREAD_CALL
    #define THREAD_RETVAL(v) (v)
    #define THREAD_CREATE(thread, func, arg) (pthread_create(&(thread), NULL, (func), (arg)) == 0)
    #define THREAD_JOIN(thread) pthread_join(thread, NULL)
    #define THREAD_EXIT(ret) do { pthread_exit(ret); return THREAD_RETVAL(ret); } while(0)
    #define THREAD_RETURN_TYPE thread_ret_t
#endif

// =============================================================================
// MUTEXES
// =============================================================================

#ifdef PLATFORM_WINDOWS
typedef CRITICAL_SECTION mutex_t;
    #define MUTEX_INIT(m) InitializeCriticalSection(&(m))
    #define MUTEX_DESTROY(m) DeleteCriticalSection(&(m))
    #define MUTEX_LOCK(m) EnterCriticalSection(&(m))
    #define MUTEX_TRYLOCK(m) TryEnterCriticalSection(&(m))
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(&(m))
    
    // Static initializer not available on Windows, must call MUTEX_INIT
    #define MUTEX_STATIC_INIT {0}
#else
typedef pthread_mutex_t mutex_t;
    #define MUTEX_INIT(m) pthread_mutex_init(&(m), NULL)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(&(m))
    #define MUTEX_LOCK(m) pthread_mutex_lock(&(m))
    #define MUTEX_TRYLOCK(m) (pthread_mutex_trylock(&(m)) == 0)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(&(m))
    #define MUTEX_STATIC_INIT PTHREAD_MUTEX_INITIALIZER
#endif

// =============================================================================
// CONDITION VARIABLES
// =============================================================================

#ifdef PLATFORM_WINDOWS
typedef CONDITION_VARIABLE cond_t;
    #define COND_INIT(c) InitializeConditionVariable(&(c))
    #define COND_DESTROY(c) // No-op on Windows
    #define COND_WAIT(c, m) SleepConditionVariableCS(&(c), &(m), INFINITE)
    #define COND_TIMEDWAIT(c, m, ms) SleepConditionVariableCS(&(c), &(m), (ms))
    #define COND_SIGNAL(c) WakeConditionVariable(&(c))
    #define COND_BROADCAST(c) WakeAllConditionVariable(&(c))
#else
typedef pthread_cond_t cond_t;
    #define COND_INIT(c) pthread_cond_init(&(c), NULL)
    #define COND_DESTROY(c) pthread_cond_destroy(&(c))
    #define COND_WAIT(c, m) pthread_cond_wait(&(c), &(m))
    #define COND_TIMEDWAIT(c, m, timeout) pthread_cond_timedwait(&(c), &(m), (timeout))
    #define COND_SIGNAL(c) pthread_cond_signal(&(c))
    #define COND_BROADCAST(c) pthread_cond_broadcast(&(c))
#endif

// =============================================================================
// SLEEP FUNCTIONS
// =============================================================================

#ifdef PLATFORM_WINDOWS
    #define SLEEP_MS(ms) Sleep(ms)
    #define SLEEP_S(s) Sleep((s) * 1000)
#else
    #define SLEEP_MS(ms) usleep((ms) * 1000)
    #define SLEEP_S(s) sleep(s)
#endif

// =============================================================================
// TIME FUNCTIONS
// =============================================================================

#ifdef PLATFORM_WINDOWS
typedef struct timeval timeval_t;

static inline int get_timeofday(timeval_t* tv) {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // Convert from 100-nanosecond intervals since Jan 1, 1601 to Unix epoch
    uli.QuadPart -= 116444736000000000ULL;
    tv->tv_sec = (long)(uli.QuadPart / 10000000);
    tv->tv_usec = (long)((uli.QuadPart % 10000000) / 10);
    return 0;
}

static inline uint64_t get_time_ms(void) {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart - 116444736000000000ULL) / 10000;
}

static inline uint64_t get_time_us(void) {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart - 116444736000000000ULL) / 10;
}

// clock_gettime stub for Windows compatibility
#define CLOCK_MONOTONIC 0

// =============================================================================
// FREERTOS SIMULATION STUBS (for Windows build)
// =============================================================================
#ifdef PLATFORM_WINDOWS
    #ifndef portTICK_PERIOD_MS
        #define portTICK_PERIOD_MS (1U)
    #endif
    #ifndef pdMS_TO_TICKS
        #define pdMS_TO_TICKS(ms) ((uint32_t)(ms))
    #endif

    #define pdTRUE  (1)
    #define pdFALSE (0)
    #define pdPASS  (1)
    #define pdFAIL  (0)

    #ifndef _FREERTOS_SIM_TYPES_DEFINED
        #define _FREERTOS_SIM_TYPES_DEFINED
        typedef void* TimerHandle_t;
        typedef void* QueueHandle_t;
        typedef void* SemaphoreHandle_t;
        typedef int   BaseType_t;
        typedef int   UBaseType_t;
        typedef uint32_t TickType_t;

        // Static allocation stubs (aligned to match freertos_config.h)
        typedef struct { uint8_t dummy[32]; } StaticQueue_t;
        typedef struct { uint8_t dummy[32]; } StaticSemaphore_t;
        typedef struct { uint8_t dummy[32]; } StaticTimer_t;

        static inline TickType_t xTaskGetTickCount(void) { return (TickType_t)get_time_ms(); }
        static inline void* pvTimerGetTimerID(TimerHandle_t xTimer) { return xTimer; }
    #endif
#endif

// timespec is defined in time.h since VS 2015 (MSC_VER 1900).
// We use multiple guards to prevent redefinition across different SDK versions.
#if defined(_WIN32) && !defined(_CRT_TIMESPEC_DEFINED) && !defined(_TIMESPEC_DEFINED)
    #if !defined(_MSC_VER) || _MSC_VER < 1900
        #define _TIMESPEC_DEFINED
        #define _CRT_TIMESPEC_DEFINED
        struct timespec {
            time_t tv_sec;
            long   tv_nsec;
        };
    #endif
#endif

static inline int clock_gettime(int clock_id, struct timespec *tp) {
    (void)clock_id;
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    uint64_t ns = (uli.QuadPart - 116444736000000000ULL) * 100;
    tp->tv_sec = (long)(ns / 1000000000ULL);
    tp->tv_nsec = (long)(ns % 1000000000ULL);
    return 0;
}
#else
typedef struct timeval timeval_t;
    #define get_timeofday(tv) gettimeofday(tv, NULL)
    
static inline uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

// =============================================================================
// ATOMIC OPERATIONS (simplified - for MSVC and GCC/Clang)
// =============================================================================

#ifdef _MSC_VER
    #include <intrin.h>
    #define ATOMIC_INC(ptr) _InterlockedIncrement((long*)ptr)
    #define ATOMIC_DEC(ptr) _InterlockedDecrement((long*)ptr)
    #define ATOMIC_ADD(ptr, val) _InterlockedExchangeAdd((long*)ptr, (val))
    #define MEMORY_BARRIER() _ReadWriteBarrier()
#else
    #define ATOMIC_INC(ptr) __sync_add_and_fetch(ptr, 1)
    #define ATOMIC_DEC(ptr) __sync_sub_and_fetch(ptr, 1)
    #define ATOMIC_ADD(ptr, val) __sync_add_and_fetch(ptr, val)
    #define MEMORY_BARRIER() __sync_synchronize()
#endif

// =============================================================================
// SOCKET TYPES (for network code)
// =============================================================================

#ifdef PLATFORM_WINDOWS
typedef SOCKET socket_t;
    #define SOCKET_INVALID INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
    #define SOCKET_CLOSE(s) closesocket(s)
    #define SOCKET_INIT() { WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData); }
    #define SOCKET_CLEANUP() WSACleanup()
    #define SOCKET_ERRNO WSAGetLastError()
    #define SOCKET_EAGAIN WSAEWOULDBLOCK
    #define SOCKET_EINPROGRESS WSAEWOULDBLOCK
    #define SOCKET_SET_NONBLOCKING(s) { u_long mode = 1; ioctlsocket(s, FIONBIO, &mode); }
#else
typedef int socket_t;
    #define SOCKET_INVALID -1
    #define SOCKET_ERROR_VALUE -1
    #define SOCKET_CLOSE(s) close(s)
    #define SOCKET_INIT()
    #define SOCKET_CLEANUP()
    #define SOCKET_ERRNO errno
    #define SOCKET_EAGAIN EAGAIN
    #define SOCKET_EINPROGRESS EINPROGRESS
    #include <fcntl.h>
    #define SOCKET_SET_NONBLOCKING(s) { int flags = fcntl(s, F_GETFL, 0); fcntl(s, F_SETFL, flags | O_NONBLOCK); }
#endif

// =============================================================================
// EXPORT/IMPORT MACROS (for shared libraries)
// =============================================================================

#ifdef _WIN32
    #ifdef BUILDING_DLL
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define DLL_EXPORT __attribute__((visibility("default")))
#endif

// =============================================================================
// PACKED STRUCT MACROS (for cross-compiler compatibility)
// =============================================================================

#ifdef _MSC_VER
    #define PACKED_STRUCT
    #define START_PACKED_STRUCT __pragma(pack(push, 1))
    #define END_PACKED_STRUCT __pragma(pack(pop))
#else
    #define PACKED_STRUCT __attribute__((packed))
    #define START_PACKED_STRUCT
    #define END_PACKED_STRUCT
#endif

// =============================================================================
// TERMINAL UI (ncurses/PDCurses)
// =============================================================================
#ifdef HAS_CURSES
    // On Windows, PDCurses uses curses.h. On Unix, ncurses uses ncurses.h.
    #ifdef PLATFORM_WINDOWS
        #include <curses.h>
    #else
        #include <ncurses.h>
    #endif
#endif

// =============================================================================
// ATOMIC OPERATIONS (Windows replacement for GCC builtins)
// =============================================================================
#ifdef _WIN32
    #include <intrin.h>
    static inline int __sync_fetch_and_add(int *ptr, int value) {
        return _InterlockedExchangeAdd((volatile long*)ptr, value);
    }
    static inline int __sync_fetch_and_sub(int *ptr, int value) {
        return _InterlockedExchangeAdd((volatile long*)ptr, -value);
    }
#endif

// =============================================================================
// IOCTL (Windows replacement for Unix ioctl)
// =============================================================================
#ifdef _WIN32
    #define ioctl(fd, request, ...) _ioctl(fd, request, ##__VA_ARGS__)
    static inline int _ioctl(int fd, unsigned long request, ...) {
        // Stub for Windows - ioctl is Unix-specific
        (void)fd; (void)request;
        return -1;
    }
#endif

// =============================================================================
// VISUAL FUNCTION STUBS (when HAS_CURSES is not defined)
// =============================================================================
#ifndef HAS_CURSES
    static inline void visual_packet(const uint8_t *src, const uint8_t *dst, int src_port, int dst_port, int known_mac, int is_arp, int vlan_id) {
        (void)src; (void)dst; (void)src_port; (void)dst_port; (void)known_mac; (void)is_arp; (void)vlan_id;
    }
    static inline void visual_mac_table(uint8_t macs[][6], int ports[], int count) {
        (void)macs; (void)ports; (void)count;
    }
    static inline void visual_arp_table(uint32_t ips[], uint8_t macs[][6], int count) {
        (void)ips; (void)macs; (void)count;
    }
    static inline void arp_table_print_visual(void) {
    }
#endif

#endif // CROSS_PLATFORM_H
