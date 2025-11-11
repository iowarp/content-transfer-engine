@CLAUDE.md Implement PutBlob and data placement algorithms

# Target Score

The target score should be a number between 0 and 1. Let's use normalized log bandwidth. So, the score for target i would be ``log(bandwidth_i) / log(bandwidth_MAX)``. We should add the target score to the target info. This score should be auto-calculated. 

# Data placement

Takes as input a vector of targets where data could be placed and the score of the blob. Outputs a single target where the blob should be placed. The Data Placement engine should be a factory. We should have an enum for representing the different engines available.

## Random Placement

1. Randomly choose a target to place data
2. Check if the target theoretically has space
3. If it does, then return that target.
4. Otherwise, go to next target. Keep repeating until space
5. If no space, than return a null target.

## Round-Robin Placement

1. Keep a static integer. 
2. Hash the integer to a target in the target vector.
3. If that target has space, return that target
4. Otherwise go to next target. Keep repeating until space.
5. If no space, return a null target

## MaxBW Placement

1. Sort the targets by bandwidth if the I/O is >= 32KB, otherwise sort by latency.
2. Find the first target with space that has a score lower than ours.

# PutBlob

1. Check if the blob already exists. Create if it doesn't.
2. Find the parts of the blob that should be modified. The blob should have a vector of Blocks. Each block should include the bdev client, offset, and size of the block. The block vector is in order. So block 0 represents the first size bytes of the blob. If we modify offset 1024 in a blob, for example, we need to find the first target that contains this offset by iterating over this vector.
3. Write the modifications using async tasks using target client api. Use async tasks and check their completion later.
4. Use a data placement engine (DPE) to determine the best target to place new data. The cte configuration should specify the DPE as a string. We should add a string parser to convert a dpe name string to enum.
5. Allocate space from the chosen target using bdev client. If the allocation function actually fails due to real-time contention for data placement, then change the remaining space for the target to 0 and then retry.
6. After blocks are allocated, place the data in those blocks using the bdev Write api.

# GetBlob

Similar to PutBlob, but we do not perform data placement, allocation, or modification. 
1. Check if the blob name is non-empty and exists. If it does, then check if the ID exists. If it doesn't, error. 
2. Check if the blob id is non-null and exists. If it doesn't, error.
3. Use a for loop similar to ModifyExistingData. Except this time, instead of AsyncWrite, you do AsyncRead and wait for the reads to complete. 

Based on PutBlob
1. If the blob does not already exist, error
2. Get the blocks where data is located
3. Read the data into the shared-memory pointer apart of the task. Use async tasks to read multiple parts at the same time if there are multiple blocks.

