@CLAUDE.md 

Implement the concept of neighborhoods. The neighborhood is the set of nodes the CTE is allowed to buffer to. This should be a new configuration parameter called neighbrohood_ (apart of performance). The default value is 4. Remove network category from CTE config. 

## Create (core_runtime.cc)

Instead of iterating over each storage device, we need to iterate over every storage device and 0 <= container_hash <= neighborhood. If the neighborhood size is larger than the number of nodes, we set the neighborhood size equal to the number of nodes. RegisterTarget should be called for each (storage, container_hash) combination. RegisterTarget should take as input a PoolQuery::DirectHash(container_hash), which will be the node to create the bdev on.

## RegisterTargetTask

RegisterTarget should take as input a new parameter called target_query, which should be the PoolQuery::DirectHash from the loop iteration in Create.  We need to store the PoolQuery in the TargetInfo as well so that other functions in the code can access it.

## RegisterTarget

Update calls to bdev to take as input a PoolQuery. The bdev API has changed to support this. Instead of using Dynamic for the PoolQuery, let's use the target_query.

## Other Bdev Calls

Ensure that every called to bdev APIs passes the target_query using the TargetInfo data structure. This mainly includes GetBlob and PutBlob.
