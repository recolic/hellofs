#!/usr/bin/fish

#test_dir="test-dir"
#test_mount_point="test-mount-point"

function hellofs_create_test_image
    dd bs=4096 count=6000 if=/dev/zero of=$argv[1]
    ../mkfs-hellofs $argv[1]
end

function hellofs_mount_fs_image
    mkdir $argv[2]
    sudo insmod ../hellofs.ko
    sudo mount -o loop,owner,group,users -t hellofs $argv[1] $argv[2]
end

function hellofs_unmount_fs
    sudo umount $argv[1]
    sudo rmmod ../hellofs.ko
end

#cleanup
#trap cleanup SIGINT EXIT
#mkdir "$test_dir" "$test_mount_point"
#create_test_image "$test_dir/image"
#
## run 1
#mount_fs_image "$test_dir/image" "$test_mount_point"
#do_some_operations "$test_mount_point"
#cd "$root_pwd"
#unmount_fs "$test_mount_point"
#
## run 2
#mount_fs_image "$test_dir/image" "$test_mount_point"
#do_read_operations "$test_mount_point"
#cd "$root_pwd"
#ls -lR "$test_mount_point"
#unmount_fs "$test_mount_point"
#
#echo "Test finished successfully!"
#cleanup
#
#make clean
#rm -rf "$test_dir/image" "$test_mount_point"
