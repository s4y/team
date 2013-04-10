main: async.h main.cpp
	#c++ --std=c++11 -stdlib=libc++ -g -o main main.cpp
	c++ --std=c++11 -stdlib=libc++\
		-Ideps/uv/include\
		-framework CoreFoundation\
		-framework CoreServices\
		deps/uv/libuv.a\
		main.cpp\
		-g\
		-o main
