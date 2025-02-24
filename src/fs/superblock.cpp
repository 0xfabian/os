#include <fs/superblock.h>

void Superblock::destroy()
{
    fs->destroy_sb(this);
    fs->num_sb--;
}