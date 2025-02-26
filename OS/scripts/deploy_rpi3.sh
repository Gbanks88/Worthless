#!/bin/bash

# Configuration
RPI_IP="raspberrypi.local"  # Change this to your Pi's IP or hostname
RPI_USER="pi"               # Change this to your Pi's username
BUILD_DIR="build/rpi3"
TEST_DIR="tests"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "Building for Raspberry Pi 3..."

# Create build directory
mkdir -p $BUILD_DIR

# Build the project
make -f config/rpi3.mk clean
make -f config/rpi3.mk

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Create test directory on RPi
echo "Creating test directory on Raspberry Pi..."
ssh $RPI_USER@$RPI_IP "mkdir -p ~/os_test"

# Copy files to RPi
echo "Copying files to Raspberry Pi..."
scp $BUILD_DIR/kernel7.img $RPI_USER@$RPI_IP:~/os_test/
scp $TEST_DIR/test_ipc_rpi3 $RPI_USER@$RPI_IP:~/os_test/

# Run tests
echo "Running tests on Raspberry Pi..."
ssh $RPI_USER@$RPI_IP "cd ~/os_test && sudo ./test_ipc_rpi3"

echo -e "${GREEN}Deployment and testing completed!${NC}"
