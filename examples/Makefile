EXAMPLES = $(patsubst %.cpp,bin/%,$(wildcard *.cpp))

CXX=clang++
CXXFLAGS +=\
	 -std=c++11 -stdlib=libc++\
	 -Wall\
	 -Wpedantic\
	 -Werror\
	-D_XOPEN_SOURCE=1\
	-I..\
	-I../deps/uv/include\
	-framework CoreFoundation\
	-framework CoreServices\
	../deps/uv/libuv.a

CXXFLAGS += -g

examples: $(EXAMPLES)

bin/http: http.cpp ../team
	$(CXX) $(CXXFLAGS)\
		-I../deps/http-parser\
		../deps/http-parser/http_parser_g.o\
		-o $@ $<

bin/%: %.cpp ../team
	$(CXX) $(CXXFLAGS) -o $@ $<
