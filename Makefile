all: chain parser

bestchain: bestchain.cpp
	g++ -O3 $< sha1.cpp sha256.cpp -std=c++14 -o $@

parser: parser.cpp
	g++ -pthread -O3 $< sha1.cpp sha256.cpp -std=c++14 -o $@

clean:
	rm -f bestchain parser
