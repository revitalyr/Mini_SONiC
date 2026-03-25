FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
# Ubuntu 24.04 provides CMake 3.28+ which is required for C++20 modules support
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-14 g++-14 \
    cmake \
    git \
    ninja-build \
    python3 \
    python3-requests \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc-14
ENV CXX=g++-14

WORKDIR /app

COPY . .

# Build the project
# Prefer modular build if available (Mini SONiC modern architecture)
RUN if [ -f CMakeLists_modular.txt ]; then cp CMakeLists_modular.txt CMakeLists.txt; fi && \
    rm -rf build && \
    mkdir -p build && cd build && \
    cmake -GNinja .. && \
    ninja

# Ensure the executable is available as ./mini_sonic
RUN if [ -f build/mini_sonic_modular ]; then \
        cp build/mini_sonic_modular ./mini_sonic; \
    elif [ -f build/mini_sonic ]; then \
        cp build/mini_sonic ./mini_sonic; \
    fi

# Copy test executable if it exists
RUN if [ -f build/mini_sonic_tests ]; then \
        cp build/mini_sonic_tests ./mini_sonic_tests; \
    fi

CMD ["./mini_sonic"]