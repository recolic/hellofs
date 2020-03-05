#include "krfs.h"

ssize_t rfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos) {
    auto inode = filp->f_path.dentry->d_inode;
    auto sb = inode->i_sb;
    auto rfs_inode = RFS_INODE(inode);

    if (*ppos >= rfs_inode->file_size) {
        return 0;
    }

    auto bh = sb_bread(sb, rfs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n", rfs_inode->data_block_no);
        return 0;
    }

    auto buffer = (char *)bh->b_data + *ppos;
    auto nbytes = min((size_t)(rfs_inode->file_size - *ppos), len);

    if (copy_to_user(buf, buffer, nbytes)) {
        brelse(bh);
        printk(KERN_ERR "Error copying file content to userspace buffer\n");
        return -EFAULT;
    }

    brelse(bh);
    *ppos += nbytes;
    return nbytes;
}

/* TODO We didn't use address_space/pagecache here.
   If we hook file_operations.write = do_sync_write,
   and file_operations.aio_write = generic_file_aio_write,
   we will use write to pagecache instead. */
ssize_t rfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos) {
    auto inode = filp->f_path.dentry->d_inode;
    auto sb = inode->i_sb;
    auto rfs_inode = RFS_INODE(inode);

    // Recolic: compilation issue, temporary disable. TODO
    // ret = generic_write_checks(filp, ppos, &len, 0);
    // if (ret) {
    //     return ret;
    // }

    auto bh = sb_bread(sb, rfs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n", rfs_inode->data_block_no);
        return 0;
    }

    auto buffer = (char *)bh->b_data + *ppos;
    if (copy_from_user(buffer, buf, len)) {
        brelse(bh);
        printk(KERN_ERR "Error copying file content from userspace buffer "
                        "to kernel space\n");
        return -EFAULT;
    }
    *ppos += len;

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    rfs_inode->file_size = max((size_t)(rfs_inode->file_size), (size_t)(*ppos));
    rfs_save_rfs_inode(sb, rfs_inode);

    /* TODO We didn't update file size here. To be frank I don't know how. */

    return len;
}
