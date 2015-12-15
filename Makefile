all: chain parser show

chain: chain.cpp
	g++ -O3 $< sha1.cpp sha256.cpp -std=c++14 -o $@

parser: parser.cpp
	g++ -pthread -O3 $< sha1.cpp sha256.cpp -std=c++14 -o $@

show: show.cpp
	g++ -O3 $< -std=c++14 -o $@

clean:
	rm -f chain parser show
