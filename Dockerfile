FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python CLI dependencies
COPY cli/requirements.txt /tmp/
RUN pip3 install -r /tmp/requirements.txt

# Create app directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir build && cd build && \
    cmake .. && \
    make

# Expose port for REST API
EXPOSE 8080

# Default command
CMD ["./build/mini_sonic"]
