#include "krfs.h"

void rfs_destroy_inode(struct inode *inode) {
    auto rfs_inode = RFS_INODE(inode);
    printk(KERN_INFO "destroy_inode free private data of %p (%lu)\n", rfs_inode, inode->i_ino);
    kmem_cache_free(rfs_inode_cache, rfs_inode);
}

void rfs_fill_inode(struct super_block *sb, struct inode *inode, struct rfs_inode *rfs_inode) {
    inode->i_mode = rfs_inode->mode;
    inode->i_sb = sb;
    inode->i_ino = rfs_inode->inode_no;
    inode->i_op = &rfs_inode_ops;
    // TODO hope we can use rfs_inode to store timespec
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_private = rfs_inode;

    if (S_ISDIR(rfs_inode->mode)) {
        inode->i_fop = &rfs_dir_operations;
    } else if (S_ISREG(rfs_inode->mode)) {
        inode->i_fop = &rfs_file_operations;
    } else {
        printk(KERN_WARNING "Inode %lu is neither a directory nor a regular file", inode->i_ino);
        inode->i_fop = NULL;
    }

    /* TODO rfs_inode->file_size seems not reflected in inode */
}

/* TODO I didn't implement any function to dealloc rfs_inode */
int rfs_alloc_rfs_inode(struct super_block *sb, uint64_t *out_inode_no) {
    int ret = -ENOSPC;

    mutex_lock(&rfs_sb_lock);

    auto bh = sb_bread(sb, RFS_INODE_BITMAP_BLOCK_NO);
    BUG_ON(!bh);
    auto rfs_sb = RFS_SB(sb);

    auto bitmap = bh->b_data;
    for (auto i = 0; i < rfs_sb->inode_table_size; i++) {
        auto slot = bitmap + i / BITS_IN_BYTE;
        auto needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_inode_no = i;
            *slot |= needle;
            rfs_sb->inode_count += 1;
            ret = 0;
            break;
        }
    }
    // Booms if inode buffer is full.
    // locking critical section is too large, but Im too lazy
    //   to have it optimized.

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    rfs_save_sb(sb);

    mutex_unlock(&rfs_sb_lock);
    return ret;
}

struct rfs_inode *rfs_get_rfs_inode(struct super_block *sb, uint64_t inode_no) {
    struct buffer_head *bh;
    struct rfs_inode *inode;
    struct rfs_inode *inode_buf;

    bh = sb_bread(sb, RFS_INODE_TABLE_START_BLOCK_NO + RFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);

    inode = (struct rfs_inode *)(bh->b_data + RFS_INODE_BYTE_OFFSET(sb, inode_no));
    inode_buf = kmem_cache_alloc(rfs_inode_cache, GFP_KERNEL);
    memcpy(inode_buf, inode, sizeof(*inode_buf));

    brelse(bh);
    return inode_buf;
}

void rfs_save_rfs_inode(struct super_block *sb, struct rfs_inode *inode_buf) {
    auto inode_no = inode_buf->inode_no;
    auto bh = sb_bread(sb, RFS_INODE_TABLE_START_BLOCK_NO + RFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);

    auto inode = (struct rfs_inode *)(bh->b_data + RFS_INODE_BYTE_OFFSET(sb, inode_no));
    memcpy(inode, inode_buf, sizeof(*inode));

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}

int rfs_add_dir_record(struct super_block *sb, struct inode *dir, struct dentry *dentry,
                       struct inode *inode) {
    auto parent_rfs_inode = RFS_INODE(dir);
    if (unlikely(parent_rfs_inode->dir_children_count >= RFS_DIR_MAX_RECORD(sb))) {
        return -ENOSPC;
    }

    auto bh = sb_bread(sb, parent_rfs_inode->data_block_no);
    BUG_ON(!bh);

    auto dir_record = (struct rfs_dir_record *)bh->b_data;
    dir_record += parent_rfs_inode->dir_children_count;
    dir_record->inode_no = inode->i_ino;
    strcpy(dir_record->filename, dentry->d_name.name);

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    parent_rfs_inode->dir_children_count += 1;
    rfs_save_rfs_inode(sb, parent_rfs_inode);

    return 0;
}

int rfs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no) {
    int ret = -ENOSPC;

    auto rfs_sb = RFS_SB(sb);
    mutex_lock(&rfs_sb_lock);

    auto bh = sb_bread(sb, RFS_DATA_BLOCK_BITMAP_BLOCK_NO);
    BUG_ON(!bh);

    auto bitmap = bh->b_data;
    for (auto i = 0; i < rfs_sb->data_block_table_size; i++) {
        auto slot = bitmap + i / BITS_IN_BYTE;
        auto needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_data_block_no = RFS_DATA_BLOCK_TABLE_START_BLOCK_NO(sb) + i;
            *slot |= needle;
            rfs_sb->data_block_count += 1;
            ret = 0;
            break;
        }
    }

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    rfs_save_sb(sb);

    mutex_unlock(&rfs_sb_lock);
    return ret;
}

int rfs_create_inode(struct inode *dir, struct dentry *dentry, umode_t mode) {
    auto sb = dir->i_sb;
    auto rfs_sb = RFS_SB(sb);

    /* Create rfs_inode */
    uint64_t inode_no;
    auto ret = rfs_alloc_rfs_inode(sb, &inode_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk inode. "
                        "Is inode table full? "
                        "Inode count: %llu\n",
               rfs_sb->inode_count);
        return -ENOSPC;
    }
    auto rfs_inode = (struct rfs_inode *)kmem_cache_alloc(rfs_inode_cache, GFP_KERNEL);
    rfs_inode->inode_no = inode_no;
    rfs_inode->mode = mode;
    if (S_ISDIR(mode)) {
        rfs_inode->dir_children_count = 0;
    } else if (S_ISREG(mode)) {
        rfs_inode->file_size = 0;
    } else {
        printk(KERN_WARNING "Inode %llu is neither a directory nor a regular file", inode_no);
    }

    /* Allocate data block for the new rfs_inode */
    ret = rfs_alloc_data_block(sb, &rfs_inode->data_block_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk data block. "
                        "Is data block table full? "
                        "Data block count: %llu\n",
               rfs_sb->data_block_count);
        return -ENOSPC;
    }

    /* Create VFS inode */
    auto inode = new_inode(sb);
    if (!inode) {
        return -ENOMEM;
    }
    rfs_fill_inode(sb, inode, rfs_inode);

    /* Add new inode to parent dir */
    ret = rfs_add_dir_record(sb, dir, dentry, inode);
    if (0 != ret) {
        printk(KERN_ERR "Failed to add inode %lu to parent dir %lu\n", inode->i_ino, dir->i_ino);
        return -ENOSPC;
    }

    inode_init_owner(inode, dir, mode);
    d_add(dentry, inode);

    /* TODO we should free newly allocated inodes when error occurs */

    return 0;
}

int rfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl) {
    RLIB_KTRACE_FUNC(rfs_create);
    return rfs_create_inode(dir, dentry, mode);
}

int rfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode) {
    RLIB_KTRACE_FUNC(rfs_mkdir);
    /* @Sankar: The mkdir callback does not have S_IFDIR set.
       Even ext2 sets it explicitly. Perhaps this is a bug */
    mode |= S_IFDIR;
    return rfs_create_inode(dir, dentry, mode);
}

struct dentry *rfs_lookup(struct inode *dir, struct dentry *child_dentry, unsigned int flags) {
    RLIB_KTRACE_FUNC(rfs_lookup);

    auto parent_rfs_inode = RFS_INODE(dir);
    auto sb = dir->i_sb;
    auto bh = sb_bread(sb, parent_rfs_inode->data_block_no);
    BUG_ON(!bh);

    auto dir_record = (struct rfs_dir_record *)bh->b_data;

    for (auto i = 0; i < parent_rfs_inode->dir_children_count; i++) {
        printk(KERN_INFO "rfs_lookup: i=%d, dir_record->filename=%s, child_dentry->d_name.name=%s",
               i, dir_record->filename, child_dentry->d_name.name); // TODO
        if (0 == strcmp(dir_record->filename, child_dentry->d_name.name)) {
            auto rfs_child_inode = rfs_get_rfs_inode(sb, dir_record->inode_no);
            auto child_inode = new_inode(sb);
            if (!child_inode) {
                printk(KERN_ERR "Cannot create new inode. No memory.\n");
                return NULL;
            }
            rfs_fill_inode(sb, child_inode, rfs_child_inode);
            inode_init_owner(child_inode, dir, rfs_child_inode->mode);
            d_add(child_dentry, child_inode);
            return NULL;
        }
        dir_record++;
    }

    printk(KERN_ERR "No inode found for the filename: %s\n", child_dentry->d_name.name);
    return NULL;
}
