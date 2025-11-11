@CLAUDE.md We need to update ReorganizeBlob to be called ReorganizeBlobs. It should take as input a vector
of blob names (strings). We need to update the chimaera_mod.yaml, the method name, the task, and the runtime code to do this.

We also need to add a new chimod function called GetContainedBlobs. This will return a vector
of strings containing the names of the blobs that belong to a particular tag. 

ReorganizeBlobs should iterate over the Blob names and scores. It should do a controlled iteration
over the blobs and their scores, where at most 32 asynchronous operations are scheduled at a time.
```
1. Asynchronously get up to 32 blob scores.
1. Remove any blobs with negligibly different scores from consideration. Let's add this as a configuration parameter in the CTE_CONFIG. The default value should be .05.
1. Asynchronously get up to 32 blob sizes.
1. Wait
1. Allocate pointers and asynchronously get the blobs. Wait.
1. Allocate shared memory for the 32 blobs.
1. Asynchronously get 32 blobs. Wait.
1. Asynchronously put 32 blobs, but with the new score. Wait
1. Repeat until all blobs and scores have been set
```
