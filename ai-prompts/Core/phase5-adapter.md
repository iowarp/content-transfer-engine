# Adapters

Use incremental logic builder to update the cpp code and code reviewer for updating the cmakes. Do not run any unit tests at this time. Focus on getting the existing adapters compiling.

We need to refactor the old adapter code to the new CTE apis. I want you to start with hermes_adapters/filesystem and hermes_adapter/posix. You can ignore the Append operations for writes at this time. We will come back to append later. In addition, you can remove the code regarding building file parameters with hermes::BinaryFileStager::BuildFileParams.

Bucket apis (e.g., hermes::Bucket) are analagous to tag apis. If the bucket API used doesn't seem to match any existing api, then comment it out and document the reason. hermes::Bucket is like a wrp::cte::Core client.

hermes::Blob is similar to CHI_IPC->AllocateBuffer.

## Config
@CLAUDE.md Make a new configuration called the WRP_CAE_CONFIG. This configuration stores the set of paths that should be tracked for the adapters. It should be a YAML file with one entry called paths, where each path is a string representing something to scan. It should also have the adapter page size variable

## Splitting a blob

@CLAUDE.md The filesystem base class needs to divide blobs into fixed-size pages indicated by adapter page size. So a 16MB write needs to be split into 16 1MB writes if the page size is 1MB. The blobs should be named as the stringified index of the blob. So if we write to offset 0, the blob name would be 0 for the first 1MB. The next 1MB would be offset 1. So on and so forth.

