/*
 * This file is part of the libbuf library.
 *
 * Copyright (c) 2012, Ales Teska
 * All rights reserved.
 *
 */

#include "bbuf.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

////////////////////////////////////////////////////////////////////////////////

// Local forwards
static void __buf_getabstimeout(struct timespec * ts, unsigned int timeout_ms);

// Error reporting callback
void (*bbuf_perror)(const char *) = perror;

////////////////////////////////////////////////////////////////////////////////

int bbuf_init(struct bbuf_t * b, unsigned int max_size)
{
	int err;

	assert(b != NULL);
	assert(max_size > 0);

	b->buffer = NULL;
	b->max_size = max_size;
	b->head_pos = 0;
	b->tail_pos = 0;

	// Allocate memory for bounded buffer
	b->buffer = malloc(max_size * sizeof(*b->buffer));
	if (b->buffer == NULL) return -1;

	// Initialize mutex
	err = pthread_mutex_init(&b->mutex, NULL);
	if (err)
	{
		bbuf_perror("bbuf_init pthread_mutex_init");
		free(b->buffer);
		return BBUF_FAILED;
	}

	// Initialize 'not full' conditional variable
	err = pthread_cond_init(&b->not_full, NULL);
	if (err)
	{
		bbuf_perror("bbuf_init pthread_cond_init");
		free(b->buffer);
		pthread_mutex_destroy(&b->mutex);
		return BBUF_FAILED;
	}

	// Initialize 'not empty' conditional variable
	err = pthread_cond_init(&b->not_empty, NULL);
	if (err)
	{
		bbuf_perror("bbuf_init pthread_cond_init");
		free(b->buffer);
		pthread_mutex_destroy(&b->mutex);
		pthread_cond_destroy(&b->not_full);
		return BBUF_FAILED;
	}

	return BBUF_OK;
}

///

int bbuf_destroy(struct bbuf_t * b)
{
	int err;


	assert(b != NULL);
	assert(b->buffer != NULL);


	free(b->buffer);
	b->head_pos = 0;
	b->tail_pos = 0;
	b->max_size = 0;


	err = pthread_mutex_destroy(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_destroy pthread_mutex_destroy");
		return BBUF_FAILED;
	}

	err = pthread_cond_destroy(&b->not_full);
	if (err)
	{
		bbuf_perror("bbuf_destroy pthread_cond_destroy");
		return BBUF_FAILED;
	}

	err = pthread_cond_destroy(&b->not_empty);
	if (err)
	{
		bbuf_perror("bbuf_destroy pthread_cond_destroy");
		return BBUF_FAILED;
	}

	return BBUF_OK;
}

///

bool bbuf_full(struct bbuf_t * b)
{
	if (b->tail_pos > 0)
	{
		return (b->tail_pos == (b->head_pos + 1));
	}

	return (b->head_pos == (b->max_size - 1));
}

///

bool bbuf_empty(struct bbuf_t * b)
{
	return (b->tail_pos == b->head_pos);
}

///

unsigned int bbuf_size(struct bbuf_t * b)
{
	if (b->tail_pos > b->head_pos)
	{
		return (b->tail_pos == (b->head_pos + 1));
	} else {
		return (b->head_pos - b->tail_pos);
	}	
}

///

static void __bbuf_put_internal(struct bbuf_t * b, void * item)
{
	// Insert item into a buffer
	b->buffer[b->head_pos] = item;
	if ((b->head_pos + 1) == b->max_size) b->head_pos = 0;
	else b->head_pos += 1;

	// Send 'not empty' event
	pthread_cond_signal(&b->not_empty);
}

///

int bbuf_put(struct bbuf_t * b, void * item)
{
	int err;


	assert(b != NULL);
	assert(b->buffer != NULL);
	assert(b->head_pos < b->max_size);
	assert(b->tail_pos < b->max_size);


	// Lock and conditionally wait for 'not full' event
	err = pthread_mutex_lock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_put pthread_mutex_lock");
		return BBUF_FAILED;
	}


	while (bbuf_full(b))
	{
		err = pthread_cond_wait(&b->not_full, &b->mutex);
		if (err)
		{
			bbuf_perror("bbuf_put pthread_cond_wait");
			pthread_mutex_unlock(&b->mutex);
			return BBUF_FAILED;
		}
	}


	// Insert item into buffer
	__bbuf_put_internal(b, item);


	// Unlock buffer
	err = pthread_mutex_unlock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_put pthread_mutex_unlock");
		return BBUF_FAILED;
	}


	return BBUF_OK;
}

///

int bbuf_timed_put(struct bbuf_t * b, void * item, unsigned int timeout_ms)
{
	struct timespec ts = {0,0};
	int err;


	assert(b != NULL);
	assert(b->buffer != NULL);
	assert(b->head_pos < b->max_size);
	assert(b->tail_pos < b->max_size);


	// Lock and conditionally wait for 'not full' event
	err = pthread_mutex_lock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_timed_put pthread_mutex_lock");
		return BBUF_FAILED;
	}


	while (bbuf_full(b))
	{
		// When iterating for first time, compute absolute time for timeout
		if (ts.tv_sec == 0) __buf_getabstimeout(&ts, timeout_ms);

		err = pthread_cond_timedwait(&b->not_full, &b->mutex, &ts);
		if (err != 0)
		{
			int ret = (err == ETIMEDOUT) ? BBUF_TIMEOUT : BBUF_FAILED;
			
			// Unlock buffer and return due to timeout condition
			err = pthread_mutex_unlock(&b->mutex);
			if (err)
			{
				bbuf_perror("bbuf_timed_put pthread_mutex_unlock");
				return BBUF_FAILED;	
			}
			
			return ret;
		}
	}


	// Insert item into buffer
	__bbuf_put_internal(b, item);


	// Unlock buffer
	err = pthread_mutex_unlock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_timed_put pthread_mutex_unlock");
		return BBUF_FAILED;
	}


	return BBUF_OK;
}

///

static void __bbuf_get_internal(struct bbuf_t * b, void ** item)
{
	// Get item out of a buffer
	*item = b->buffer[b->tail_pos];
	if ((b->tail_pos + 1) == b->max_size) b->tail_pos = 0;
	else b->tail_pos += 1;

	// Send 'not full' event
	pthread_cond_signal(&b->not_full);
}

///

int bbuf_get(struct bbuf_t * b, void ** item)
{
	int err;


	assert(b != NULL);
	assert(b->buffer != NULL);
	assert(b->head_pos < b->max_size);
	assert(b->tail_pos < b->max_size);


	// Lock and conditionally wait for 'not empty' event
	err = pthread_mutex_lock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_get pthread_mutex_lock");
		return BBUF_FAILED;
	}


	while (bbuf_empty(b))
	{
		err = pthread_cond_wait(&b->not_empty, &b->mutex);
		if (err)
		{
			bbuf_perror("bbuf_get pthread_cond_wait");
			err = pthread_mutex_unlock(&b->mutex);
			if (err) bbuf_perror("bbuf_get pthread_mutex_unlock");
			return BBUF_FAILED;
		}

	}


	// Extract item from buffer
	__bbuf_get_internal(b,item);


	// Unlock buffer
	err = pthread_mutex_unlock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_get pthread_mutex_unlock");
		return BBUF_FAILED;
	}

	return BBUF_OK;
}

///

int bbuf_timed_get(struct bbuf_t * b, void ** item, unsigned int timeout_ms)
{
	struct timespec ts = {0, 0};
	int err;


	assert(b != NULL);
	assert(b->buffer != NULL);
	assert(b->head_pos < b->max_size);
	assert(b->tail_pos < b->max_size);

	
	// Lock and conditionally wait for 'not empty' event
	err = pthread_mutex_lock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_timed_get pthread_mutex_lock");
		return BBUF_FAILED;
	}


	while (bbuf_empty(b))
	{
		// When iterating for first time, compute absolute time for timeout
		if (ts.tv_sec == 0) __buf_getabstimeout(&ts, timeout_ms);
	
		err = pthread_cond_timedwait(&b->not_empty, &b->mutex, &ts);
		if (err)
		{
			int ret = (err == ETIMEDOUT) ? BBUF_TIMEOUT : BBUF_FAILED;
			
			// Unlock buffer and return due to timeout condition
			err = pthread_mutex_unlock(&b->mutex);
			if (err)
			{
				bbuf_perror("bbuf_timed_get pthread_mutex_unlock");
				return BBUF_FAILED;	
			}
			
			return ret;
		}

	}


	// Extract item from buffer
	__bbuf_get_internal(b,item);


	// Unlock buffer
	err = pthread_mutex_unlock(&b->mutex);
	if (err)
	{
		bbuf_perror("bbuf_timed_get pthread_mutex_unlock");
		return BBUF_FAILED;
	}


	return 0;
}

////

static void __buf_getabstimeout(struct timespec * ts, unsigned int timeout_ms)
{
	struct timeval now;

	// Prepare absolute time for timeout
	gettimeofday(&now, NULL);
	ts->tv_sec = now.tv_sec;
	ts->tv_nsec = now.tv_usec*1000;

	ts->tv_sec += timeout_ms / 1000;
	ts->tv_nsec += timeout_ms % 1000 * 1000000;

	if (ts->tv_nsec > 999999999)
	{
		ts->tv_sec++;
		ts->tv_nsec = ts->tv_nsec % 1000000000;
	}
}
