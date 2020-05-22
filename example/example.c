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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

#include "scqd.h"

const int max = 10000000;

static int total;
static pthread_mutex_t lock; 

static void *consumer(void *_queue);

static void *consumer(void *_queue){
queue_t *queue = (queue_t *)_queue;

	uintptr_t cnt = 0;
	int empty_cnt = 0;
	do {
		void *data = lfq_dequeue(queue);
		if ( data == LFQ_CLOSED ) {
			printf("Q closed\n");
			break;
		}
		if ( data == LFQ_EMPTY ) {
			empty_cnt++;
		} else {
			cnt++;
		}
	} while (1);

	pthread_mutex_lock(&lock);
	total += cnt;
	pthread_mutex_unlock(&lock);

	printf("thread done: %lu, empty_cnt: %d\n", cnt, empty_cnt);
	pthread_exit(NULL);
}

int main(int argc, char **argv) {

	queue_t *queue;

	total = 0;
	pthread_mutex_init(&lock, NULL);
	queue = lfq_init(15);

	pthread_t _thread[2];
	pthread_create(&_thread[0],NULL,consumer,queue);
	pthread_create(&_thread[1],NULL,consumer,queue);

	uintptr_t cnt = 1;
	uintptr_t failed_cnt = 0;
	do {
		if ( lfq_enqueue(queue, (void *)cnt) != LFQ_OK ) {
			failed_cnt++;
			printf("enqueue empty_cnt\n");
		} else {
			cnt++;
		}
	} while ( cnt <= max );
	cnt--;

	lfq_close(queue);
	printf("Feed done: %lu, failed enqueues: %lu\n", cnt, failed_cnt);

	pthread_join(_thread[0],NULL);
	pthread_join(_thread[1],NULL);

	printf("Done. Total consumed count: %d\n", total);
	lfq_free(queue);

	return 0;
}
