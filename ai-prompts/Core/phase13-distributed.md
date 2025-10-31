@CLAUDE.md I want to make this code leverage the PooolQuery::Dynamic() for all core methods. This will be used to implement distributed algorithms for data placement. Read @docs/chimaera/MODULE_DEVELOPMENT_GUIDE.md to see how to implement dynamic scheduling using the runtime context object and ExecMode.

# Target Operations

## kRegisterTarget: 10
This will update locally. If dynamic is used, just set the pool query to local.

## kUnregisterTarget: 11
This will update locally. If dynamic is used, just set the pool query to local.

## kListTargets: 12
This will update locally. If dynamic is used, just set the pool query to local.

## kStatTargets: 13
This will update locally. If dynamic is used, just set the pool query to local.





# Tag Operations

## kGetOrCreateTag: 14
If dynamic is used, resolve to local.

## kGetTagSize: 16
A broadcast operation. Dynamic will always resolve to PoolQuery::Bcast().
Ensure that the task implements an Aggregate method.
The aggregator should sum the sizes of the two tags.

## kGetContainedBlobs: 24
A broadcast operation. Dynamic will always resolve to PoolQuery::Bcast(). 
Ensures the task implements an Aggregate method.
The aggregator should merge the two blob vectors.



# Blob Operations

We should have a unified HashBlobToContainer function that performs: PoolQuery::GetDirectHash(hash(tag_id, blob_name)).
Most methods below should call this function instead of resolving manually.

## kPutBlob: 15 
Dynamic will always resolve to a PoolQuery::GetDirectHash(hash(tag_id, blob_name)).

## kGetBlob: 16    
If dynamic, always resolve to a PoolQuery::GetDirectHash(hash(tag_id, blob_name)).

## kReorganizeBlob: 17
If dynamic, always resolve to a PoolQuery::Local().
Update this function to do only a single blob instead of multiple blob reorganizations.

## kDelBlob: 18
If dynamic, set to a PoolQuery::GetDirectHash(hash(tag_id, blob_name)).

## kGetBlobScore: 22
If dynamic, set to a PoolQuery::GetDirectHash(hash(tag_id, blob_name)).

## kGetBlobSize: 23
If dynamic, set to a PoolQuery::GetDirectHash(hash(tag_id, blob_name)).
