EXAMPLES = $(patsubst %.cpp,bin/%,$(wildcard *.cpp))

examples: $(EXAMPLES)

bin/%: %.cpp ../team
	c++ --std=c++11 -stdlib=libc++\
		-D_XOPEN_SOURCE=1\
		-I..\
		-I../deps/uv/include\
		-framework CoreFoundation\
		-framework CoreServices\
		../deps/uv/libuv.a\
		-g\
		-o $@ $<
