all:
	ff-c++ test.cc
	clang++ -c -g -std=c++11 -I/builds/freefem-source-develop/lib/ff++/4.0/include test.cc
	g++ -bundle -undefined dynamic_lookup -g test.o -o ./myserver.dylib

