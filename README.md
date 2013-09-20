# Team

Event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. Here's an example:

```c++

#include <team/async.h>

using async::sleep

void amain() {

	await {
		A [] { sleep(2); printf("Slow thing done\n"); };
		printf("Started a slow thing\n");
		A [] { sleep(1); printf("Quick thing done\n"); };
		printf("Started a quick thing\n");
	}

	printf("Everything's done!\n");

}
```

It prints this:

```
Started a slow thing
Started a quick thing
Quick thing done
Slow thing done
Everything done!
```

Calls block, but `A` runs whatever you pass it (usually a lambda) asynchronously, returning control to the caller if it blocks, or when it finishes.

`await` blocks until every asynchronous task spawned inside it finishes.

Status: Relatively immature.

I want it to work like Tame but without code transformation (except for the C++ preprocessor), and thus able to be used with libraries written in a blocking style.

## Examples

You should check out these examples first:

1. [`hello_world`](https://github.com/Sidnicious/team/blob/master/examples/echo_server.cpp) — How to run stuff asynchronously.
2. [`timers`](https://github.com/Sidnicious/team/blob/master/examples/timers.cpp) — Pretty similar. Includes a fire-and-forget example.
3. [`channels`](https://github.com/Sidnicious/team/blob/master/examples/channels.cpp) — Hacked-together [Clojure/Go-style channels](http://blog.drewolson.org/blog/2013/07/04/clojure-core-dot-async-and-go-a-code-comparison/). Just proof that you can build other async constructs on top of team core. You can skip this.
4. [`generators`](https://github.com/Sidnicious/team/blob/master/examples/generators.cpp) — Python-style generators with `yield`. Also hacked together, you can skip this.
5. [`echo_server`](https://github.com/Sidnicious/team/blob/master/examples/echo_server.cpp) — Start it up and use one or more copies of `nc` to throw packets its way.

## Inspiration

- [Tame](https://github.com/okws/sfslite/wiki/tame)
- [gevent](http://www.gevent.org/)

## Notes to self

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
