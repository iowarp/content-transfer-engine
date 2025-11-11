@CLAUDE.md I want to update tag operations.

# Tag Operations

## kGetOrCreateTag: 14
If dynamic is used, resolve to local if the tag exists locally.
Otherwise, spawn a copy of this task using DirectHash(tag_name). 
The task copy should be allocated using NewCopy() method from this container.
When the task returns, we will create a local TagId entry containing the task id.

## kGetTagSize: 16
A broadcast operation. Dynamic will always resolve to PoolQuery::Bcast().
Ensure that the task implements an Aggregate method.
The aggregator should sum the sizes of the two tags.

## kGetContainedBlobs: 24
A broadcast operation. Dynamic will always resolve to PoolQuery::Bcast(). 
Ensures the task implements an Aggregate method.
The aggregator should merge the two blob vectors.


