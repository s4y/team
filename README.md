# Team

Event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. Here's an example:

```c++

#include <team/async.h>

void printAfterSeconds(int s, const char m[]) {
	asleep(s);
	printf("%s\n", m);
}

void amain() {

	printf("Before\n");

	await {
		A{ printAfterSeconds(2, "Slow thing done"); };
		printf("Kicked off one thing\n");
		A{ printAfterSeconds(1, "Quick thing done"); };
		printf("Kicked off another thing\n");
	}

	printf("After\n");

}
```

It prints this:

```
Before
Kicked off one thing
Kicked off another thing
Quick thing done
Slow thing done
After
```

Calls block, but `A{ }` runs its contents asynchronously, returning control to the caller if it blocks, or when it finishes.

`await` blocks until every asynchronous task spawned inside it finishes.

Status: Pretty immature, a couple of weeks in.

I want it to work like Tame but without code transformation (except for the C++ preprocessor), and thus able to be used with libraries written in a blocking style.

## More examples

[Clojure/Go-style channels](http://blog.drewolson.org/blog/2013/07/04/clojure-core-dot-async-and-go-a-code-comparison/) (WIP):

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
```


## Inspiration

- [Tame](https://github.com/okws/sfslite/wiki/tame)
- [gevent](http://www.gevent.org/)

## Notes to self

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
