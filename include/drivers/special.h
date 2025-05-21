#pragma once

struct FileOps;

void give_null_fops(FileOps* fops);
void give_zero_fops(FileOps* fops);
void give_random_fops(FileOps* fops);