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
  u64 logical_time_;
}
```

Add logical_time_ as a member to CteTelemetry. Store an atomic counter in the runtime code representing the total number of telemetry entries generated. Every time we log a new entry the counter is incremented. 

Create a new chimod function called kPollTelemetryLog. Edit chimod.yaml and then call ``module load iowarp-runtime && chi_refresh_repo .`` It takes as input a minimum_logical_time_ and outputs the last logical_time_ scanned. The minimum time is used to filter the telemetry log to
prevent applications from collecting duplicate values.
