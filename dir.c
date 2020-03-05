#include "krfs.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#error fs api changed in linux 3.11.0. Please use a better kernel to build my code!
#endif

int rfs_iterate(struct file *filp, struct dir_context *ctx) {
    loff_t pos;
    struct inode *inode;
    struct super_block *sb;
    struct buffer_head *bh;
    struct rfs_inode *rfs_inode;
    struct rfs_dir_record *dir_record;
    uint64_t i;
    RLIB_KTRACE_FUNC(iterate);

    pos = ctx->pos;
    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    rfs_inode = RFS_INODE(inode);

    if (pos) {
        // TODO @Sankar: we use a hack of reading pos to figure if we have filled in data.
        return 0;
    }

    printk(KERN_INFO "readdir: rfs_inode->inode_no=%llu", rfs_inode->inode_no);

    if (unlikely(!S_ISDIR(rfs_inode->mode))) {
        printk(KERN_ERR
               "Inode %llu of dentry %s is not a directory\n",
               rfs_inode->inode_no,
               filp->f_path.dentry->d_name.name);
        return -ENOTDIR;
    }

    bh = sb_bread(sb, rfs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct rfs_dir_record *)bh->b_data;
    for (i = 0; i < rfs_inode->dir_children_count; i++) {
        dir_emit(ctx, dir_record->filename, RFS_FILENAME_MAXLEN, dir_record->inode_no, DT_UNKNOWN);
        ctx->pos += sizeof(struct rfs_dir_record);
        pos += sizeof(struct rfs_dir_record);
        dir_record++;
    }
    brelse(bh);

    return 0;
}
