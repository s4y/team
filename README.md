I'm trying to make a version of Tame that doesn't require code transformation (and thus can be used with existing libraries written in a blocking style). The plan is to manipulate the stack (basically making fibers with multiple "roots") to support parallelism:

    { AWAIT;
		do_stuff();
		do_more_stuff();
	}

Status: just started doing research. Pretty much no code written.

## Notes

- http://www.rethinkdb.com/blog/improving-a-large-c-project-with-coroutines/
- http://www.rethinkdb.com/blog/making-coroutines-fast/
