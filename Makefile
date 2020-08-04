all:
	ff-c++ myserver.cc
	clang++ -c -g -std=c++17 -I/usr/local/lib/ff++/4.6/include myserver.cc
	clang++ -bundle -undefined dynamic_lookup -g myserver.o -o ./myserver.dylib

pdf:
	ff-c++ plotPDF.cc
	clang++ -c -g -std=c++17 -I/usr/local/lib/ff++/4.6/include plotPDF.cc
	g++ -bundle -undefined dynamic_lookup -g plotPDF.o -o ./plotPDF.dylib