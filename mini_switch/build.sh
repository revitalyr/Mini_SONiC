#!/bin/bash
# Build script for Windows/Linux compatibility

echo "Building Mini Switch..."

# Create directories
mkdir -p obj
mkdir -p bin

# Compile source files
echo "Compiling source files..."
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/main.c -o obj/main.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/port.c -o obj/port.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/ring_buffer.c -o obj/ring_buffer.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/ethernet.c -o obj/ethernet.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/mac_table.c -o obj/mac_table.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/vlan.c -o obj/vlan.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/arp.c -o obj/arp.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/ip.c -o obj/ip.o
gcc -O2 -Wall -Wextra -pthread -Iinclude -g -c src/forwarding.c -o obj/forwarding.o

# Link
echo "Linking..."
gcc obj/*.o -o bin/mini_switch -pthread

echo "Build complete! Binary: bin/mini_switch"
