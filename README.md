Implementation of bounded-buffer (known also as [producer-consumer primitive](http://en.wikipedia.org/wiki/Producer–consumer_problem)) in C available as a library.
It is useful for inter-thread communication, simple message queue, core of application event loop etc.

It is quite fast; >100k transactions per seconds are easily achievable on common hardware.

Currently POSIX (pthreads) platforms are supported: Linux, Mac OSX, ...


#Performance of libbbuf on Macbook Pro 2009

* Result of test_bbuffer
* Speed: 117647 op/s

Configuration
```
#define NUM_PRODUCER_THREADS 10
#define NUM_CONSUMER_THREADS 10
#define TEST_COUNT 1000000
```

* Processor Name:	Intel Core 2 Duo
* Processor Speed:	2.53 GHz
* Number Of Processors:	1
* Total Number Of Cores:	2
* Memory:	8 GB
* Bus Speed:	1.07 GHz
* Mac OS X 10.6.8 (10K549)
* Darwin 10.8.0
