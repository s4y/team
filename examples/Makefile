EXAMPLES = $(patsubst %.cpp,bin/%,$(wildcard *.cpp))

CXX=clang++
CXXFLAGS +=\
	 -std=c++11 -stdlib=libc++\
	 -Wall\
	 -Wpedantic\
	 -Werror\
	-D_XOPEN_SOURCE=1\
	-I../include\
	-framework CoreFoundation\
	-framework CoreServices\
	../deps/http-parser/http_parser_g.o\
	../deps/uv/libuv.a

CXXFLAGS += -g

examples: $(EXAMPLES)

bin/%: %.cpp ../include/team
	$(CXX) $(CXXFLAGS) -o $@ $<
