/*-
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

#ifndef SCQD_H
#define SCQD_H

#define LFRING_EMPTY	(~(size_t) 0U)

#define LFQ_OK	   (void *) ((void *)1)
#define LFQ_CLOSED (void *) ((void *)-2)
#define LFQ_FULL   (void *) ((void *)-3)
#define LFQ_EMPTY  (void *) LFRING_EMPTY


// Generic queue type - content encapsulated
typedef void queue_t;

/* 
 * Init the queue
 *    order: queue size = 2^order
 */
queue_t *lfq_init(size_t order);

void *lfq_enqueue(queue_t *q, void *val);

void *lfq_dequeue(queue_t *q);

void lfq_close(queue_t *q);

void lfq_free(queue_t *q);

void lfq_dump(queue_t *q);

#endif 
