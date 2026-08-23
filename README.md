# High Resource Cuckatoo Miner

### Description
Cuckatoo miner for Windows, macOS, and Linux based on [Nicolas Flamel's work](https://github.com/NicolasFlamel1) optimized for the [Clang compiler](https://clang.llvm.org/) that supports cuckatoo10 to cuckatoo32.

This program utilizes a system's CPU and GPU together during the mining process to maximize its mining rate without regard for power usage. This is done in a pipelined approach where the GPU performs the initial edge trimming rounds and transfers the remaining edges to the CPU for the next graph while the CPU performs the remaining edge trimming rounds and searches the remaining edges for a solution for the current graph. If a solution is found then the CPU and GPU work together to recover the edges for that solution. This pipelined approach is visualized in the following table. The CPU and GPU stages in a single column in the table ideally take the same amount of time to complete so that neither the CPU or GPU has to wait very long for the other to complete its current stage. The CPU edge searching stage only scales up to four CPU cores in terms of speed, so most of the additional CPU tasks, like socket I/O and BLAKE2b hashing, are performed during that stage since at least one CPU core is unutilized at that time.

| Time          | →    | →       | →    | →       | →    | →       | Only If Solution Is Found | →    | →       | →    | →        | →   | →       |
| ------------- | ---- | -------- | ---- | -------- | ---- | -------- | ------------------------- | ---- | -------- | ---- | -------- | ---- | -------- |
| GPU Graph     | 1    | 1        | 2    | 2        | 3    | 3        | 2                         | 4    | 4        | 5    | 5        | 6    | 6        |
| GPU Step      | Trim | Transfer | Trim | Transfer | Trim | Transfer | Recover                   | Trim | Transfer | Trim | Transfer | Trim | Transfer |
| CPU Graph     |      |          | 1    | 1        | 2    | 2        | 2                         | 3    | 3        | 4    | 4        | 5    | 5        |
| CPU Step      |      |          | Trim | Search   | Trim | Search   | Recover                   | Trim | Search   | Trim | Search   | Trim | Search   |
| Graph Started | 1    |          | 2    |          | 3    |          |                           | 4    |          | 5    |          | 6    |          |
| Graph Ended   |      |          |      |          | 1    |          |                           | 2    |          | 3    |          | 4    |          |

This program requires approximately 0.28 GB of RAM and 27 GB of VRAM to perform cuckatoo31 mining, and this program requires approximately 0.38 GB of RAM and 52 GB of VRAM to perform cuckatoo32 mining. As such, this program can perform cuckatoo32 mining on any Apple silicon device that has at least 64 GB of unified memory.

### Building
It's recommended that you build this program on the same system that you'll be running it on since it uses the `-mtune=native` and `-march=native` compiler flags to optimize itself for the current system's available features.

#### Windows
After downloading [LLVM MinGW](https://github.com/mstorsjo/llvm-mingw), extracting its contents, and adding its `bin` folder to your `PATH` environment variable, this program can be built and ran with Windows by running the following commands in a command prompt or MSYS shell from the root of this project:
```
mingw32-make
mingw32-make run
```

#### macOS
After installing [Xcode](https://developer.apple.com/xcode), this program can be built and ran with macOS by running the following commands in a terminal from the root of this project:
````
make
make run
````

#### Linux
This program can be built and ran with Linux by running the following commands in a terminal from the root of this project:
```
sudo apt install libc++-dev
make
make run
```

### Usage
A stratum server address, port, and username can be provided when running this program to set the stratum server that it will mine to. For example, the following command will connect to the stratum server with the address `127.0.0.1` at port `3416` using the username `username`. After this program connects to a stratum server, it will start mining and submit all valid solutions that it finds to that stratum server regardless of each solution's difficulty in relation to that stratum server's minimum solution difficulty.
```
"./High Resource Cuckatoo Miner" -a 127.0.0.1 -p 3416 -u username
```

### Tuning
All of this programs tuning settings are provided at build time and they are hard coded into the program. The values for all the tuning settings are verified at build time, so this program will fail to build if you attempt to use a setting that is invalid or outside of its expected range. Here are all the tuning settings available:

* An `EDGE_BITS` setting can be used to set the cuckatoo variation to perform. Set this to work with the cryptocurrency that you want to mine, such as `32` for cuckatoo32. The default value for this setting is `32`.
```
make EDGE_BITS=32
```

* A `GPU_TRIMMING_ROUNDS` setting can be used to set the number of trimming rounds performed by the GPU. This settings affects the GPU trimming speed and GPU transferring speed. The default value for this setting is `5`.
```
make GPU_TRIMMING_ROUNDS=5
```

* A `CPU_TRIMMING_ROUNDS` setting can be used to set the number of trimming rounds performed by the CPU. This settings affects the CPU trimming speed and CPU searching speed. The default value for this setting is `335`.
```
make CPU_TRIMMING_ROUNDS=335
```

* A `SOLUTION_SIZE` setting can be used to set the size of a valid solution. Set this to work with the cryptocurrency that you want to mine. The default value for this setting is `42`.
```
make SOLUTION_SIZE=42
```

* A `HEADER_SIZE_EXCLUDING_NONCE` setting can be used to set the size of the block header excluding its nonce to find a solution for. Set this to work with the cryptocurrency that you want to mine. The default value for this setting is `238`.
```
make HEADER_SIZE_EXCLUDING_NONCE=238
```

* A `GPU_TRIMMING_USE_MORE_RAM` setting can be used to enable using a faster version of GPU trimming that uses more VRAM. This settings affects the GPU trimming speed. The default value for this setting is `false`.
```
make GPU_TRIMMING_USE_MORE_RAM=false
```

* A `GPU_TRIMMING_USE_LESS_RAM` setting can be used to enable using a slower version of GPU trimming that uses less VRAM. This settings affects the GPU trimming speed. The default value for this setting is `false`.
```
make GPU_TRIMMING_USE_LESS_RAM=false
```

* A `GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING` setting can be used to set the number of most significant bits used for coarse bucket sorting by the GPU. This settings affects the GPU trimming speed. The default value for this setting is `8`.
```
make GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING=8
```

* A `GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING` setting can be used to set the number of most significant bits used for fine bucket sorting by the GPU. This settings affects the GPU trimming speed. The default value for this setting is `7`.
```
make GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING=7
```

* A `GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU coarse bucket sort edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `512`.
```
make GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=512
```

* A `GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU fine bucket sort initial edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `512`.
```
make GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=512
```

* A `GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU trim initial edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `512`.
```
make GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=512
```

* A `GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU fine bucket sort intermediate edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `512`.
```
make GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=512
```

* A `GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU trim intermediate edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `1024`.
```
make GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=1024
```

* A `GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU fine bucket sort final edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `1024`.
```
make GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=1024
```

* A `GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU trim final edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `1024`.
```
make GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=1024
```

* A `GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU trim final edges and transfer edges kernel. This settings affects the GPU trimming speed. The default value for this setting is `1024`.
```
make GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=1024
```

* A `CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING` setting can be used to set the number of most significant bits used for fine bucket sorting by the CPU. This settings affects the CPU trimming speed. The default value for this setting is `6`.
```
make CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING=6
```

* A `CPU_TRIMMING_VECTOR_SCALE_FACTOR` setting can be used to set the trimming vector scale factor used by the CPU. This settings affects the CPU trimming speed. The default value for this setting is `4`.
```
make CPU_TRIMMING_VECTOR_SCALE_FACTOR=4
```

* A `CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING` setting can be used to set the number of trimming rounds performed by the CPU before it compresses the edges. This settings affects the CPU trimming speed. The default value for this setting is `25`.
```
make CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING=25
```

* A `CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING` setting can be used to perform CPU searching during GPU trimming. This settings combines the CPU trimming speed and CPU searching speed. The default value for this setting is `true`.
```
make CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING=true
```

* A `GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM` setting can be used to set the number of edges per work item for the GPU recover edges kernel. This settings affects the GPU recovering speed. The default value for this setting is `512`.
```
make GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM=512
```

* A `GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP` setting can be used to set the number of work items per work group for the GPU recover edges kernel. This settings affects the GPU recovering speed. The default value for this setting is `64`.
```
make GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=64
```

* A `GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM` setting can be used to set the number of recovered edge candidates per work item for the GPU recover edges kernel. This settings affects the GPU recovering speed. The default value for this setting is `8`.
```
make GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM=8
```

* A `CPU_RECOVERING_PERCENT` setting can be used to set the percentage of edges recovered by the CPU. This settings affects the CPU recovering speed and GPU recovering speed. The default value for this setting is `0.143`.
```
make CPU_RECOVERING_PERCENT=0.143
```

* A `CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP` setting can be used to set the number of most significant bits used for recovering bitmap by the CPU. This settings affects the CPU recovering speed. The default value for this setting is `18`.
```
make CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP=18
```

* A `CPU_RECOVERING_VECTOR_SCALE_FACTOR` setting can be used to set the recovering vector scale factor used by the CPU. This settings affects the CPU recovering speed. The default value for this setting is `4`.
```
make CPU_RECOVERING_VECTOR_SCALE_FACTOR=4
```

* A `GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES` setting can be used to set an additional amount of memory to configure the GPU to use. The default value for this setting is `1024`.
```
make GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES=1024
```

* A `STRATUM_SERVER_DEFAULT_ADDRESS` setting can be used to set the default stratum server address. The default value for this setting is `localhost`.
```
make STRATUM_SERVER_DEFAULT_ADDRESS=localhost
```

* A `STRATUM_SERVER_DEFAULT_PORT` setting can be used to set the default stratum server port. The default value for this setting is `3416`.
```
make STRATUM_SERVER_DEFAULT_PORT=3416
```

* A `STRATUM_SERVER_READ_TIMEOUT_SECONDS` setting can be used to set the read timeout for the stratum server. The default value for this setting is `60`.
```
make STRATUM_SERVER_READ_TIMEOUT_SECONDS=60
```

* A `STRATUM_SERVER_WRITE_TIMEOUT_SECONDS` setting can be used to set the write timeout for the stratum server. The default value for this setting is `60`.
```
make STRATUM_SERVER_WRITE_TIMEOUT_SECONDS=60
```

* A `STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS` setting can be used to set the reconnect after failure delay for the stratum server. The default value for this setting is `1`.
```
make STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS=1
```

* A `STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES` setting can be used to set the receive buffer size for the stratum server. The default value for this setting is `10`.
```
make STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES=10
```

* A `STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS` setting can be used to set the keep alive request interval for the stratum server. The default value for this setting is `10`.
```
make STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS=10
```

* A `DISPLAY_TUNING_TIMES` setting can be used to display the duration and number of edges remaining for some of the CPU stages. This setting is intended to be used by developers. The default value for this setting is `false`.
```
make DISPLAY_TUNING_TIMES=false
```

* A `RECOVER_EDGES_FOR_EVERY_GRAPH` setting can be used to enable recovering the edges for every graph instead of just the graphs that contain a solution. This setting is intended to be used by developers. The default value for this setting is `false`.
```
make RECOVER_EDGES_FOR_EVERY_GRAPH=false
```

* A `MINE_TO_A_STRATUM_SERVER` setting can be used to disable connecting to a stratum server. This setting is intended to be used by developers. The default value for this setting is `true`.
```
make MINE_TO_A_STRATUM_SERVER=true
```

* A `DISPLAY_STRATUM_SERVER_MESSAGES` setting can be used to display all messages sent to and received from the stratum server. This setting is intended to be used by developers. The default value for this setting is `false`.
```
make DISPLAY_STRATUM_SERVER_MESSAGES=false
```

* An `EMBED_GPU_CODE` setting can be used to enable reading the GPU kernels from a file instead of hard coding the GPU kernels into the program. This setting is intended to be used by developers. The default value for this setting is `true`.
```
make EMBED_GPU_CODE=true
```

* A `USE_SIGNAL_HANDLER` setting can be used to disable catching the signal that terminates this program cleanly. This setting is intended to be used by developers. The default value for this setting is `true`.
```
make USE_SIGNAL_HANDLER=true
```

* A `CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS` setting can be used to display a message when an edge is lost during CPU trimming due to lack of bucket space. This setting is intended to be used by developers. The default value for this setting is `true`.
```
make CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS=true
```

This program's default tuning settings are set for cuckatoo32 and should be fine tuned by you on the system that you'll be running it on in order to maximize its performance. If you want to perform cuckatoo31 with it, then building this program with the following tuning settings will give you a good starting point which you'll need to further fine tune on your system to maximize its performance:
```
make EDGE_BITS=31 CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING=15
```
