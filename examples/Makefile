EXAMPLES = $(patsubst %.cpp,bin/%,$(wildcard *.cpp))

CXX=clang++
CXXFLAGS +=\
	 -std=c++11 -stdlib=libc++\
	 -Wall\
	 -Wpedantic\
	 -Werror\
	-D_XOPEN_SOURCE=1\
	-I../include\
	-Iinclude\
	-framework CoreFoundation\
	-framework CoreServices\
	deps/http-parser/http_parser_g.o\
	deps/uv/libuv.a

CXXFLAGS += -g

examples: $(EXAMPLES)

bin/%: %.cpp ../include/team util.h deps/uv/libuv.a deps/http-parser/http_parser_g.o
	$(CXX) $(CXXFLAGS) -o $@ $<

deps/uv/libuv.a: deps/uv/Makefile
	$(MAKE) -C $(@D)

deps/http-parser/http_parser_g.o: deps/http_parser/Makefile
	$(MAKE) -C $(@D)

deps/uv/Makefile deps/http_parser/Makefile:
	git submodule init && git submodule update
