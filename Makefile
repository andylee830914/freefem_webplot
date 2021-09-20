all:
	ff-c++ mongoose.c webplot_new.cpp
	# clang++ -c -g -std=c++17 -I/usr/local/lib/ff++/4.6/include webplot.cpp
	# clang++ -bundle -undefined dynamic_lookup -g webplot.o -o ./webplot.dylib

old:
	ff-c++ webplot.cpp
	# clang++ -c -g -std=c++17 -I/usr/local/lib/ff++/4.6/include webplot.cpp
	# clang++ -bundle -undefined dynamic_lookup -g webplot.o -o ./webplot.dylib