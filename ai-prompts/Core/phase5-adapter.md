# Adapters

Use incremental logic builder to update the cpp code and code reviewer for updating the cmakes. Do not run any unit tests at this time. Focus on getting the existing adapters compiling.

We need to refactor the old adapter code to the new CTE apis. I want you to start with hermes_adapters/filesystem and hermes_adapter/posix. You can ignore the Append operations for writes at this time. We will come back to append later. In addition, you can remove the code regarding building file parameters with hermes::BinaryFileStager::BuildFileParams.

Bucket apis (e.g., hermes::Bucket) are analagous to tag apis. If the bucket API used doesn't seem to match any existing api, then comment it out and document the reason. hermes::Bucket is like a wrp::cte::Core client.

hermes::Blob is similar to CHI_IPC->AllocateBuffer.

