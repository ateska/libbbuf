Implementation of bounded-buffer (known also as [producer-consumer primitive](http://en.wikipedia.org/wiki/Producer–consumer_problem)) in C available as a library.
It is useful for inter-thread communication, simple message queue, core of application event loop etc.

It is quite fast; >100k transactions per seconds are easily achievable on common hardware.

Currently POSIX (pthreads) platforms are supported: Linux, Mac OSX, ...
