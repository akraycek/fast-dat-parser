BUFFER_SIZE ?= 104857600
N_THREADS ?= 2

index: clean
	g++ -pthread -O3 *.cpp -std=c++14 -o index

clean:
	rm -f index

test: index
	cat blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) > test.output

run: index
	cat ~/.bitcoin/blocks/blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) > ~/.bitcoin/script-index.dat

dry-run: index
	cat ~/.bitcoin/blocks/blk*.dat | ./index -b=$(BUFFER_SIZE) -n=$(N_THREADS) > /dev/null
