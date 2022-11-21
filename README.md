# hashtable-linux-kernel
linux kernel hashtable example

## HOWTO USE
This will create hashtable with size 33554432 (`1 << 25`) and
fill it with random data with density 0.2 
`make ht && ./build/ht 25 0.2`


### static
`./build/ht` or `build/ht_static`

### shared
`LD_LIBRARY_PATH=build ./build/ht_shared`