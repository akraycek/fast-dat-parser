all: chain parser show

chain: chain.cpp
	g++ -O3 chain.cpp sha1.cpp sha256.cpp -std=c++14 -o chain

parser: parser.cpp
	g++ -pthread -O3 parser.cpp sha1.cpp sha256.cpp -std=c++14 -o parser

show: show.cpp
	g++ -O3 show.cpp -std=c++14 -o show

clean:
	rm -f chain parser show
