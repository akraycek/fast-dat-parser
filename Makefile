all: index chain

index: index.cpp
	g++ -pthread -O3 index.cpp sha1.cpp sha256.cpp -std=c++14 -o index

chain: chain.cpp
	g++ -O3 chain.cpp -std=c++14 -o chain

clean:
	rm -f chain index
