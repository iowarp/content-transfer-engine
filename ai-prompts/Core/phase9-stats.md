@CLAUDE.md 

We should add timestamps to the blob info and tag info for last modified and read time. The timestamps should be updated during GetBlob, PutBlob, GetOrCreateTag, GetTagSize.

We need to add a telemetry log. We should store a ring buffer containing information. Use hshm::circular_mpsc_queue for this. Create a new data structure that can store the parameters of GetBlob, PutBlob, DelBlob, GetOrCreateTag, and DelTag.

For PutBlob and GetBlob, the relevant information includes the id of the blob, the offset and size of the update within the blob, 
and the id of the tag the blob belongs to.

For DelBlob, only the id of the blob and the tag it belongs to matters.

The struct should look roughly as follows:
```
struct CteTelemetry {
  CteOp op_;  // e.g., PutBlob, GetBlob, etc.
  size_t off_;
  size_t size_;
  BlobId blob_id_;
  TagId tag_id_;
  Timestamp mod_time_;
  Timestamp read_time_;
}
```
