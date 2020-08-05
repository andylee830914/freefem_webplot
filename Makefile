all:
	ff-c++ webplot.cc
	clang++ -c -g -std=c++17 -I/usr/local/lib/ff++/4.6/include webplot.cc
	clang++ -bundle -undefined dynamic_lookup -g webplot.o -o ./webplot.dylib