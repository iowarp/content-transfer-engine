@CLAUDE.md 

Implement a query api for iowarp. Read @docs/chimaera/module_dev_guide.md to see how to edit chimods.

Add both APIs to the python bindings under wrapper/python/core_bindings.cpp.

Ensure everything compiles. 

Add tests for this api. add them to a new file named test/unit/test_query.cc.

# Tag Query

Create a new chimod method named kTagQuery. Implement the task and associated methods.

Add the following method to wrp_cte::core::ContentTransferEngine:
```
std::vector<std::string> TagQuery(const std::string &tag_re, const PoolQuery &pool_query = PoolQuery::kBroadcast)
```

## core_runtime.cc

Iterate over the tag table and find the set of tags matching this query. store in a std::vector.
Then copy the vectory using copy assignment to the task's hipc::vector.

# Blob Query
Create a new chimod method named kBlobQuery. Implement the task and associated methods.

Query the set of blobs using a regex query. Return the set of
blob names that have tags matching the regex.

Add the following method to wrp_cte::core::ContentTransferEngine:
```
std::vector<std::string> BlobQuery(const std::string &tag_re, const std::string &blob_re, const PoolQuery &pool_query = PoolQuery::kBroadcast)
```

## core_runtime.cc

Iterate over the tag table and check if tag matches regex.
Add to an unordered_set<TagInfo*>.
Then iterate over the blob table.
If any blob name matches the regex, add it to a std::vector.
After loop iterates over both tables, copy the vectory using copy assignment to the task's hipc::vector.
