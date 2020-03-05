#include "krfs.h"

DEFINE_MUTEX(rfs_sb_lock);

struct file_system_type rfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "rfs",
    .mount = rfs_mount,
    .kill_sb = rfs_kill_superblock,
    .fs_flags = FS_REQUIRES_DEV,
};

const struct super_operations rfs_sb_ops = {
    .destroy_inode = rfs_destroy_inode,
    .put_super = rfs_put_super,
};

const struct inode_operations rfs_inode_ops = {
    .create = rfs_create,
    .mkdir = rfs_mkdir,
    .lookup = rfs_lookup,
};

const struct file_operations rfs_dir_operations = {
    .owner = THIS_MODULE,
    .iterate = rfs_iterate,
};

const struct file_operations rfs_file_operations = {
    .read = rfs_read,
    .write = rfs_write,
};

struct kmem_cache *rfs_inode_cache = NULL;

static int __init rfs_init(void)
{
    rfs_inode_cache = kmem_cache_create("rfs_inode_cache",
                                         sizeof(struct rfs_inode),
                                         0,
                                         (SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
                                         NULL);
    if (!rfs_inode_cache) {
        return -ENOMEM;
    }

    if (likely(register_filesystem(&rfs_fs_type) == 0)) {
        printk(KERN_INFO "Sucessfully registered rfs\n");
    } else {
        printk(KERN_ERR "Failed to register rfs. Error code: %d\n", ret);
    }

    return ret;
}

static void __exit rfs_exit(void)
{
    kmem_cache_destroy(rfs_inode_cache);

    if (likely(unregister_filesystem(&rfs_fs_type) == 0)) {
        printk(KERN_INFO "Sucessfully unregistered rfs\n");
    } else {
        printk(KERN_ERR "Failed to unregister rfs. Error code: %d\n",
               ret);
    }
}

module_init(rfs_init);
module_exit(rfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("accelazh");
