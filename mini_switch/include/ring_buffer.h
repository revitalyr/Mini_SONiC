#pragma once
#include "common.h"

int ring_push(void *r, void *item);
void* ring_pop(void *r);
int ring_is_empty(void *r);
int ring_is_full(void *r);
void ring_init(void *r);
