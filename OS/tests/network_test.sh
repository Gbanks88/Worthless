#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Testing Network Connection on Raspberry Pi 3...${NC}\n"

# Test 1: Check if ethernet interface is up
echo "1. Checking Ethernet Interface..."
if ip link show eth0 | grep -q "state UP"; then
    echo -e "${GREEN}✓ Ethernet interface is UP${NC}"
else
    echo -e "${RED}✗ Ethernet interface is DOWN${NC}"
    echo "Try: Check if the Ethernet cable is properly connected"
fi

# Test 2: Check link speed
echo -e "\n2. Checking Link Speed..."
SPEED=$(ethtool eth0 | grep "Speed:" | cut -d: -f2)
if [ ! -z "$SPEED" ]; then
    echo -e "${GREEN}✓ Link speed:$SPEED${NC}"
else
    echo -e "${RED}✗ Could not determine link speed${NC}"
    echo "Try: Check cable quality and network switch speed"
fi

# Test 3: Check for packet errors
echo -e "\n3. Checking for Packet Errors..."
RX_ERRORS=$(ip -s link show eth0 | grep "RX:" -A 1 | tail -n 1 | awk '{print $3}')
TX_ERRORS=$(ip -s link show eth0 | grep "TX:" -A 1 | tail -n 1 | awk '{print $3}')
if [ "$RX_ERRORS" -eq 0 ] && [ "$TX_ERRORS" -eq 0 ]; then
    echo -e "${GREEN}✓ No packet errors detected${NC}"
else
    echo -e "${RED}✗ Packet errors detected${NC}"
    echo "RX errors: $RX_ERRORS"
    echo "TX errors: $TX_ERRORS"
    echo "Try: Check for cable damage or interference"
fi

# Test 4: Check DNS resolution
echo -e "\n4. Testing DNS Resolution..."
if ping -c 1 google.com &> /dev/null; then
    echo -e "${GREEN}✓ DNS resolution working${NC}"
else
    echo -e "${RED}✗ DNS resolution failed${NC}"
    echo "Try: Check network DNS settings"
fi

# Test 5: Check network latency
echo -e "\n5. Testing Network Latency..."
PING_RESULT=$(ping -c 3 8.8.8.8 | tail -1 | awk '{print $4}' | cut -d '/' -f 2)
if [ ! -z "$PING_RESULT" ]; then
    echo -e "${GREEN}✓ Average latency: ${PING_RESULT}ms${NC}"
else
    echo -e "${RED}✗ Could not measure latency${NC}"
    echo "Try: Check network connectivity"
fi

# Test 6: Check cable connection
echo -e "\n6. Checking Physical Connection..."
if ethtool eth0 | grep -q "Link detected: yes"; then
    echo -e "${GREEN}✓ Physical connection detected${NC}"
else
    echo -e "${RED}✗ No physical connection detected${NC}"
    echo "Try: 
    1. Check if Ethernet cable is properly plugged in
    2. Try a different Ethernet cable
    3. Try a different port on your router/switch
    4. Check if the network adapter is enabled"
fi

# Summary
echo -e "\n${YELLOW}Network Test Summary:${NC}"
echo "• If any tests failed, try the suggested solutions"
echo "• For persistent issues, try:"
echo "  - Rebooting the Raspberry Pi"
echo "  - Testing with a different Ethernet cable"
echo "  - Testing with a different network port"
echo "  - Checking router/switch configuration"

# Additional network information
echo -e "\n${YELLOW}Additional Network Information:${NC}"
echo "IP Configuration:"
ip addr show eth0

echo -e "\nRouting Table:"
ip route

echo -e "\nNetwork Statistics:"
netstat -i | grep eth0
