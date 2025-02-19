#pragma once

struct Filesystem;
struct Superblock;
struct Inode;
struct Device;

struct Mount
{
    Inode* mountpoint;
    Superblock* sb;

    static Mount* mount(Inode* target, Device* dev, Filesystem* fs);
    void unmount();

    static Mount* from_mountpoint(Inode* inode);
    static Mount* from_superblock(Superblock* sb);

    Inode* get_mountpoint();
};

struct MountTable
{

};