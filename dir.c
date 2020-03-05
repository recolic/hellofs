#include "krfs.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#error fs api changed in linux 3.11.0. Please use a better kernel to build my code!
#endif

int rfs_iterate(struct file *filp, struct dir_context *ctx) {
    RLIB_KTRACE_FUNC(iterate);

    auto inode = filp->f_path.dentry->d_inode;
    auto sb = inode->i_sb;
    auto rfs_inode = RFS_INODE(inode);

    if (ctx->pos) {
        // TODO @Sankar: we use a hack of reading pos to figure if we have filled in data.
        printk(KERN_ALERT "iterate, pos != 0.\n");
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

    auto bh = sb_bread(sb, rfs_inode->data_block_no);
    BUG_ON(!bh);

    auto dir_record = (struct rfs_dir_record *)bh->b_data;
    for (auto i = 0; i < rfs_inode->dir_children_count; i++) {
        dir_emit(ctx, dir_record->filename, RFS_FILENAME_MAXLEN, dir_record->inode_no, DT_UNKNOWN);
        ctx->pos += sizeof(struct rfs_dir_record);
        dir_record++;
    }
    brelse(bh);

    return 0;
}
