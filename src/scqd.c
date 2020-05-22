/*-
 * Copyright (c) 2019 Ruslan Nikolaev.  All Rights Reserved.
 * Copyright (c) 2020 Peter Haag
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* vi: set tabstop=4: */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <string.h>

#include "lf.h"
#include "scqd.h"

#if LFATOMIC_WIDTH == 32
# define LFRING_MIN	(LF_CACHE_SHIFT - 2)
#elif LFATOMIC_WIDTH == 64
# define LFRING_MIN	(LF_CACHE_SHIFT - 3)
#elif LFATOMIC_WIDTH == 128
# define LFRING_MIN	(LF_CACHE_SHIFT - 4)
#else
# error "Unsupported LFATOMIC_WIDTH."
#endif

struct __lfring {
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) head;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfsatomic_t) threshold;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) tail;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) array[1];
};

typedef struct _queue_s {
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) closed;
	size_t order;
	struct __lfring *aq;
	struct __lfring *fq;
	void **val;
} _queue_t;

#define LFRING_ALIGN	(_Alignof(struct __lfring))
#define LFRING_SIZE(o)	\
	(offsetof(struct __lfring, array) + (sizeof(lfatomic_t) << ((o) + 1)))


#define __lfring_cmp(x, op, y)	((lfsatomic_t) ((x) - (y)) op 0)

#if LFRING_MIN != 0
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n) {
	return (size_t) (((idx & (n - 1)) >> (order + 1 - LFRING_MIN)) |
			((idx << LFRING_MIN) & (n - 1)));
}
#else
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n) {
	return (size_t) (idx & (n - 1));
}
#endif

#define __lfring_threshold3(half, n) ((long) ((half) + (n) - 1))

static inline size_t lfring_pow2(size_t order) {
	return (size_t) 1U << order;
}

static inline void lfring_init_empty(struct __lfring *q, size_t order) {
	size_t i, n = lfring_pow2(order + 1);

	for (i = 0; i != n; i++)
		atomic_init(&q->array[i], (lfsatomic_t) -1);

	atomic_init(&q->head, 0);
	atomic_init(&q->threshold, -1);
	atomic_init(&q->tail, 0);
}

static inline void lfring_init_fill(struct __lfring *q, size_t s, size_t e, size_t order) {
	size_t i, half = lfring_pow2(order), n = half * 2;

	for (i = 0; i != s; i++)
		atomic_init(&q->array[__lfring_map(i, order, n)], 2 * n - 1);
	for (; i != e; i++)
		atomic_init(&q->array[__lfring_map(i, order, n)], n + i);
	for (; i != n; i++)
		atomic_init(&q->array[__lfring_map(i, order, n)], (lfsatomic_t) -1);

	atomic_init(&q->head, s);
	atomic_init(&q->threshold, __lfring_threshold3(half, n));
	atomic_init(&q->tail, e);
}

static inline bool lfring_enqueue(struct __lfring *q, size_t order, size_t eidx, bool nonempty) {
	size_t tidx, half = lfring_pow2(order), n = half * 2;
	lfatomic_t tail, entry, ecycle, tcycle;

	eidx ^= (n - 1);

	while (1) {
		tail = atomic_fetch_add_explicit(&q->tail, 1, memory_order_acq_rel);
		tcycle = (tail << 1) | (2 * n - 1);
		tidx = __lfring_map(tail, order, n);
		entry = atomic_load_explicit(&q->array[tidx], memory_order_acquire);
retry:
		ecycle = entry | (2 * n - 1);
		if (__lfring_cmp(ecycle, <, tcycle) && ((entry == ecycle) ||
				((entry == (ecycle ^ n)) &&
				 __lfring_cmp(atomic_load_explicit(&q->head,
				  memory_order_acquire), <=, tail)))) {

			if (!atomic_compare_exchange_weak_explicit(&q->array[tidx],
					&entry, tcycle ^ eidx,
					memory_order_acq_rel, memory_order_acquire))
				goto retry;

			if (!nonempty && (atomic_load(&q->threshold) != __lfring_threshold3(half, n)))
				atomic_store(&q->threshold, __lfring_threshold3(half, n));
			return true;
		}
	}
}

static inline void __lfring_catchup(struct __lfring *q, lfatomic_t tail, lfatomic_t head) {
	while (!atomic_compare_exchange_weak_explicit(&q->tail, &tail, head,
			memory_order_acq_rel, memory_order_acquire)) {
		head = atomic_load(&q->head);
		tail = atomic_load(&q->tail);
		if (__lfring_cmp(tail, >=, head))
			break;
	}
}

static inline size_t lfring_dequeue(struct __lfring *q, size_t order, bool nonempty) {
	size_t hidx, n = lfring_pow2(order + 1);
	lfatomic_t head, entry, entry_new, ecycle, hcycle, tail;
	size_t attempt;

	if (!nonempty && atomic_load(&q->threshold) < 0) {
		return LFRING_EMPTY;
	}

	while (1) {
		head = atomic_fetch_add_explicit(&q->head, 1, memory_order_acq_rel);
		hcycle = (head << 1) | (2 * n - 1);
		hidx = __lfring_map(head, order, n);
		attempt = 0;
again:
		entry = atomic_load_explicit(&q->array[hidx], memory_order_acquire);

		do {
			ecycle = entry | (2 * n - 1);
			if (ecycle == hcycle) {
				atomic_fetch_or_explicit(&q->array[hidx], (n - 1),
						memory_order_acq_rel);
				return (size_t) (entry & (n - 1));
			}

			if ((entry | n) != ecycle) {
				entry_new = entry & ~(lfatomic_t) n;
				if (entry == entry_new)
					break;
			} else {
				if (++attempt <= 10000)
					goto again;
				entry_new = hcycle ^ ((~entry) & n);
			}
		} while (__lfring_cmp(ecycle, <, hcycle) &&
					!atomic_compare_exchange_weak_explicit(&q->array[hidx],
					&entry, entry_new,
					memory_order_acq_rel, memory_order_acquire));

		if (!nonempty) {
			tail = atomic_load_explicit(&q->tail, memory_order_acquire);
			if (__lfring_cmp(tail, <=, head + 1)) {
				__lfring_catchup(q, tail, head + 1);
				atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel);
				return LFRING_EMPTY;
			}

			if (atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel) <= 0)
				return LFRING_EMPTY;
		}
	}
}

queue_t *lfq_init(size_t order) {

	_queue_t *queue = calloc(1, sizeof(_queue_t));
	if ( !queue )
		return NULL;

	// int queue struct
	queue->order = order;
	atomic_init(&queue->closed, 0);

	// allocate one chunk of memory depending on order
	size_t totalSize = 2 * (LFRING_SIZE(order)) + (1U << order) * sizeof(void *);
	void *queueMem = calloc(1, totalSize);
	if ( !queueMem ) {
		free(queue);
		return NULL;
	}

	// assign memory to queue
	queue->aq  = queueMem;
	queue->fq  = (struct __lfring *)(queueMem + LFRING_SIZE(order));
	queue->val = (void **)(queueMem + 2*(LFRING_SIZE(order)));

	// init rings
	lfring_init_empty(queue->aq, order);
	lfring_init_fill(queue->fq, 0, 1U << order, order);

	return (queue_t *)queue;

} // End of lfq_init

void *lfq_enqueue(queue_t *q, void * val) {
	_queue_t *queue = (_queue_t *)q;

	if ( atomic_load(&queue->closed) )
		return LFQ_CLOSED;

	size_t eidx;
	eidx = lfring_dequeue(queue->fq, queue->order, true);
	if (eidx == LFRING_EMPTY)
		return LFQ_FULL;

	queue->val[eidx] = val;

	lfring_enqueue(queue->aq, queue->order, eidx, false);
	return LFQ_OK;

} // End of lfq_enqueue

void *lfq_dequeue(queue_t * q) {
	_queue_t *queue = (_queue_t *)q;

	size_t eidx = lfring_dequeue(queue->aq, queue->order, false);
	if (eidx == LFRING_EMPTY) {
		if ( atomic_load(&queue->closed) )
			return LFQ_CLOSED;
		return LFQ_EMPTY;
	}

	void *val = queue->val[eidx];
	lfring_enqueue(queue->fq, queue->order, eidx, true);

	return val;
} // End of lfq_dequeue

void lfq_close(queue_t * q) {
	_queue_t *queue = (_queue_t *)q;
	
	atomic_fetch_add_explicit(&queue->closed, 1, memory_order_acq_rel);
	return;
} // End of lfq_close

void lfq_free(queue_t *q) {
	_queue_t *queue = (_queue_t *)q;

	free(queue->aq);
	free(queue);
} // End of lfq_free

void lfq_dump(queue_t *q) {
	_queue_t *queue = (_queue_t *)q;

	for (int i = 0; i < (1U << queue->order); i++)
		printf("slot: %u - %lu\n", i, (uintptr_t)queue->val[i]);

} // End of lfq_dump


