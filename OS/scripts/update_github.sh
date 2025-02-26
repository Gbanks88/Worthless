#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
REPO_URL="https://github.com/Gbanks88/Rasp-Pie.git"
BRANCH="main"

# Function to check git installation
check_git() {
    if ! command -v git &> /dev/null; then
        echo -e "${RED}Git is not installed. Please install git first.${NC}"
        exit 1
    fi
}

# Function to check if we're in a git repository
check_repo() {
    if ! git rev-parse --is-inside-work-tree &> /dev/null; then
        echo -e "${YELLOW}Initializing git repository...${NC}"
        git init
        git remote add origin $REPO_URL
    fi
}

# Function to stage and commit changes
commit_changes() {
    local message="$1"
    git add .
    git commit -m "$message"
}

# Main script
echo -e "${YELLOW}Updating Raspberry Pi files to GitHub...${NC}"

# Check git installation
check_git

# Navigate to project root
cd /Users/developer/OS

# Check repository
check_repo

# Create .gitignore if it doesn't exist
if [ ! -f .gitignore ]; then
    echo -e "${YELLOW}Creating .gitignore...${NC}"
    cat > .gitignore << EOF
# Build artifacts
build/
*.o
*.img

# System files
.DS_Store
.vscode/
*.swp

# Temporary files
*.tmp
*.bak

# Log files
*.log

# Dependencies
node_modules/
EOF
fi

# Create README.md if it doesn't exist
if [ ! -f README.md ]; then
    echo -e "${YELLOW}Creating README.md...${NC}"
    cat > README.md << EOF
# Raspberry Pi 3 Custom OS Project

This repository contains custom OS implementation and boot files for Raspberry Pi 3.

## Directory Structure

- \`/boot\`: Boot files and configurations
- \`/kernel\`: Kernel source files
- \`/include\`: Header files
- \`/scripts\`: Utility scripts
- \`/tests\`: Test programs

## Boot Files

- \`bootcode.bin\`: Initial boot loader
- \`start.elf\`: Main GPU firmware
- \`kernel7.img\`: Custom kernel image
- \`config.txt\`: Boot configuration
- \`cmdline.txt\`: Kernel command line parameters

## Features

- Custom IPC implementation
- Memory-mapped file support
- Socket multiplexing
- Priority FIFO system
- Secure RPC implementation

## Building

1. Install required tools:
   \`\`\`bash
   brew install arm-linux-gnueabihf-binutils
   \`\`\`

2. Build the kernel:
   \`\`\`bash
   make -f config/rpi3.mk
   \`\`\`

3. Create boot disk:
   \`\`\`bash
   sudo ./scripts/create_boot_disk.sh
   \`\`\`

## Testing

Run the test suite:
\`\`\`bash
./scripts/deploy_test.sh
\`\`\`

## License

MIT License

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request
EOF
fi

# Stage all files
echo -e "${YELLOW}Staging files...${NC}"
git add .

# Show status
echo -e "${YELLOW}Current status:${NC}"
git status

# Commit changes
echo -e "${YELLOW}Enter commit message (or press enter for default):${NC}"
read commit_msg
if [ -z "$commit_msg" ]; then
    commit_msg="Update Raspberry Pi OS files and configurations"
fi

echo -e "${YELLOW}Committing changes...${NC}"
commit_changes "$commit_msg"

# Push to GitHub
echo -e "${YELLOW}Pushing to GitHub...${NC}"
if git push origin $BRANCH; then
    echo -e "${GREEN}Successfully updated GitHub repository!${NC}"
else
    echo -e "${RED}Failed to push to GitHub. Please check your credentials and try again.${NC}"
    echo "You can manually push using:"
    echo "git push origin $BRANCH"
fi

echo -e "\n${YELLOW}Repository URL: $REPO_URL${NC}"
