@CLAUDE.md Do not use static variables in the runtime. No single target lock or configuration. No single tag lock. In fact, we should have a set of locks instead. Let's say the maximum number of locks equals the maximum number of lanes.

@CLAUDE.md Do not generate a blob name automatically. PutBlob will get or create the blob. Both the name  and id should not be null. If the blob is new, the name is required. If the blob did not exist and the name is null, you should error. Do not automatically produce names

@CLAUDE.md You need to read the docs. Check @docs/chiamera/bdev.md

@CLAUDE.md Why are you parameterizing perf_metrics yourself! Call the bdev stat method instead! Target_info should just store a PerfMetrics data structure internally, do not repeat its parameters.