BUFFER_SIZE ?= 104857600
N_THREADS ?= 2

index: clean
	g++ -pthread -O3 index.cpp sha1.cpp sha256.cpp -std=c++14 -o index

chain:
	g++ -O3 chain.cpp -std=c++14 -o chain

clean:
	rm -f chain index

test: index
	cat blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) > output.dat

run: index
	cat ~/.bitcoin/blocks/blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) > output.dat

run-longest-chain: index chain
	cat ~/.bitcoin/blocks/blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) -bo=1 | ./chain > output.dat
