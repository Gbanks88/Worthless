#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
RPI_OS_URL="https://downloads.raspberrypi.org/raspios_lite_armhf_latest"
BOOT_PATH="/Volumes/boot"
ROOT_PATH="/Volumes/rootfs"

# Function to check if running with sudo
check_sudo() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Please run this script with sudo${NC}"
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

# Function to identify the SD card
identify_disk() {
    echo -e "${YELLOW}Available disks:${NC}"
    diskutil list
    
    echo -e "\n${YELLOW}Enter the disk identifier for your SD card (e.g., disk2):${NC}"
    read DISK_ID
    
    if [[ ! $DISK_ID =~ ^disk[0-9]+$ ]]; then
        echo -e "${RED}Invalid disk identifier${NC}"
        exit 1
    fi
    
    echo -e "${RED}WARNING: All data on /dev/$DISK_ID will be erased!${NC}"
    echo -e "Are you sure you want to continue? (y/N)"
    read CONFIRM
    
    if [[ ! $CONFIRM =~ ^[Yy]$ ]]; then
        echo "Aborting."
        exit 1
    fi
}

# Function to write image to SD card
write_image() {
    local IMAGE_FILE=$(ls *.img)
    
    echo -e "${YELLOW}Unmounting disk...${NC}"
    diskutil unmountDisk /dev/$DISK_ID
    
    echo -e "${YELLOW}Writing image to SD card...${NC}"
    dd if=$IMAGE_FILE of=/dev/r$DISK_ID bs=1m status=progress
    
    echo -e "${YELLOW}Syncing...${NC}"
    sync
    
    echo -e "${YELLOW}Ejecting disk...${NC}"
    diskutil eject /dev/$DISK_ID
}

# Function to copy our OS files
copy_os_files() {
    echo -e "${YELLOW}Mounting boot partition...${NC}"
    diskutil mount /dev/${DISK_ID}s1
    
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
    diskutil mount /dev/${DISK_ID}s2
    
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

# Main script
echo -e "${YELLOW}Raspberry Pi 3 Boot Disk Creator${NC}"
echo -e "${YELLOW}=============================${NC}\n"

# Check if running with sudo
check_sudo

# Download Raspberry Pi OS
download_os

# Identify the SD card
identify_disk

# Write the image
write_image

# Copy OS files
copy_os_files

# Configure the system
configure_system

echo -e "${GREEN}Boot disk creation complete!${NC}"
echo -e "\nNext steps:"
echo "1. Insert the SD card into your Raspberry Pi"
echo "2. Connect power and Ethernet"
echo "3. Wait for the system to boot (about 1-2 minutes)"
echo "4. Connect via SSH: ssh pi@raspberrypi.local"
echo "   Default password: raspberry"
echo -e "\n${YELLOW}Don't forget to change the default password!${NC}"
