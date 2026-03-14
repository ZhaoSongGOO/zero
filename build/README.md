# Build

This repository provides essential configuration files for C++ compilation and building. It enables you to leverage the gn&ninja build toolchain to build your own C++ projects efficiently.

## What's Included

The repository contains core scripts and configuration files to streamline C++ project setup with Gennija, including:

- Build configuration templates
- Toolchain setup scripts
- Environment configuration utilities

## Key Features

- Simplified initialization of Gennija-based C++ projects
- Pre-configured build settings for common C++ scenarios
- Easy-to-use scripts for managing build configurations
- Clean separation of build configuration from source code

## Script Documentation

### init.sh
- **Function**: Initializes Gennija build configuration for C++ projects
- **Purpose**: Generates a `.gn` file in the current directory pointing to the build configuration at `//build/BUILDCONFIG.gn`
- **Note**: Will not overwrite existing `.gn` files to prevent configuration loss

```bash
# Run initialization
./build/script/init.sh
```

### clean.sh
- **Function**: Cleans Gennija build configuration
- **Purpose**: Removes the `.gn` file from the current directory, allowing for fresh configuration

```bash
# Clean configuration
./build/script/clean.sh
```

## Usage Workflow

1. Set up your C++ project structure
2. Initialize Gennija configuration:
   ```bash
   ./build/script/init.sh
   ```
3. Build your project using Gennija commands
4. To reconfigure or start fresh:
   ```bash
   ./build/script/clean.sh
   ```

## Copyright Information
Copyright (c) zhaosonggo Authors. All rights reserved.