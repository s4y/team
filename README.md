I'm trying to make event loop ([libuv](https://github.com/joyent/libuv))-driven coroutines for C++ that are easy to use with minimal syntax. 

Heavy inspirations:

- [Tame](https://github.com/okws/sfslite/wiki/tame)
- [gevent](http://www.gevent.org/)

I want it to work like Tame but without code transformation (except for the C++ preprocessor), and thus able to be used with libraries written in a blocking style.

Here's an example:

```c++

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

Calls block by default. `A{ }` runs its contents in a new context and returns control to the caller when it finishes or blocks.

`await` blocks until every task you spawned inside it finishes.

Status: Pretty immature, a couple of weeks in.

## Notes to self

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
