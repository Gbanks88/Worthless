#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
RPI_OS_URL="https://downloads.raspberrypi.org/raspios_lite_armhf_latest"
KINGSTON_DISK="disk2"  # Based on diskutil output
BOOT_PATH="/Volumes/boot"
ROOT_PATH="/Volumes/rootfs"

# Function to check if running with sudo
check_sudo() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Please run this script with sudo${NC}"
        exit 1
    fi
}

# Function to verify Kingston drive
verify_kingston() {
    if ! diskutil info $KINGSTON_DISK | grep -q "KINGSTON"; then
        echo -e "${RED}Kingston drive not found at $KINGSTON_DISK${NC}"
        echo "Available disks:"
        diskutil list
        exit 1
    fi
    
    echo -e "${YELLOW}Kingston drive found at /dev/$KINGSTON_DISK${NC}"
    echo -e "${RED}WARNING: All data on the Kingston drive will be erased!${NC}"
    echo -e "Are you sure you want to continue? (y/N)"
    read CONFIRM
    
    if [[ ! $CONFIRM =~ ^[Yy]$ ]]; then
        echo "Aborting."
        exit 1
    fi
}

# Function to download Raspberry Pi OS
download_os() {
    echo -e "${YELLOW}Downloading Raspberry Pi OS...${NC}"
    curl -L $RPI_OS_URL -o raspios.zip
    unzip raspios.zip
    rm raspios.zip
}

# Function to write image to Kingston drive
write_image() {
    local IMAGE_FILE=$(ls *.img)
    
    echo -e "${YELLOW}Unmounting Kingston drive...${NC}"
    diskutil unmountDisk /dev/$KINGSTON_DISK
    
    echo -e "${YELLOW}Writing image to Kingston drive...${NC}"
    echo -e "${RED}DO NOT DISCONNECT THE DRIVE${NC}"
    dd if=$IMAGE_FILE of=/dev/r$KINGSTON_DISK bs=1m status=progress
    
    echo -e "${YELLOW}Syncing...${NC}"
    sync
    
    # Wait for the OS to remount the partitions
    sleep 5
}

# Function to copy our OS files
copy_os_files() {
    echo -e "${YELLOW}Mounting boot partition...${NC}"
    if ! diskutil mount /dev/${KINGSTON_DISK}s1; then
        echo -e "${RED}Failed to mount boot partition${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Copying boot files...${NC}"
    cp /Users/developer/OS/build/rpi3/kernel7.img $BOOT_PATH/
    cp /Users/developer/OS/boot/config.txt $BOOT_PATH/
    cp /Users/developer/OS/boot/cmdline.txt $BOOT_PATH/
    
    echo -e "${YELLOW}Creating WiFi configuration...${NC}"
    echo "Enter WiFi SSID:"
    read WIFI_SSID
    echo "Enter WiFi password:"
    read -s WIFI_PASS
    
    cat > $BOOT_PATH/wpa_supplicant.conf << EOF
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="$WIFI_SSID"
    psk="$WIFI_PASS"
    key_mgmt=WPA-PSK
}
EOF
    
    # Enable SSH
    touch $BOOT_PATH/ssh
    
    echo -e "${YELLOW}Unmounting boot partition...${NC}"
    diskutil unmount $BOOT_PATH
}

# Function to configure the system
configure_system() {
    echo -e "${YELLOW}Mounting root partition...${NC}"
    if ! diskutil mount /dev/${KINGSTON_DISK}s2; then
        echo -e "${RED}Failed to mount root partition${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Configuring system...${NC}"
    
    # Set hostname
    echo "raspberrypi" > $ROOT_PATH/etc/hostname
    
    # Configure network
    cat > $ROOT_PATH/etc/network/interfaces.d/eth0 << EOF
auto eth0
iface eth0 inet dhcp
EOF
    
    # Set up automatic updates
    cat > $ROOT_PATH/etc/apt/apt.conf.d/20auto-upgrades << EOF
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Unattended-Upgrade "1";
EOF
    
    echo -e "${YELLOW}Unmounting root partition...${NC}"
    diskutil unmount $ROOT_PATH
}

# Function to verify the written image
verify_image() {
    echo -e "${YELLOW}Verifying boot partition...${NC}"
    if ! diskutil mount /dev/${KINGSTON_DISK}s1; then
        echo -e "${RED}Failed to mount boot partition for verification${NC}"
        return 1
    fi
    
    if [ -f "$BOOT_PATH/kernel7.img" ] && [ -f "$BOOT_PATH/config.txt" ] && [ -f "$BOOT_PATH/cmdline.txt" ]; then
        echo -e "${GREEN}Boot files verified successfully${NC}"
    else
        echo -e "${RED}Boot files verification failed${NC}"
        return 1
    fi
    
    diskutil unmount $BOOT_PATH
    return 0
}

# Main script
echo -e "${YELLOW}Raspberry Pi 3 Boot Disk Creator for Kingston Drive${NC}"
echo -e "${YELLOW}=============================================${NC}\n"

# Check if running with sudo
check_sudo

# Verify Kingston drive
verify_kingston

# Download Raspberry Pi OS
download_os

# Write the image
write_image

# Copy OS files
copy_os_files

# Configure the system
configure_system

# Verify the image
verify_image

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}Boot disk creation on Kingston drive complete!${NC}"
    echo -e "\nNext steps:"
    echo "1. Safely eject the Kingston drive"
    echo "2. Insert it into your Raspberry Pi"
    echo "3. Connect power and Ethernet"
    echo "4. Wait for the system to boot (about 1-2 minutes)"
    echo "5. Connect via SSH: ssh pi@raspberrypi.local"
    echo "   Default password: raspberry"
    echo -e "\n${YELLOW}Don't forget to change the default password!${NC}"
    
    # Safely eject the drive
    echo -e "\n${YELLOW}Safely ejecting Kingston drive...${NC}"
    diskutil eject /dev/$KINGSTON_DISK
else
    echo -e "\n${RED}Boot disk creation failed. Please check the errors above.${NC}"
fi
