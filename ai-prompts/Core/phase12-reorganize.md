@CLAUDE.md We need to update ReorganizeBlob to be called ReorganizeBlobs. It should take as input a vector
of blob names (strings). We need to update the chimaera_mod.yaml, the method name, the task, and the runtime code to do this.

We also need to add a new chimod function called GetContainedBlobs. This will return a vector
of strings containing the names of the blobs that belong to a particular tag. 