/*
 * This file is part of the libbuf library.
 *
 * Copyright (c) 2012, Ales Teska
 * All rights reserved.
 *
 */

#ifndef _LIBBBUF_BBUFFER_H_
#define _LIBBBUF_BBUFFER_H_

#include <stdbool.h>
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

struct bbuf_t
{
	void ** buffer;
	unsigned int max_size;
	unsigned int head_pos;
	unsigned int tail_pos;

	pthread_cond_t not_full;
	pthread_cond_t not_empty;
	pthread_mutex_t mutex;
};

#define BBUF_OK 0
#define BBUF_FAILED -1
#define BBUF_TIMEOUT 1

int bbuf_init(struct bbuf_t * b, unsigned int maxsize);
int bbuf_destroy(struct bbuf_t * b);

unsigned int bbuf_size(struct bbuf_t * b);
bool bbuf_empty(struct bbuf_t * b);
bool bbuf_full(struct bbuf_t * b);

int bbuf_put(struct bbuf_t * b, void * item);
int bbuf_timed_put(struct bbuf_t * b, void * item, unsigned int timeout_ms);

int bbuf_get(struct bbuf_t * b, void ** item);
int bbuf_timed_get(struct bbuf_t * b, void ** item, unsigned int timeout_ms);

extern char * bbuf_version;
extern void (*bbuf_perror)(const char *);

////////////////////////////////////////////////////////////////////////////////

#endif //_LIBBBUF_BBUFFER_H_
