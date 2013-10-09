# Team

Event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. Here's an example:

```c++
#include <team/team.h>
#include <iostream>

using namespace std;
using team::sleep;

template <typename T>
void log (T s) { cout << s << endl; }

int main() {

    await {
        async { sleep(2); log("Slow thing done"); };
        log("Started a slow thing");
        async { sleep(1); log("Quick thing done"); };
        log("Started a quick thing");
    }

    log("Everything's done!");

}
```

It prints this:

```
Started a slow thing
Started a quick thing
Quick thing done
Slow thing done
Everything's done!
```

Calls block. `async { };` runs its body in a coroutine and returns control to the caller if it blocks, or when it finishes.

`await` blocks until every asynchronous task spawned inside it finishes.

Status: Relatively new. Gradually becoming useful though.

I want it to work like Tame but without code transformation (except for the C++ preprocessor), and thus able to be used with libraries written in a blocking style.

## Examples

You should check out these examples first:

1. [`hello_world`](https://github.com/Sidnicious/team/blob/master/examples/hello_world.cpp) — How to run stuff asynchronously.
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
