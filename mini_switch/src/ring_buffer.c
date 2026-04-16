#include "common.h"
#include "ring_buffer.h"

packet_queues_t global_queues;

void ring_init(void *r) {
    typedef struct {
        void *data[RING_SIZE];
        atomic_uint head;
        atomic_uint tail;
    } ring_t;
    
    ring_t *ring = (ring_t*)r;
    atomic_store(&ring->head, 0);
    atomic_store(&ring->tail, 0);
    memset(ring->data, 0, sizeof(ring->data));
}

int ring_push(void *r, void *item) {
    typedef struct {
        void *data[RING_SIZE];
        atomic_uint head;
        atomic_uint tail;
    } ring_t;
    
    ring_t *ring = (ring_t*)r;
    unsigned head = atomic_load(&ring->head);
    unsigned next = (head + 1) % RING_SIZE;
    
    if (next == atomic_load(&ring->tail)) {
        return -1;
    }
    
    ring->data[head] = item;
    atomic_store(&ring->head, next);
    return 0;
}

void* ring_pop(void *r) {
    typedef struct {
        void *data[RING_SIZE];
        atomic_uint head;
        atomic_uint tail;
    } ring_t;
    
    ring_t *ring = (ring_t*)r;
    unsigned tail = atomic_load(&ring->tail);
    
    if (tail == atomic_load(&ring->head)) {
        return NULL;
    }
    
    void *item = ring->data[tail];
    atomic_store(&ring->tail, (tail + 1) % RING_SIZE);
    return item;
}

int ring_is_empty(void *r) {
    typedef struct {
        void *data[RING_SIZE];
        atomic_uint head;
        atomic_uint tail;
    } ring_t;
    
    ring_t *ring = (ring_t*)r;
    return atomic_load(&ring->head) == atomic_load(&ring->tail);
}

int ring_is_full(void *r) {
    typedef struct {
        void *data[RING_SIZE];
        atomic_uint head;
        atomic_uint tail;
    } ring_t;
    
    ring_t *ring = (ring_t*)r;
    unsigned head = atomic_load(&ring->head);
    unsigned next = (head + 1) % RING_SIZE;
    return next == atomic_load(&ring->tail);
}
