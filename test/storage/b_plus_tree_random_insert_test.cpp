#include <sys/types.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "gtest/gtest.h"
#include "storage/disk/disk_manager_memory.h"
#include "storage/index/b_plus_tree.h"
#include "test_util.h"  // NOLINT
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unordered_map>
using std::cout, std::endl;



namespace bustub {

using bustub::DiskManagerUnlimitedMemory;

}  // namespace bustub