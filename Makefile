EXAMPLES = $(patsubst examples%.cpp,bin%,$(wildcard examples/*.cpp))

examples: $(EXAMPLES)

bin/%: examples/%.cpp team
	c++ --std=c++11 -stdlib=libc++\
		-I.\
		-Ideps/uv/include\
		-framework CoreFoundation\
		-framework CoreServices\
		deps/uv/libuv.a\
		-g\
		-o $@ $<
