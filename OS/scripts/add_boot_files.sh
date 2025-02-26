#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
KINGSTON_DISK="disk2"
BOOT_PATH="/Volumes/boot"
FIRMWARE_URL="https://github.com/raspberrypi/firmware/raw/master/boot"

# Required boot files
BOOT_FILES=(
    "bootcode.bin"
    "start.elf"
    "fixup.dat"
    "fixup_cd.dat"
    "start_cd.elf"
    "fixup_x.dat"
    "start_x.elf"
    "bcm2710-rpi-3-b.dtb"
    "bcm2710-rpi-3-b-plus.dtb"
    "overlays/disable-bt.dtbo"
    "overlays/disable-wifi.dtbo"
    "overlays/i2c-rtc.dtbo"
    "overlays/pi3-disable-bt.dtbo"
    "overlays/pi3-disable-wifi.dtbo"
    "overlays/spi-rtc.dtbo"
)

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Please run as root (use sudo)${NC}"
    exit 1
fi

# Verify Kingston drive
if ! diskutil info $KINGSTON_DISK | grep -q "KINGSTON"; then
    echo -e "${RED}Kingston drive not found at $KINGSTON_DISK${NC}"
    echo "Available disks:"
    diskutil list
    exit 1
fi

# Mount boot partition
echo -e "${YELLOW}Mounting boot partition...${NC}"
if ! diskutil mount /dev/${KINGSTON_DISK}s1; then
    echo -e "${RED}Failed to mount boot partition${NC}"
    exit 1
fi

# Create overlays directory if it doesn't exist
mkdir -p "$BOOT_PATH/overlays"

# Download and copy boot files
echo -e "${YELLOW}Downloading and copying boot files...${NC}"
for file in "${BOOT_FILES[@]}"; do
    echo -e "Processing $file..."
    
    # Create directory for overlays if needed
    dir=$(dirname "$BOOT_PATH/$file")
    mkdir -p "$dir"
    
    # Download file
    curl -L "$FIRMWARE_URL/$file" -o "$BOOT_PATH/$file"
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to download $file${NC}"
        continue
    fi
done

# Create custom config.txt if it doesn't exist
if [ ! -f "$BOOT_PATH/config.txt" ]; then
    echo -e "${YELLOW}Creating config.txt...${NC}"
    cat > "$BOOT_PATH/config.txt" << EOF
# Raspberry Pi 3 Configuration
arm_64bit=0
kernel=kernel7.img

# CPU configuration
arm_freq=1200
over_voltage=0
temp_limit=85

# Memory configuration
gpu_mem=128

# Display configuration
hdmi_force_hotplug=1
hdmi_group=1
hdmi_mode=16

# Network configuration
dtoverlay=disable-wifi
dtoverlay=disable-bt

# Enable hardware watchdog
dtparam=watchdog=on

# Enable I2C
dtparam=i2c_arm=on

# Enable SPI
dtparam=spi=on

# Enable audio
dtparam=audio=on

# Boot configuration
boot_delay=0
disable_splash=1
avoid_warnings=1
EOF
fi

# Create cmdline.txt if it doesn't exist
if [ ! -f "$BOOT_PATH/cmdline.txt" ]; then
    echo -e "${YELLOW}Creating cmdline.txt...${NC}"
    echo "console=serial0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait quiet init=/bin/systemd" > "$BOOT_PATH/cmdline.txt"
fi

# Copy our custom kernel if it exists
if [ -f "/Users/developer/OS/build/rpi3/kernel7.img" ]; then
    echo -e "${YELLOW}Copying custom kernel...${NC}"
    cp "/Users/developer/OS/build/rpi3/kernel7.img" "$BOOT_PATH/"
fi

# Create a README file
cat > "$BOOT_PATH/README.txt" << EOF
Raspberry Pi 3 Boot Files
========================

This drive contains all necessary files to boot a Raspberry Pi 3:

1. Core Boot Files:
   - bootcode.bin: Initial boot loader
   - start.elf: Main GPU firmware
   - fixup.dat: Memory configuration

2. Device Tree Files:
   - bcm2710-rpi-3-b.dtb: Device tree for Pi 3 Model B
   - bcm2710-rpi-3-b-plus.dtb: Device tree for Pi 3 Model B+

3. Configuration:
   - config.txt: Main configuration file
   - cmdline.txt: Kernel command line parameters

4. Custom Files:
   - kernel7.img: Custom kernel (if present)
   - Various overlay files in /overlays

Default login:
Username: pi
Password: raspberry

Remember to change the password on first boot!
EOF

# Verify files
echo -e "${YELLOW}Verifying boot files...${NC}"
missing_files=0
for file in "${BOOT_FILES[@]}"; do
    if [ ! -f "$BOOT_PATH/$file" ]; then
        echo -e "${RED}Missing: $file${NC}"
        missing_files=$((missing_files + 1))
    fi
done

if [ $missing_files -eq 0 ]; then
    echo -e "${GREEN}All boot files present and verified!${NC}"
else
    echo -e "${RED}Warning: $missing_files files are missing${NC}"
fi

# Calculate total size
total_size=$(du -sh "$BOOT_PATH" | cut -f1)
echo -e "${YELLOW}Total size of boot files: $total_size${NC}"

# Sync and unmount
echo -e "${YELLOW}Syncing files...${NC}"
sync

echo -e "${YELLOW}Unmounting boot partition...${NC}"
diskutil unmount "$BOOT_PATH"

echo -e "${GREEN}Boot files installation complete!${NC}"
echo -e "\nNext steps:"
echo "1. Insert the Kingston drive into your Raspberry Pi 3"
echo "2. Connect power and wait for boot (1-2 minutes)"
echo "3. Connect via SSH: ssh pi@raspberrypi.local"
echo -e "${YELLOW}Don't forget to change the default password!${NC}"
