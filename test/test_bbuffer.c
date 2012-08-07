/*
 * This file is part of the libbuf library.
 *
 * Copyright (c) 2012, Ales Teska
 * All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <bbuf.h>

#define NUM_PRODUCER_THREADS 10
#define NUM_CONSUMER_THREADS 10
#define TEST_COUNT 1000000

struct bbuf_t testbuf;

///

#define NUMBERS_SIZE 5000000
int numbers[NUMBERS_SIZE];

long consumer_count;
long producer_count;

long consumer_sum;
long producer_sum;

///

void *producer_thread_entry(void *threadid)
{
	int rc;
	int i;
	int valuepos;
	long checkval = 0;

	for (i=0;i<TEST_COUNT;i++)
	{
		valuepos = random() % NUMBERS_SIZE;
		rc = bbuf_timed_put(&testbuf, (void *)&numbers[valuepos], 1000);
		if (rc == BBUF_TIMEOUT)
		{
			usleep(100);
			continue;
		}

		assert(!rc);

		checkval += numbers[valuepos];
	}

	__sync_add_and_fetch(&producer_count, TEST_COUNT);
	__sync_add_and_fetch(&producer_sum, checkval);
	
	printf("Producer exiting ...\n");
	pthread_exit(NULL);
}

///

void *consumer_thread_entry(void *threadid)
{
	int rc;
	int * value;

	long cnt = 0;
	long checkval = 0;

	for (;;)
	{
		rc = bbuf_timed_get(&testbuf, (void **)&value, 500);
		if (rc == BBUF_TIMEOUT) break;

		assert(!rc);

		checkval += *value;
		cnt += 1;
	}

	__sync_add_and_fetch(&consumer_count, cnt);
	__sync_add_and_fetch(&consumer_sum, checkval);

	printf("Consumer exiting ...\n");
	pthread_exit(NULL);
}

///

int main(int argc, char ** argv)
{
	pthread_t producer_threads[NUM_PRODUCER_THREADS];
	pthread_t consumer_threads[NUM_CONSUMER_THREADS];
	
	int err;
	int rc, i;
	time_t startt, endt;
	
	printf("Test started\n");
	
	srandom(time(NULL));

	consumer_count = 0;
	consumer_sum = 0;
	producer_count = 0;
	producer_sum = 0;

	for (i=0;i<NUMBERS_SIZE;i++) numbers[i] = i;
	
	err = bbuf_init(&testbuf, 64);
	assert(!err);

	printf("Full: %d / Empty: %d\n", bbuf_full(&testbuf), bbuf_empty(&testbuf));

	startt = time(NULL);
	
	for(i=0; i<NUM_PRODUCER_THREADS; i++)
	{
		rc = pthread_create(&producer_threads[i], NULL, producer_thread_entry, NULL);
		assert(!rc);
	}

	for(i=0; i<NUM_CONSUMER_THREADS; i++)
	{
		rc = pthread_create(&consumer_threads[i], NULL, consumer_thread_entry, NULL);
		assert(!rc);
	}

	printf("Joining producers ...\n");
	for(i=0; i<NUM_PRODUCER_THREADS; i++)
	{
		rc = pthread_join(producer_threads[i], NULL);
		assert(!rc);
	}

	printf("Joining consumers ...\n");
	for(i=0; i<NUM_CONSUMER_THREADS; i++)
	{
		rc = pthread_join(consumer_threads[i], NULL);
		assert(!rc);
	}

	err = bbuf_destroy(&testbuf);
	assert(!err);

	endt = time(NULL) - startt;
	
	printf("Producer operations count: %ld\nConsumer operations count: %ld\n", producer_count, consumer_count);
	if (endt > 0) printf("Speed: %ld op/s\n", producer_count / endt);
	printf("Producer operations sum: %ld\nConsumer operations sum: %ld\n", producer_sum, consumer_sum);

	assert(producer_count == (TEST_COUNT * NUM_PRODUCER_THREADS));
	assert(producer_count == consumer_count);
	assert(producer_sum == consumer_sum);
	
	return 0;
}
