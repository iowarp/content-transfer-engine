@CLAUDE.md Implement PutBlob and data placement algorithms

# Target Score

The target score should be a number between 0 and 1. Let's use normalized log bandwidth. So, the score for target i would be ``log(bandwidth_i) / log(bandwidth_MAX)``.

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


