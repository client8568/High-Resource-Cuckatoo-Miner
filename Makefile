# Settings

# Edge bits
EDGE_BITS = 32

# GPU trimming rounds
GPU_TRIMMING_ROUNDS = 5

# CPU trimming rounds
CPU_TRIMMING_ROUNDS = 335

# Solution size
SOLUTION_SIZE = 42

# Nonce size
NONCE_SIZE = 8

# Nonce in header is big endian
NONCE_IN_HEADER_IS_BIG_ENDIAN = true

# Header size excluding nonce
HEADER_SIZE_EXCLUDING_NONCE = 238

# GPU trimming use max RAM
GPU_TRIMMING_USE_MAX_RAM = false

# GPU trimming use more RAM
GPU_TRIMMING_USE_MORE_RAM = false

# GPU trimming use less RAM
GPU_TRIMMING_USE_LESS_RAM = false

# GPU trimming use min RAM
GPU_TRIMMING_USE_MIN_RAM = false

# GPU number of most significant bits used for coarse bucket sorting
GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING = 8

# GPU number of most significant bits used for initial fine bucket sorting
GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING = 8

# GPU number of most significant bits used for fine bucket sorting
GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING = 7

# GPU coarse bucket sort edges kernel number of work items per work group
GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 512

# GPU fine bucket sort initial edges kernel number of work items per work group
GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 512

# GPU trim initial edges kernel number of work items per work group
GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 512

# GPU fine bucket sort intermediate edges kernel number of work items per work group
GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 512

# GPU trim intermediate edges kernel number of work items per work group
GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 1024

# GPU fine bucket sort final edges kernel number of work items per work group
GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 1024

# GPU trim final edges kernel number of work items per work group
GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 1024

# GPU trim final edges and transfer edges kernel number of work items per work group
GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 1024

# CPU number of most significant bits used for fine bucket sorting
CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING = 6

# CPU trimming use max RAM
CPU_TRIMMING_USE_MAX_RAM = false

# CPU trimming use more RAM
CPU_TRIMMING_USE_MORE_RAM = true

# CPU trimming vector scale factor
CPU_TRIMMING_VECTOR_SCALE_FACTOR = 4

# CPU trimming rounds before compressing
CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING = 25

# CPU perform searching during GPU trimming
CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING = true

# GPU recover edges kernel number of edges per work item
GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM = 512

# GPU recover edges kernel number of work items per work group
GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP = 64

# GPU recover edges kernel number of recovered edge candidates per work item
GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM = 8

# CPU recovering percent
CPU_RECOVERING_PERCENT = 0.143

# CPU number of most significant bits used for recovering bitmap
CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP = 18

# CPU recovering vector scale factor
CPU_RECOVERING_VECTOR_SCALE_FACTOR = 4

# GPU set memory size additional space megabytes
GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES = 1024

# Stratum server default address
STRATUM_SERVER_DEFAULT_ADDRESS = localhost

# Stratum server default port
STRATUM_SERVER_DEFAULT_PORT = 3416

# Stratum server read timeout seconds
STRATUM_SERVER_READ_TIMEOUT_SECONDS = 60

# Stratum server write timeout seconds
STRATUM_SERVER_WRITE_TIMEOUT_SECONDS = 60

# Stratum server reconnect after failure delay seconds
STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS = 1

# Stratum server receive buffer size kilobytes
STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES = 10

# Stratum server send keep alive request interval seconds
STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS = 10

# Display tuning times
DISPLAY_TUNING_TIMES = false

# Recover edges for every graph
RECOVER_EDGES_FOR_EVERY_GRAPH = false

# Mine to a stratum server
MINE_TO_A_STRATUM_SERVER = true

# Display power usage
DISPLAY_POWER_USAGE = true

# Prevent sleep
PREVENT_SLEEP = true

# Display stratum server messages
DISPLAY_STRATUM_SERVER_MESSAGES = false

# Embed GPU code
EMBED_GPU_CODE = true

# Use signal handler
USE_SIGNAL_HANDLER = true

# CPU trimming bounds checking avoids conditional statements
CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS = true

# Starting nonce
STARTING_NONCE = 0

# Starting header
STARTING_HEADER =

# Stop after number of graphs
STOP_AFTER_NUMBER_OF_GRAPHS = 0


# Constants

# Name
NAME = High Resource Cuckatoo Miner

# Version
VERSION = 0.0.1

# Compiler
CC = clang++

# Strip
STRIP = strip

# Flags
CFLAGS = -stdlib=libc++ -fexperimental-library -std=c++23 -mtune=native -march=native -fno-rtti -fno-exceptions -O3 -ffast-math -flto=full -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-value -Wno-vla-cxx-extension -Wno-missing-designated-field-initializers -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-tautological-constant-out-of-range-compare -Wno-overlength-strings -Wno-nonnull -D NAME="$(NAME)" -D VERSION=$(VERSION) -D EDGE_BITS=$(EDGE_BITS) -D GPU_TRIMMING_ROUNDS=$(GPU_TRIMMING_ROUNDS) -D CPU_TRIMMING_ROUNDS=$(CPU_TRIMMING_ROUNDS) -D SOLUTION_SIZE=$(SOLUTION_SIZE) -D NONCE_SIZE=$(NONCE_SIZE) -D NONCE_IN_HEADER_IS_BIG_ENDIAN=$(NONCE_IN_HEADER_IS_BIG_ENDIAN) -D HEADER_SIZE_EXCLUDING_NONCE=$(HEADER_SIZE_EXCLUDING_NONCE) -D GPU_TRIMMING_USE_MAX_RAM=$(GPU_TRIMMING_USE_MAX_RAM) -D GPU_TRIMMING_USE_MORE_RAM=$(GPU_TRIMMING_USE_MORE_RAM) -D GPU_TRIMMING_USE_LESS_RAM=$(GPU_TRIMMING_USE_LESS_RAM) -D GPU_TRIMMING_USE_MIN_RAM=$(GPU_TRIMMING_USE_MIN_RAM) -D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING=$(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING) -D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING=$(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING) -D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING=$(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING) -D GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING=$(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING) -D CPU_TRIMMING_USE_MAX_RAM=$(CPU_TRIMMING_USE_MAX_RAM) -D CPU_TRIMMING_USE_MORE_RAM=$(CPU_TRIMMING_USE_MORE_RAM) -D CPU_TRIMMING_VECTOR_SCALE_FACTOR=$(CPU_TRIMMING_VECTOR_SCALE_FACTOR) -D CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING=$(CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING) -D CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING=$(CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING) -D GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM=$(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM) -D GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=$(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) -D GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM=$(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM) -D CPU_RECOVERING_PERCENT=$(CPU_RECOVERING_PERCENT) -D CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP=$(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP) -D CPU_RECOVERING_VECTOR_SCALE_FACTOR=$(CPU_RECOVERING_VECTOR_SCALE_FACTOR) -D GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES=$(GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES) -D STRATUM_SERVER_DEFAULT_ADDRESS="$(STRATUM_SERVER_DEFAULT_ADDRESS)" -D STRATUM_SERVER_DEFAULT_PORT="$(STRATUM_SERVER_DEFAULT_PORT)" -D STRATUM_SERVER_READ_TIMEOUT_SECONDS=$(STRATUM_SERVER_READ_TIMEOUT_SECONDS) -D STRATUM_SERVER_WRITE_TIMEOUT_SECONDS=$(STRATUM_SERVER_WRITE_TIMEOUT_SECONDS) -D STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS=$(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS) -D STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES=$(STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES) -D STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS=$(STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS) -D DISPLAY_TUNING_TIMES=$(DISPLAY_TUNING_TIMES) -D RECOVER_EDGES_FOR_EVERY_GRAPH=$(RECOVER_EDGES_FOR_EVERY_GRAPH) -D MINE_TO_A_STRATUM_SERVER=$(MINE_TO_A_STRATUM_SERVER) -D DISPLAY_POWER_USAGE=$(DISPLAY_POWER_USAGE) -D PREVENT_SLEEP=$(PREVENT_SLEEP) -D DISPLAY_STRATUM_SERVER_MESSAGES=$(DISPLAY_STRATUM_SERVER_MESSAGES) -D EMBED_GPU_CODE=$(EMBED_GPU_CODE) -D USE_SIGNAL_HANDLER=$(USE_SIGNAL_HANDLER) -D CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS=$(CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS) -D STARTING_NONCE=$(STARTING_NONCE) -D STARTING_HEADER_SIZE=$(words $(subst A,A ,$(subst B,B ,$(subst C,C ,$(subst D,D ,$(subst E,E ,$(subst F,F ,$(subst G,G ,$(subst H,H ,$(subst I,I ,$(subst J,J ,$(subst K,K ,$(subst L,L ,$(subst M,M ,$(subst N,N ,$(subst O,O ,$(subst P,P ,$(subst Q,Q ,$(subst R,R ,$(subst S,S ,$(subst T,T ,$(subst U,U ,$(subst V,V ,$(subst W,W ,$(subst X,X ,$(subst Y,Y ,$(subst Z,Z, $(subst a,a ,$(subst b,b ,$(subst c,c ,$(subst d,d ,$(subst e,e ,$(subst f,f ,$(subst g,g ,$(subst h,h ,$(subst i,i ,$(subst j,j ,$(subst k,k ,$(subst l,l ,$(subst m,m ,$(subst n,n ,$(subst o,o ,$(subst p,p ,$(subst q,q ,$(subst r,r ,$(subst s,s ,$(subst t,t ,$(subst u,u ,$(subst v,v ,$(subst w,w ,$(subst x,x ,$(subst y,y ,$(subst z,z ,$(subst $() $(),a,$(STARTING_HEADER))))))))))))))))))))))))))))))))))))))))))))))))))))))) -D STARTING_HEADER="$(STARTING_HEADER)" -D STOP_AFTER_NUMBER_OF_GRAPHS=$(STOP_AFTER_NUMBER_OF_GRAPHS)

# Libraries
LIBS =

# Check if compiling for Windows
ifeq ($(OS),Windows_NT)

	# Program name
	PROGRAM_NAME = $(NAME).exe
	
	# Set flags and link libraries
	CFLAGS += -static-libstdc++ -static-libgcc -I"./opencl_headers"
	LIBS += -lws2_32 -lsetupapi
	
	# Check if using MSYS shell
	ifneq (,$(MSYSTEM))
	
		# Set flags and link libraries
		LIBS += "$(shell echo $$SYSTEMROOT)\System32\OpenCL.dll"
		
		# Delete command
		DELETE_COMMAND = rm -rf
		
		# Null location
		NULL_LOCATION = "/dev/null"
		
	# Otherwise
	else
	
		# Set flags and link libraries
		LIBS += "$(shell echo %SYSTEMROOT%)\System32\OpenCL.dll"
		
		# Delete command
		DELETE_COMMAND = del /q
		
		# Null location
		NULL_LOCATION = "nul"
	endif
	
# Otherwise check if compiling for macOS
else ifeq ($(shell uname),Darwin)

	# Program name
	PROGRAM_NAME = $(NAME)
	
	# Set flags and link libraries
	LIBS += -framework Foundation -framework IOKit -framework Metal
	
	# Check if displaying power usage
	ifeq ($(DISPLAY_POWER_USAGE),true)
	
		# Set flags and link libraries
		LIBS += -lIOReport
	endif
	
	# Delete command
	DELETE_COMMAND = rm -rf
	
	# Null location
	NULL_LOCATION = "/dev/null"
	
# Otherwise
else

	# Program name
	PROGRAM_NAME = $(NAME)
	
	# Set flags and link libraries
	CFLAGS += -static-libstdc++ -static-libgcc -I"./opencl_headers"
	LIBS += -Wl,-Bstatic -L"./opencl_loader/dist/linux/$(shell uname -m)/lib" -lOpenCL -Wl,-Bdynamic
	
	# Check if preventing sleep
	ifeq ($(PREVENT_SLEEP),true)
	
		# Set flags and link libraries
		CFLAGS += `pkg-config --cflags dbus-1`
		LIBS += `pkg-config --libs dbus-1`
	endif
	
	# Delete command
	DELETE_COMMAND = rm -rf
	
	# Null location
	NULL_LOCATION = "/dev/null"
endif


# Commands

# All
all:
	"$(shell echo $(CC))" $(CFLAGS) -o "./$(PROGRAM_NAME)" "./main.cpp" $(LIBS)
	"$(shell echo $(STRIP))" "./$(PROGRAM_NAME)"
	
# Run
run:
	"./$(PROGRAM_NAME)"
	
# Clean
clean:
	$(DELETE_COMMAND) "./$(NAME)" "./$(NAME).exe" "./v2026.05.29.tar.gz" "./OpenCL-Headers-2026.05.29" "./OpenCL-ICD-Loader-2026.05.29" "./metal-cpp_macOS27_iOS27.zip" "./metal-cpp-release-metal-cpp_macOS27_iOS27" > $(NULL_LOCATION) 2>&1
	
# Make Linux dependencies (This command works when using Linux: make linuxDependencies)
linuxDependencies:
	
	# OpenCL headers
	rm -rf "./v2026.05.29.tar.gz" "./OpenCL-Headers-2026.05.29" "./opencl_headers"
	wget "https://github.com/KhronosGroup/OpenCL-Headers/archive/refs/tags/v2026.05.29.tar.gz"
	tar -xf "./v2026.05.29.tar.gz"
	rm "./v2026.05.29.tar.gz"
	mv "./OpenCL-Headers-2026.05.29" "./opencl_headers"
	
	# OpenCL loader
	rm -rf "./OpenCL-ICD-Loader-2026.05.29" "./opencl_loader"
	wget "https://github.com/KhronosGroup/OpenCL-ICD-Loader/archive/refs/tags/v2026.05.29.tar.gz"
	tar -xf "./v2026.05.29.tar.gz"
	mv "./OpenCL-ICD-Loader-2026.05.29" "./opencl_loader"
	cd "./opencl_loader" && rm -f "./CMakeCache.txt" && cmake -DCMAKE_INSTALL_PREFIX="$(CURDIR)/opencl_loader/dist/linux/x86_64" -DCMAKE_C_COMPILER_TARGET=x86_64-linux-gnu -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DOPENCL_ICD_LOADER_HEADERS_DIR="$(CURDIR)/opencl_headers" -DCMAKE_C_COMPILER="$(shell echo $(subst ++,,$(CC)))" -DCMAKE_C_FLAGS=-fmacro-prefix-map="$(shell pwd)"="." "./CMakeLists.txt" && make && make install && make clean
	cd "./opencl_loader" && rm -f "./CMakeCache.txt" && cmake -DCMAKE_INSTALL_PREFIX="$(CURDIR)/opencl_loader/dist/linux/aarch64" -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DOPENCL_ICD_LOADER_HEADERS_DIR="$(CURDIR)/opencl_headers" -DCMAKE_C_COMPILER="$(shell echo $(subst ++,,$(CC)))" -DCMAKE_C_FLAGS=-fmacro-prefix-map="$(shell pwd)"="." "./CMakeLists.txt" && make && make install && make clean
	find "./opencl_loader/dist/linux" ! -name "libOpenCL.a" -type f -delete && find "./opencl_loader/dist/linux" -empty -type d -delete
	x86_64-linux-gnu-strip --strip-unneeded "./opencl_loader/dist/linux/x86_64/lib/libOpenCL.a"
	aarch64-linux-gnu-strip --strip-unneeded "./opencl_loader/dist/linux/aarch64/lib/libOpenCL.a"
	tar -xf "./v2026.05.29.tar.gz"
	rm "./v2026.05.29.tar.gz"
	mv "./opencl_loader/dist" "./OpenCL-ICD-Loader-2026.05.29"
	rm -r "./opencl_loader"
	mv "./OpenCL-ICD-Loader-2026.05.29" "./opencl_loader"
	
# Make Apple dependencies (This command works when using macOS: make appleDependencies)
appleDependencies:
	
	# Metal-cpp
	rm -rf "./metal-cpp_macOS27_iOS27.zip" "./metal-cpp-release-metal-cpp_macOS27_iOS27" "./metal"
	curl -LO "https://github.com/apple/metal-cpp/archive/refs/tags/release/metal-cpp_macOS27_iOS27.zip"
	unzip "./metal-cpp_macOS27_iOS27.zip"
	rm "./metal-cpp_macOS27_iOS27.zip"
	mkdir "./metal"
	cd "./metal-cpp-release-metal-cpp_macOS27_iOS27" && "./SingleHeader/MakeSingleHeader.py" -o "../metal/metal.h" "./Metal/Metal.hpp"
	mv "./metal-cpp-release-metal-cpp_macOS27_iOS27/LICENSE.txt" "./metal/LICENSE"
	rm -r "./metal-cpp-release-metal-cpp_macOS27_iOS27"
	
# Make Windows dependencies (This command works when using Windows: mingw32-make windowsDependencies)
windowsDependencies:
	
	rem OpenCL headers
	del /q "./v2026.05.29.tar.gz" > "nul" 2>&1
	if exist "./OpenCL-Headers-2026.05.29" rd /q /s "./OpenCL-Headers-2026.05.29" > "nul"
	if exist "./opencl_headers" rd /q /s "./opencl_headers" > "nul"
	curl -LO "https://github.com/KhronosGroup/OpenCL-Headers/archive/refs/tags/v2026.05.29.tar.gz"
	tar -xf "./v2026.05.29.tar.gz"
	del "./v2026.05.29.tar.gz"
	rename "./OpenCL-Headers-2026.05.29" "./opencl_headers"
