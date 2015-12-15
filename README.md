# fast-dat-parser

Needs the *.dat files to be in-order, handles zero-byte gaps, but includes orphans.
For fastest performance, pre-process the *.dat files to exclude orphans and remove zero-byte gaps.

``` bash
# parse the local-best blockchain
cat ~/.bitcoin/blocks/blk*.dat | ./parser -f=0 -n4 > _headers.dat
cat _headers.dat | ./chain > headers.dat
rm _headers.dat

# parse the blockchain, including only blocks matching hashes in headers.dat
cat ~/.bitcoin/blocks/blk*.dat | ./parser -f=1 -n4 -i=headers.dat > scripts.dat
```
