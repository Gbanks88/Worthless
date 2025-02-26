#!/bin/bash

# Configuration - CHANGE THESE AS NEEDED
RPI_IP="raspberrypi.local"  # Change this to your Pi's IP address if needed
RPI_USER="pi"              # Change this if your Pi uses a different username

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "Deploying test to Raspberry Pi..."

# Create remote directory
ssh $RPI_USER@$RPI_IP "mkdir -p ~/ipc_test"

# Copy test file
scp rpi3_simple_test.c $RPI_USER@$RPI_IP:~/ipc_test/

# Compile and run on the Pi
ssh $RPI_USER@$RPI_IP "cd ~/ipc_test && gcc -o test_program rpi3_simple_test.c && ./test_program"

echo -e "${GREEN}Test completed!${NC}"
