EXAMPLES = $(patsubst %.cpp,bin/%,$(wildcard *.cpp))

CXX=clang++
CXXFLAGS +=\
	 --std=c++11 -stdlib=libc++\
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

		-o $@ $<

bin/%: %.cpp ../team
	$(CXX) $(CXXFLAGS) -o $@ $<
