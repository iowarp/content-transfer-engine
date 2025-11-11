@CLAUDE.md Implement the following specification. Make sure to consider @docs/chimaera/admin.md, @docs/chimaera/bdev.md, and @docs/chimaera/MODULE_DEVELOPMENT_GUIDE.md

Focus on getting an initial version compiling and building a correct chimod. Make sure to use CMakePresets.json. In your root cmake, make sure to also load .env.cmake if it exists. Make it optional to do this using a cmake option boolean.

# Content Transfer Engine (CTE)

The cte is a system for placing data in tiered storage. This is implemented as a chimod. Build a chimod repo in this directory. It has the namespace wrp_cte. The chimod has the name core.

## Create

There is a YAML configuration file whose path can be passed to the CreateTask. This is the only parameter to the CreateTask. By default, if the path is null, the path will be set to the path pointed to by the environment variable WRP_RUNTIME_CONF.

In the runtime, we need to do the following:
1. Create targets on this node. 
2. Collect targets from neighboring nodes. 

## TARGET APIs

These apis will leverage chimaera's existing bdev chimod. It will use the chimaera bdev client API for creating the bdevs. This is a thin wrapper around that.

### RegisterTarget

Get or create a bdev on this node locally. Create a struct called Target, which contains the bdev client and the performance stats structure.

### UnregisterTarget

Unlink the bdev from this container. At this time, do not destroy the bdev container.

### ListTargets

Returns the set of registered targets on this node.

### StatTargets

Polls each target in the target client vector in a for loop. Typically this is a periodic operation. The StatTargets task has no inputs or outputs. It will simply update the internal target vector with the performance statistics.

## Tag APIs

A tag represents a grouping of blobs. A blob is simply an uninterpreted array of bytes. Each blob has a unique ID and semantic name. Names are expected to be unique within a tag. 

### GetOrCreateTag

The task should contain the following extra parameters:
1. the name of the tag (required, IN)
2. the unique ID of the tag (default none, INOUT)

In the container, we should have the following unordered_maps:
1. tag_name -> tag_id
2. tag_id -> TagInfo
3. tag_id.blob_name -> blob_id
4. blob_id -> BlobInfo

TagInfo and BlobInfo are classes. TagInfo stores the name and id of the tag, and the set of blob ids belonging to it. BlobInfo stores the id and name of the blob, the target and location within the target the blob is stored in. 

## Blob APIs

Blobs are uninterpreted arrays of bytes. Blobs are stored in targets. 

### PutBlob

Puts a blob in cte. For now, leave unimplemented.

Takes as input:
1. TagId (the tag the blob belongs to)
2. BlobName (the name of the blob in the tag, optional)
3. BlobId (the ID of the blob in the tag, optional, INOUT)
4. Blob offset (offset in the blob to write data)
5. Blob size (size of the data to write to the blob)
6. BlobData (a shared memory pointer to the blob data to write)
7. Score (the score of the data between 0 and 1)
8. flags (e.g., fire & forget, default empty)

### GetBlob

Get a blob from cte. For now, leave unimplemented.

Takes as input:
1. TagId (the tag the blob belongs to)
2. BlobName (the name of the blob in the tag, optional)
3. BlobId (the ID of the blob in the tag, optional, INOUT)
4. Blob offset (offset in the blob to write data)
5. Blob size (size of the data to write to the blob)
6. flags (e.g., fire & forget, default empty)

Has the following outputs:
1. BlobData (a shared memory pointer to the blob data to write)

## Buffer Reorganization APIs

### ReorganizeBlob

Changes the score of a blob. For now also leave unimplemented.