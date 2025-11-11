@CLAUDE.md use incremental logic builder. BlobId should be a typedef of chi::UniqueId, which is a  struct with u32 major_ and minor_. You should use the node_id_ from IPC Manager as the major. Store  a unique integer counter atomic number in the Container class to create the unique number for the  minor. Only create a blob id if its name is non-null and the blob did not already exist.

PutBlob should be able to locate a blob by either name or blob id. If blob id is provided and is not null, then search by this. Otherwise, if name is provided and not null, then search by this. Otherwise, return with error code because name and blob id should not be null.

BlobId should never be created by the user. BlobId should be created internally by the Container.


@CLAUDE.md Remove CreateBdevForTarget. For PutBlob, do not do any additional verifications if the blob exists. You are also using the offset parameter wrong. The offset does not represent the location of the blob in the target. It represents the offset of data within the blob. To get a new offset of data in the the target, you need to use bdev_client's Allocate function. 

Again, the logic is as follows:
1. Check if the blob already exists. Create if it doesn't.
2. Find the parts of the blob that should be modified. The blob should have a vector of Blocks. Each block should include the bdev client, offset, and size of the block. The block vector is in order. So block 0 represents the first size bytes of the blob. If we modify offset 1024 in a blob, for example, we need to find the first target that contains this offset by iterating over this vector.
3. Write the modifications using async tasks using target client api. Use async tasks and check their completion later.
4. Use a data placement engine (DPE) to determine the best target to place new data. The cte configuration should specify the DPE as a string. We should add a string parser to convert a dpe name string to enum.
5. Allocate space from the chosen target using bdev client. If the allocation function actually fails due to real-time contention for data placement, then change the remaining space for the target to 0 and then retry.
6. After blocks are allocated, place the data in those blocks using the bdev Write api.

@CLAUDE.md No, you just slightly change the function name. The algorithm should work like this:
```
ModifyExistingData(const std::vector<Block> &blocks, hipc::Pointer data, size_t data_size, size_t data_offset_in_blob):
1. Initially store the remaining_size equal to data_size. We iterate over every block in the blob.
2. Store the offset of the block in the blob. The first block is offset 0. Call this block_offset_in_blob.
3. If the data we are writing is within the range [block_offset_in_blob, block_offset_in_blob + block.size), then we should modify this data. 
4. Clamp the range [data_offset_in_blob, data_offset_in_blob + data_size) to the range [block_offset_in_blob, block_offset_in_blob + block.size). data_offset_in_blob must be no lower than block_offset_in_blob. data_offset_in_blob + data_size must be no larger than block_offset_in_blob + block.size.
5. Perform async write on the updated range.
6. Subtract the amount of data we have written from the remaining_size
7. If remaining size is 0, quit the for loop. Wait for all Async write operations to complete.
```