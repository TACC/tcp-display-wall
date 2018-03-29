# TCP Bridged Display Wall for OSPray Readme (v0.2)

This display wall module is based of the MPI Distributed module provided by the base OSPRay source code.

## Prerequisites

#### OSPRay

Follow the instructions to install from source [OSPRay](https://github.com/ospray/OSPRay)

Currently it dependes on a modifications of the MPI Offload module for OSPray that can be
found here

[https://github.com/jgbarbosa/ospray/tree/dw](https://github.com/jgbarbosa/ospray/tree/dw)

#### Google SNAPPY Compression

The display wall TCP connection depends on Google SNAPPY compression algorithm to compress/uncompress the data stream between
the display and the farm.

Google implementation can be found [here https://github.com/google/snappy](https://github.com/google/snappy)

```
git clone https://github.com/google/snappy.git
cd snappy
mkdir build;cd build
cmake .. -DCMAKE_INSTALL_PREIX=<install path>
make -j8; make install
export SNAPPY_HOME=<install path>
```

## Download and compile the module

```
cd \<path to ospray\>/modules
git clone https://jgbarbosa@bitbucket.org/jgbarbosa/dw.git
cd ../
mkdir build

CC=<your favorite C compiler> CXX=<your favorite C compiler> cmake .. -DOSPRAY_MODULE_DISPLAYWALL=ON -DOSPRAY_MODULE_MPI=ON

make -j 8
```


## Executing

### Environment variables configuration

 Environment Variable  |  Values  | Description  |
 --------------------- | -------- | -------------|
 DW_HOSTNAME | string | FQCN Hostname of the display head node |
 DW_HOSTPORT | int | Display head node port number to listen/connect to|
 DW_CONFIG_FILE | string | Display configuration file |
 DW_FULLSCREEN | 0/1 | Fullscreen in the display wall nodes (except head node) |
 
### Display wall configuration file
 
```
 monitor_with monitor_height
 number_monitor_x number_monitors_y
 basel_compensation_x basel_compensation_y
 hosts_orientation
```
 
 hosts_orientation
 * 0 - Sorted by x coordinate first
 * 1 - Sorted by y coordinate first
 
### 3x3 Screen hosts order maching (X coordiante)
 
 |  6  |  7   |  8   |
 |:---:|:----:|:----:|
 |  3  |  4   |  5   |
 |  0  |  1   |  2   |
 
### Launching the display
 
 First we launch the display, since it is the one waiting for the farm to connect to it. We will launch a ospApp on the head node and use dwDisplay to spwan the display
 window on the display nodes. 
 
```
 mpirun -n 1 ./ospExampleViewer --osp:module:dwdisplay --osp:device:dwdisplay <Visualiztion parameter> : -n <number of display nodes> ./dwDisplay --osp:module:dwdisplay --osp:device:dwdisplay
```

To disable full screen:
```
 export DW_FULLSCREEN=0
 mpirun -n 1 ./ospExampleViewer --osp:module:dwdisplay --osp:device:dwdisplay <Visualiztion parameter> : -n <number of display nodes> ./dwDisplay --osp:module:dwdisplay --osp:device:dwdisplay
```

By default it will try to load a file named _default.conf_. If we whish teh head node to read anotehr configuration file.

```
 export DW_CONF_FILE=<path_to>/other.conf
 mpirun -n 1 ./ospExampleViewer --osp:module:dwdisplay --osp:device:dwdisplay <Visualiztion parameter> : -n <number of display nodes> ./dwDisplay --osp:module:dwdisplay --osp:device:dwdisplay
```
 
### Launching farm
The farm will receive all data and commands from the display wall head node. And for now we will loose one of the nodes to handle communication,

```
 mpirun -n <number of nodes> ./dwFarm -osp:module:dwdisplay --osp:device:dwdisplay 
```

### Notes:

    - When compiling for large display walls use larger TILE_SIZE (cmake <other options> -DTILE_SIZE=256)
    - Use the same compiler for the display wall cluster and rendering cluster

### TODO:

    - [x] Compressed/Decompress all TCOP connections
    - [ ] Fix basel compensation code
    - [ ] Create single window client
    - [ ] Implement multiple render farm deployment
    - [ ] Implement window manager in the client side
