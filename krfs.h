#ifndef __KRFS_H__
#define __KRFS_H__

/* krfs.h defines symbols to work in kernel space */

#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "rfs.h"

/* Declare operations to be hooked to VFS */

extern struct file_system_type rfs_fs_type;
extern const struct super_operations rfs_sb_ops;
extern const struct inode_operations rfs_inode_ops;
extern const struct file_operations rfs_dir_operations;
extern const struct file_operations rfs_file_operations;

struct dentry *rfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name,
                         void *data);
void rfs_kill_superblock(struct super_block *sb);

void rfs_destroy_inode(struct inode *inode);
void rfs_put_super(struct super_block *sb);

int rfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
struct dentry *rfs_lookup(struct inode *parent_inode, struct dentry *child_dentry,
                          unsigned int flags);
int rfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#error fs api changed in linux 3.11.0. Please use a better kernel to build my code!
#endif
int rfs_iterate(struct file *filp, struct dir_context *ctx);

ssize_t rfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);
ssize_t rfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);

extern struct kmem_cache *rfs_inode_cache;

/* Helper functions */

// To translate VFS superblock to rfs superblock
static inline struct rfs_superblock *RFS_SB(struct super_block *sb) { return sb->s_fs_info; }
static inline struct rfs_inode *RFS_INODE(struct inode *inode) { return inode->i_private; }

static inline uint64_t RFS_INODES_PER_BLOCK(struct super_block *sb) {
    struct rfs_superblock *rfs_sb;
    rfs_sb = RFS_SB(sb);
    return RFS_INODES_PER_BLOCK_HSB(rfs_sb);
}

// Given the inode_no, calcuate which block in inode table contains the corresponding inode
static inline uint64_t RFS_INODE_BLOCK_OFFSET(struct super_block *sb, uint64_t inode_no) {
    return inode_no / RFS_INODES_PER_BLOCK_HSB(RFS_SB(sb));
}
static inline uint64_t RFS_INODE_BYTE_OFFSET(struct super_block *sb, uint64_t inode_no) {
    return (inode_no % RFS_INODES_PER_BLOCK_HSB(RFS_SB(sb))) * sizeof(struct rfs_inode);
}

static inline uint64_t RFS_DIR_MAX_RECORD(struct super_block *sb) {
    return RFS_SB(sb)->blocksize / sizeof(struct rfs_dir_record);
}

// From which block does data blocks start
static inline uint64_t RFS_DATA_BLOCK_TABLE_START_BLOCK_NO(struct super_block *sb) {
    return RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(RFS_SB(sb));
}

void rfs_save_sb(struct super_block *sb);

// functions to operate inode
void rfs_fill_inode(struct super_block *sb, struct inode *inode, struct rfs_inode *rfs_inode);
int rfs_alloc_rfs_inode(struct super_block *sb, uint64_t *out_inode_no);
struct rfs_inode *rfs_get_rfs_inode(struct super_block *sb, uint64_t inode_no);
void rfs_save_rfs_inode(struct super_block *sb, struct rfs_inode *inode);
int rfs_add_dir_record(struct super_block *sb, struct inode *dir, struct dentry *dentry,
                       struct inode *inode);
int rfs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no);
int rfs_create_inode(struct inode *dir, struct dentry *dentry, umode_t mode);

#endif /*__KRFS_H__*/
