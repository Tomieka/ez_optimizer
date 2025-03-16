<p align="center">
  <img src="logo.png" />
</p>

# Pacman Cache Cleaner and System Optimizer

A comprehensive Qt application to manage and optimize your Arch Linux system.

## Features

### Cache Management
- Display and clear the Pacman package manager cache
- One-click cache clearing

### Orphaned Packages Management
- List and remove orphaned packages (packages that were installed as dependencies but are no longer required)
- Select individual packages or all at once

### System Logs Management
- View, compress, or remove system log files
- Filter logs by age (days, weeks, months)
- Batch operations on multiple log files

### System Services
- View all system services with detailed information
- Start, stop, enable, or disable services
- Filter services by name

### Disk Usage Analysis
- Analyze disk usage across partitions
- Browse directories and view detailed space usage
- Find large files that may be consuming significant space
- Apply filters to find specific file types

## Requirements
- Qt 5/6
- zenity (for file dialogs when running as root)
- Arch Linux or other Pacman-based distribution

## Building the Application

### Install Dependencies
```
sudo pacman -S qt5-base qt6-base polkit zenity
```

### Build the Application
```
mkdir build
cd build
cmake ..
make
```

## Usage
Run the application using the provided launcher script:
```
./launch_cache_cleaner.sh
```

The launcher will automatically run the application with the necessary privileges.

## Note
This application requires administrative privileges for most operations. It uses sudo to run with elevated permissions. 
