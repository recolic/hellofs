#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rfs.h"

int main(int argc, char *argv[]) {
    auto fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Error opening the device");
        return -1;
    }

    // construct superblock
    struct rfs_superblock rfs_sb = {
        .version = 1,
        .magic = RFS_MAGIC,
        .blocksize = RFS_DEFAULT_BLOCKSIZE,
        .inode_table_size = RFS_DEFAULT_INODE_TABLE_SIZE,
        .inode_count = 2,
        .data_block_table_size = RFS_DEFAULT_DATA_BLOCK_TABLE_SIZE,
        .data_block_count = 2,
    };

    // construct inode bitmap
    char inode_bitmap[rfs_sb.blocksize];
    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    inode_bitmap[0] = 1;

    // construct data block bitmap
    char data_block_bitmap[rfs_sb.blocksize];
    memset(data_block_bitmap, 0, sizeof(data_block_bitmap));
    data_block_bitmap[0] = 1;

    // construct root inode
    struct rfs_inode root_rfs_inode = {
        .mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH,
        .inode_no = RFS_ROOTDIR_INODE_NO,
        .data_block_no =
            RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) + RFS_ROOTDIR_DATA_BLOCK_NO_OFFSET,
        .dir_children_count = 1,
    };

    // construct welcome file inode
    char welcome_body[] = "Welcome Hellofs!!\n";
    auto welcome_inode_no = RFS_ROOTDIR_INODE_NO + 1;
    auto welcome_data_block_no_offset = RFS_ROOTDIR_DATA_BLOCK_NO_OFFSET + 1;
    struct rfs_inode welcome_rfs_inode = {
        .mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
        .inode_no = welcome_inode_no,
        .data_block_no =
            RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) + welcome_data_block_no_offset,
        .file_size = sizeof(welcome_body),
    };

    // construct root inode data block
    struct rfs_dir_record root_dir_records[] = {
        {
            .filename = "wel_helo.txt",
            .inode_no = welcome_inode_no,
        },
    };

    auto ret = -1;
    assert(sizeof(rfs_sb) <= rfs_sb.blocksize);
    // write super block
    if (sizeof(rfs_sb) != write(fd, &rfs_sb, sizeof(rfs_sb))) {
        goto err;
    }
    if ((off_t)-1 == lseek(fd, rfs_sb.blocksize, SEEK_SET)) {
        goto err;
    }

    // write inode bitmap
    if (sizeof(inode_bitmap) != write(fd, inode_bitmap, sizeof(inode_bitmap))) {
        goto err;
    }

    // write data block bitmap
    if (sizeof(data_block_bitmap) != write(fd, data_block_bitmap, sizeof(data_block_bitmap))) {
        goto err;
    }

    // write root inode
    if (sizeof(root_rfs_inode) != write(fd, &root_rfs_inode, sizeof(root_rfs_inode))) {
        goto err;
    }

    // write welcome file inode
    if (sizeof(welcome_rfs_inode) != write(fd, &welcome_rfs_inode, sizeof(welcome_rfs_inode))) {
        goto err;
    }

    printf("block size = %d, debug sizes=(sb)%d,(in_bitm)%d,(db_bitm)%d,(in_size)%d\n",
           rfs_sb.blocksize, sizeof(rfs_sb), sizeof(inode_bitmap), sizeof(data_block_bitmap),
           sizeof(root_rfs_inode));
    printf("Writing root inode data block at pos 0x%x\n",
           RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) * rfs_sb.blocksize);
    printf("welcome file data block starts at pos 0x%x\n",
           (RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) + 1) * rfs_sb.blocksize);
    // write root inode data block
    if ((off_t)-1 ==
        lseek(fd, RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) * rfs_sb.blocksize, SEEK_SET)) {
        goto err;
    }
    if (sizeof(root_dir_records) != write(fd, root_dir_records, sizeof(root_dir_records))) {
        goto err;
    }

    // write welcome file inode data block
    if ((off_t)-1 ==
        lseek(fd, (RFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&rfs_sb) + 1) * rfs_sb.blocksize,
              SEEK_SET)) {
        goto err;
    }
    if (sizeof(welcome_body) != write(fd, welcome_body, sizeof(welcome_body))) {
        goto err;
    }

    ret = 0; // success
err:
    close(fd);
    return ret;
}
