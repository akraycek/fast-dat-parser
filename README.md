# fast-dat-parser

Needs the *.dat files to be in-order, handles zero-byte gaps, but includes orphans.
For fastest performance, pre-process the *.dat files to exclude orphans and remove zero-byte gaps, I guess.

Using 4 modern cores, this parses the blockchain (w/ height @ ~378000) almost fast as your IO can pipe it out.

All memory as allocated up front.
All output goes to `stdout`, `stderr` is used for logging.


#### parser

- `-f` - parse function (default `0`, see pre-packaged parse functions below)
- `-m` - memory usage (default `104857600` bytes, ~100 MiB)
- `-n` - N threads for parallel computation (default `1`)
- `-w` - whitelist file, for omitting blocks from parsing


##### parse functions

- `0` - Outputs solely the *unordered* 80-byte block headers, may include orphans (binary, not hex)
- `1` - Outputs every script hash, for every transaction, in every block, ~`BLOCK_HASH | TX_HASH | SCRIPT_HASH` (binary, not hex)

Use `-w` to avoid orphan data being included. (see example for best chain filtering)


## Example

``` bash
# parse the local-best blockchain
cat ~/.bitcoin/blocks/blk*.dat | ./parser -f0 -n4 > _headers.dat
cat _headers.dat | ./bestchain > headers.dat
rm _headers.dat

# parse the blockchain, including only blocks matching hashes in headers.dat
cat ~/.bitcoin/blocks/blk*.dat | ./parser -f1 -n4 -wheaders.dat > scripts.dat
```
