#pragma once
#include <stdint.h>

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
  short type;  // Type of file
  uint64_t dev;     // File system's disk device
  uint64_t ino;    // Inode number
  short nlink; // Number of links to file
  uint64_t size;   // Size of file in bytes
};
