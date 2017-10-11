# NVMgts #

NVMgts is a kind of fast-recoverable hybrid-memory trajectory storage system. It leverage the advantages of both DRAM and Non-Volatile Memory (NVM).

## Hardware ##
### DRAM ###
+ Low latency
+ Low capacity because of expensive DRAM chip
+ Data loss when restart

### NVM ###
+ 5-10x latency compared with DRAM
+ High capacity
+ Persistence even if restarting
+ Much faster than SSD

## Goals ##
+ Get a higher avaliability than the storage system based on traditional DRAM
+ Get a much better performance (e.g., throughput, latency) than systems based on SSD or HDD
+ Achieve a better performance than system based on pure NVM
+ Support large-scale trajectory dataset through storing data on NVM

## System Mode ##
+ pure DRAM: work as an original in-memory trajectory storage on DRAM
+ pure NVM: all data are stored on NVM, DRAM is used to process the intermediate results
+ hybrid: based on pure NVM, where hot data are transferred to DRAM as a buffer

## Operators ##

### clean ###
`make clean`

### compile ###
`make -jN`

### run ###
`./test [params..]`

### restart ###
`./test r`

### clean all trajectory data on NVM ###
`./test c`

### change system mode ###
+ pure DRAM: `./test m d`
+ pure NVM: `./test m s`
+ hybrid: `./test m h`

### batch queries through file ###
first point out the file in main.cpp
then type `./test f`

### single query through input ###
first type `./test s`
then input `RQ(lon1,lon2,lat1,lat2)`
