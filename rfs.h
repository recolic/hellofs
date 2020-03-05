#ifndef __RFS_H__
#define __RFS_H__

#define BITS_IN_BYTE 8
#define RFS_MAGIC 0x20160105
#define RFS_DEFAULT_BLOCKSIZE 4096
#define RFS_DEFAULT_INODE_TABLE_SIZE 1024
#define RFS_DEFAULT_DATA_BLOCK_TABLE_SIZE 1024
#define RFS_FILENAME_MAXLEN 255

#include "rlib/macro.hpp"

/* Define filesystem structures */

extern struct mutex rfs_sb_lock;

struct rfs_dir_record {
    char filename[RFS_FILENAME_MAXLEN];
    uint64_t inode_no;
};

struct rfs_inode {
    mode_t mode;
    uint64_t inode_no;
    uint64_t data_block_no;

    // TODO struct timespec is defined kenrel space,
    // but mkfs-rfs.c is compiled in user space
    /*struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;*/

    union {
        uint64_t file_size;
        uint64_t dir_children_count;
    };
};

struct rfs_superblock {
    uint64_t version;
    uint64_t magic;
    uint64_t blocksize;

    uint64_t inode_table_size;
    uint64_t inode_count;

    uint64_t data_block_table_size;
    uint64_t data_block_count;
};

static const uint64_t RFS_SUPERBLOCK_BLOCK_NO = 0;
static const uint64_t RFS_INODE_BITMAP_BLOCK_NO = 1;
static const uint64_t RFS_DATA_BLOCK_BITMAP_BLOCK_NO = 2;
static const uint64_t RFS_INODE_TABLE_START_BLOCK_NO = 3;

static const uint64_t RFS_ROOTDIR_INODE_NO = 0;
// data block no is the absolute block number from start of device
// data block no offset is the relative block offset from start of data block table
static const uint64_t RFS_ROOTDIR_DATA_BLOCK_NO_OFFSET = 0;

/* Helper functions */

static inline uint64_t RFS_INODES_PER_BLOCK_HSB(
        struct rfs_superblock *rfs_sb) {
    return rfs_sb->blocksize / sizeof(struct rfs_inode);
}

static inline uint64_t RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(
        struct rfs_superblock *rfs_sb) {
    return RFS_INODE_TABLE_START_BLOCK_NO
           + rfs_sb->inode_table_size / RFS_INODES_PER_BLOCK_HSB(rfs_sb)
           + 1;
}

/* Debug function */
#define RLIB_KTRACE_FUNC(name) \
    printk(KERN_ALERT "Recolic ktrace: [" RLIB_MACRO_TO_CSTR(name) "] called.\n")

#endif /*__RFS_H__*/
