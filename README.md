I'm trying to make event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. 

Heavy inspirations:

- [Tame](https://github.com/okws/sfslite/wiki/tame)
- [gevent](http://www.gevent.org/)

I want it to work like Tame but without code transformation (and thus able to be used with libraries written in a blocking style).

Here's an example that works right now:

```c++

void printAfterSeconds(int s, const char m[]) { asleep(s); printf("%s\n", m); }

void amain() {

	printf("Before\n");

	{ await
		A(printAfterSeconds(2, "Slow thing done"));
		printf("Kicked off one thing\n");
		A(printAfterSeconds(1, "Quick thing done"));
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

Calls blocks by default. The `A()` macro runs its contents in a new context and returns control to the caller when it finishes or blocks.

`twait` creates a rendezvous in the current scope. When you leave the scope, the rendezvous' destructor blocks until all of the tasks you spawned finish.

Status: Pretty immature, about a week in.

## Notes to self

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
