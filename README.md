# Team

Event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. Here's an example:

```c++

#include <team/async.h>

using async::sleep

void amain() {

	await {
		A { sleep(2); printf("Slow thing done\n"); };
		printf("Started a slow thing\n");
		A { sleep(1); printf("Quick thing done\n"); };
		printf("Started a quick thing\n");
	}

	printf("Everything done!\n");

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

Calls block, but `A{ }` runs its contents asynchronously, returning control to the caller if it blocks, or when it finishes.

`await` blocks until every asynchronous task spawned inside it finishes.

Status: Pretty immature, a couple of weeks in.

I want it to work like Tame but without code transformation (except for the C++ preprocessor), and thus able to be used with libraries written in a blocking style.

## More examples

[Clojure/Go-style channels](http://blog.drewolson.org/blog/2013/07/04/clojure-core-dot-async-and-go-a-code-comparison/):

```c++
channel<int> ch;

await {
    
    A {
        while (int i = ch.recv()) {
            printf("Pong %d\n", i);
            asleep(1);
        }
    };

    A {
        for (int i = 5; i--;) {
            printf("Ping %d\n", i);
            ch.send(i);
        }
    };
}
```


## Inspiration

- [Tame](https://github.com/okws/sfslite/wiki/tame)
- [gevent](http://www.gevent.org/)

## Notes to self

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
