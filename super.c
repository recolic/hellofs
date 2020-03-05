#include "krfs.h"

static int rfs_fill_super(struct super_block *sb, void *data, int silent) {
    int ret = 0;

    auto bh = sb_bread(sb, RFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);
    auto rfs_sb = (struct rfs_superblock *)bh->b_data;
    if (unlikely(rfs_sb->magic != RFS_MAGIC)) {
        printk(KERN_ERR
               "Mount rfs filesystem: Wrong magic number in superblock: %llu != %llu\n",
               rfs_sb->magic, (uint64_t)RFS_MAGIC);
        goto release;
    }
    if (unlikely(sb->s_blocksize != rfs_sb->blocksize)) {
        printk(KERN_ERR
               "rfs seem to be formatted with mismatching blocksize: %lu\n",
               sb->s_blocksize);
        goto release;
    }

    sb->s_magic = rfs_sb->magic;
    sb->s_fs_info = rfs_sb;
    sb->s_maxbytes = rfs_sb->blocksize;
    sb->s_op = &rfs_sb_ops;

    auto root_rfs_inode = rfs_get_rfs_inode(sb, RFS_ROOTDIR_INODE_NO);
    auto root_inode = new_inode(sb);
    if (!root_inode || !root_rfs_inode) {
        ret = -ENOMEM;
        goto release;
    }
    rfs_fill_inode(sb, root_inode, root_rfs_inode);
    inode_init_owner(root_inode, NULL, root_inode->i_mode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }

release:
    brelse(bh);
    return ret;
}

struct dentry *rfs_mount(struct file_system_type *fs_type,
                             int flags, const char *dev_name,
                             void *data) {
    auto ret = mount_bdev(fs_type, flags, dev_name, data, rfs_fill_super);

    if (unlikely(IS_ERR(ret))) {
        printk(KERN_ERR "Error mounting rfs.\n");
    } else {
        printk(KERN_INFO "rfs is succesfully mounted on: %s\n",
               dev_name);
    }

    return ret;
}

void rfs_kill_superblock(struct super_block *sb) {
    printk(KERN_INFO
           "rfs superblock is destroyed. Unmount succesful.\n");
    kill_block_super(sb);
}

void rfs_put_super(struct super_block *sb) {
    return;
}

void rfs_save_sb(struct super_block *sb) {

    auto bh = sb_bread(sb, RFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);
    auto rfs_sb = RFS_SB(sb);

    bh->b_data = (char *)rfs_sb;
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}
