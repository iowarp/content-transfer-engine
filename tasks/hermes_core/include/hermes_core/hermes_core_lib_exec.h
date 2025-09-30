#ifndef CHI_HERMES_CORE_LIB_EXEC_H_
#define CHI_HERMES_CORE_LIB_EXEC_H_

/** Execute a task */
void Run(u32 method, Task *task, RunContext &rctx) override {
  switch (method) {
    case Method::kCreate: {
      Create(reinterpret_cast<CreateTask *>(task), rctx);
      break;
    }
    case Method::kDestroy: {
      Destroy(reinterpret_cast<DestroyTask *>(task), rctx);
      break;
    }
    case Method::kGetOrCreateTag: {
      GetOrCreateTag(reinterpret_cast<GetOrCreateTagTask *>(task), rctx);
      break;
    }
    case Method::kGetTagId: {
      GetTagId(reinterpret_cast<GetTagIdTask *>(task), rctx);
      break;
    }
    case Method::kGetTagName: {
      GetTagName(reinterpret_cast<GetTagNameTask *>(task), rctx);
      break;
    }
    case Method::kDestroyTag: {
      DestroyTag(reinterpret_cast<DestroyTagTask *>(task), rctx);
      break;
    }
    case Method::kTagAddBlob: {
      TagAddBlob(reinterpret_cast<TagAddBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagRemoveBlob: {
      TagRemoveBlob(reinterpret_cast<TagRemoveBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagClearBlobs: {
      TagClearBlobs(reinterpret_cast<TagClearBlobsTask *>(task), rctx);
      break;
    }
    case Method::kTagGetSize: {
      TagGetSize(reinterpret_cast<TagGetSizeTask *>(task), rctx);
      break;
    }
    case Method::kTagUpdateSize: {
      TagUpdateSize(reinterpret_cast<TagUpdateSizeTask *>(task), rctx);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      TagGetContainedBlobIds(reinterpret_cast<TagGetContainedBlobIdsTask *>(task), rctx);
      break;
    }
    case Method::kTagFlush: {
      TagFlush(reinterpret_cast<TagFlushTask *>(task), rctx);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      GetOrCreateBlobId(reinterpret_cast<GetOrCreateBlobIdTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobId: {
      GetBlobId(reinterpret_cast<GetBlobIdTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobName: {
      GetBlobName(reinterpret_cast<GetBlobNameTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobSize: {
      GetBlobSize(reinterpret_cast<GetBlobSizeTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobScore: {
      GetBlobScore(reinterpret_cast<GetBlobScoreTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobBuffers: {
      GetBlobBuffers(reinterpret_cast<GetBlobBuffersTask *>(task), rctx);
      break;
    }
    case Method::kPutBlob: {
      PutBlob(reinterpret_cast<PutBlobTask *>(task), rctx);
      break;
    }
    case Method::kGetBlob: {
      GetBlob(reinterpret_cast<GetBlobTask *>(task), rctx);
      break;
    }
    case Method::kAppendBlob: {
      AppendBlob(reinterpret_cast<AppendBlobTask *>(task), rctx);
      break;
    }
    case Method::kTruncateBlob: {
      TruncateBlob(reinterpret_cast<TruncateBlobTask *>(task), rctx);
      break;
    }
    case Method::kDestroyBlob: {
      DestroyBlob(reinterpret_cast<DestroyBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagBlob: {
      TagBlob(reinterpret_cast<TagBlobTask *>(task), rctx);
      break;
    }
    case Method::kBlobHasTag: {
      BlobHasTag(reinterpret_cast<BlobHasTagTask *>(task), rctx);
      break;
    }
    case Method::kReorganizeBlob: {
      ReorganizeBlob(reinterpret_cast<ReorganizeBlobTask *>(task), rctx);
      break;
    }
    case Method::kReorganizeNode: {
      ReorganizeNode(reinterpret_cast<ReorganizeNodeTask *>(task), rctx);
      break;
    }
    case Method::kFlushBlob: {
      FlushBlob(reinterpret_cast<FlushBlobTask *>(task), rctx);
      break;
    }
    case Method::kFlushData: {
      FlushData(reinterpret_cast<FlushDataTask *>(task), rctx);
      break;
    }
    case Method::kPollBlobMetadata: {
      PollBlobMetadata(reinterpret_cast<PollBlobMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollTargetMetadata: {
      PollTargetMetadata(reinterpret_cast<PollTargetMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollTagMetadata: {
      PollTagMetadata(reinterpret_cast<PollTagMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollAccessPattern: {
      PollAccessPattern(reinterpret_cast<PollAccessPatternTask *>(task), rctx);
      break;
    }
    case Method::kRegisterStager: {
      RegisterStager(reinterpret_cast<RegisterStagerTask *>(task), rctx);
      break;
    }
    case Method::kUnregisterStager: {
      UnregisterStager(reinterpret_cast<UnregisterStagerTask *>(task), rctx);
      break;
    }
    case Method::kStageIn: {
      StageIn(reinterpret_cast<StageInTask *>(task), rctx);
      break;
    }
    case Method::kStageOut: {
      StageOut(reinterpret_cast<StageOutTask *>(task), rctx);
      break;
    }
  }
}
/** Execute a task */
void Monitor(MonitorModeId mode, MethodId method, Task *task, RunContext &rctx) override {
  switch (method) {
    case Method::kCreate: {
      MonitorCreate(mode, reinterpret_cast<CreateTask *>(task), rctx);
      break;
    }
    case Method::kDestroy: {
      MonitorDestroy(mode, reinterpret_cast<DestroyTask *>(task), rctx);
      break;
    }
    case Method::kGetOrCreateTag: {
      MonitorGetOrCreateTag(mode, reinterpret_cast<GetOrCreateTagTask *>(task), rctx);
      break;
    }
    case Method::kGetTagId: {
      MonitorGetTagId(mode, reinterpret_cast<GetTagIdTask *>(task), rctx);
      break;
    }
    case Method::kGetTagName: {
      MonitorGetTagName(mode, reinterpret_cast<GetTagNameTask *>(task), rctx);
      break;
    }
    case Method::kDestroyTag: {
      MonitorDestroyTag(mode, reinterpret_cast<DestroyTagTask *>(task), rctx);
      break;
    }
    case Method::kTagAddBlob: {
      MonitorTagAddBlob(mode, reinterpret_cast<TagAddBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagRemoveBlob: {
      MonitorTagRemoveBlob(mode, reinterpret_cast<TagRemoveBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagClearBlobs: {
      MonitorTagClearBlobs(mode, reinterpret_cast<TagClearBlobsTask *>(task), rctx);
      break;
    }
    case Method::kTagGetSize: {
      MonitorTagGetSize(mode, reinterpret_cast<TagGetSizeTask *>(task), rctx);
      break;
    }
    case Method::kTagUpdateSize: {
      MonitorTagUpdateSize(mode, reinterpret_cast<TagUpdateSizeTask *>(task), rctx);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      MonitorTagGetContainedBlobIds(mode, reinterpret_cast<TagGetContainedBlobIdsTask *>(task), rctx);
      break;
    }
    case Method::kTagFlush: {
      MonitorTagFlush(mode, reinterpret_cast<TagFlushTask *>(task), rctx);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      MonitorGetOrCreateBlobId(mode, reinterpret_cast<GetOrCreateBlobIdTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobId: {
      MonitorGetBlobId(mode, reinterpret_cast<GetBlobIdTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobName: {
      MonitorGetBlobName(mode, reinterpret_cast<GetBlobNameTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobSize: {
      MonitorGetBlobSize(mode, reinterpret_cast<GetBlobSizeTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobScore: {
      MonitorGetBlobScore(mode, reinterpret_cast<GetBlobScoreTask *>(task), rctx);
      break;
    }
    case Method::kGetBlobBuffers: {
      MonitorGetBlobBuffers(mode, reinterpret_cast<GetBlobBuffersTask *>(task), rctx);
      break;
    }
    case Method::kPutBlob: {
      MonitorPutBlob(mode, reinterpret_cast<PutBlobTask *>(task), rctx);
      break;
    }
    case Method::kGetBlob: {
      MonitorGetBlob(mode, reinterpret_cast<GetBlobTask *>(task), rctx);
      break;
    }
    case Method::kAppendBlob: {
      MonitorAppendBlob(mode, reinterpret_cast<AppendBlobTask *>(task), rctx);
      break;
    }
    case Method::kTruncateBlob: {
      MonitorTruncateBlob(mode, reinterpret_cast<TruncateBlobTask *>(task), rctx);
      break;
    }
    case Method::kDestroyBlob: {
      MonitorDestroyBlob(mode, reinterpret_cast<DestroyBlobTask *>(task), rctx);
      break;
    }
    case Method::kTagBlob: {
      MonitorTagBlob(mode, reinterpret_cast<TagBlobTask *>(task), rctx);
      break;
    }
    case Method::kBlobHasTag: {
      MonitorBlobHasTag(mode, reinterpret_cast<BlobHasTagTask *>(task), rctx);
      break;
    }
    case Method::kReorganizeBlob: {
      MonitorReorganizeBlob(mode, reinterpret_cast<ReorganizeBlobTask *>(task), rctx);
      break;
    }
    case Method::kReorganizeNode: {
      MonitorReorganizeNode(mode, reinterpret_cast<ReorganizeNodeTask *>(task), rctx);
      break;
    }
    case Method::kFlushBlob: {
      MonitorFlushBlob(mode, reinterpret_cast<FlushBlobTask *>(task), rctx);
      break;
    }
    case Method::kFlushData: {
      MonitorFlushData(mode, reinterpret_cast<FlushDataTask *>(task), rctx);
      break;
    }
    case Method::kPollBlobMetadata: {
      MonitorPollBlobMetadata(mode, reinterpret_cast<PollBlobMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollTargetMetadata: {
      MonitorPollTargetMetadata(mode, reinterpret_cast<PollTargetMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollTagMetadata: {
      MonitorPollTagMetadata(mode, reinterpret_cast<PollTagMetadataTask *>(task), rctx);
      break;
    }
    case Method::kPollAccessPattern: {
      MonitorPollAccessPattern(mode, reinterpret_cast<PollAccessPatternTask *>(task), rctx);
      break;
    }
    case Method::kRegisterStager: {
      MonitorRegisterStager(mode, reinterpret_cast<RegisterStagerTask *>(task), rctx);
      break;
    }
    case Method::kUnregisterStager: {
      MonitorUnregisterStager(mode, reinterpret_cast<UnregisterStagerTask *>(task), rctx);
      break;
    }
    case Method::kStageIn: {
      MonitorStageIn(mode, reinterpret_cast<StageInTask *>(task), rctx);
      break;
    }
    case Method::kStageOut: {
      MonitorStageOut(mode, reinterpret_cast<StageOutTask *>(task), rctx);
      break;
    }
  }
}
/** Delete a task */
void Del(const hipc::MemContext &mctx, u32 method, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      CHI_CLIENT->DelTask<CreateTask>(mctx, reinterpret_cast<CreateTask *>(task));
      break;
    }
    case Method::kDestroy: {
      CHI_CLIENT->DelTask<DestroyTask>(mctx, reinterpret_cast<DestroyTask *>(task));
      break;
    }
    case Method::kGetOrCreateTag: {
      CHI_CLIENT->DelTask<GetOrCreateTagTask>(mctx, reinterpret_cast<GetOrCreateTagTask *>(task));
      break;
    }
    case Method::kGetTagId: {
      CHI_CLIENT->DelTask<GetTagIdTask>(mctx, reinterpret_cast<GetTagIdTask *>(task));
      break;
    }
    case Method::kGetTagName: {
      CHI_CLIENT->DelTask<GetTagNameTask>(mctx, reinterpret_cast<GetTagNameTask *>(task));
      break;
    }
    case Method::kDestroyTag: {
      CHI_CLIENT->DelTask<DestroyTagTask>(mctx, reinterpret_cast<DestroyTagTask *>(task));
      break;
    }
    case Method::kTagAddBlob: {
      CHI_CLIENT->DelTask<TagAddBlobTask>(mctx, reinterpret_cast<TagAddBlobTask *>(task));
      break;
    }
    case Method::kTagRemoveBlob: {
      CHI_CLIENT->DelTask<TagRemoveBlobTask>(mctx, reinterpret_cast<TagRemoveBlobTask *>(task));
      break;
    }
    case Method::kTagClearBlobs: {
      CHI_CLIENT->DelTask<TagClearBlobsTask>(mctx, reinterpret_cast<TagClearBlobsTask *>(task));
      break;
    }
    case Method::kTagGetSize: {
      CHI_CLIENT->DelTask<TagGetSizeTask>(mctx, reinterpret_cast<TagGetSizeTask *>(task));
      break;
    }
    case Method::kTagUpdateSize: {
      CHI_CLIENT->DelTask<TagUpdateSizeTask>(mctx, reinterpret_cast<TagUpdateSizeTask *>(task));
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      CHI_CLIENT->DelTask<TagGetContainedBlobIdsTask>(mctx, reinterpret_cast<TagGetContainedBlobIdsTask *>(task));
      break;
    }
    case Method::kTagFlush: {
      CHI_CLIENT->DelTask<TagFlushTask>(mctx, reinterpret_cast<TagFlushTask *>(task));
      break;
    }
    case Method::kGetOrCreateBlobId: {
      CHI_CLIENT->DelTask<GetOrCreateBlobIdTask>(mctx, reinterpret_cast<GetOrCreateBlobIdTask *>(task));
      break;
    }
    case Method::kGetBlobId: {
      CHI_CLIENT->DelTask<GetBlobIdTask>(mctx, reinterpret_cast<GetBlobIdTask *>(task));
      break;
    }
    case Method::kGetBlobName: {
      CHI_CLIENT->DelTask<GetBlobNameTask>(mctx, reinterpret_cast<GetBlobNameTask *>(task));
      break;
    }
    case Method::kGetBlobSize: {
      CHI_CLIENT->DelTask<GetBlobSizeTask>(mctx, reinterpret_cast<GetBlobSizeTask *>(task));
      break;
    }
    case Method::kGetBlobScore: {
      CHI_CLIENT->DelTask<GetBlobScoreTask>(mctx, reinterpret_cast<GetBlobScoreTask *>(task));
      break;
    }
    case Method::kGetBlobBuffers: {
      CHI_CLIENT->DelTask<GetBlobBuffersTask>(mctx, reinterpret_cast<GetBlobBuffersTask *>(task));
      break;
    }
    case Method::kPutBlob: {
      CHI_CLIENT->DelTask<PutBlobTask>(mctx, reinterpret_cast<PutBlobTask *>(task));
      break;
    }
    case Method::kGetBlob: {
      CHI_CLIENT->DelTask<GetBlobTask>(mctx, reinterpret_cast<GetBlobTask *>(task));
      break;
    }
    case Method::kAppendBlob: {
      CHI_CLIENT->DelTask<AppendBlobTask>(mctx, reinterpret_cast<AppendBlobTask *>(task));
      break;
    }
    case Method::kTruncateBlob: {
      CHI_CLIENT->DelTask<TruncateBlobTask>(mctx, reinterpret_cast<TruncateBlobTask *>(task));
      break;
    }
    case Method::kDestroyBlob: {
      CHI_CLIENT->DelTask<DestroyBlobTask>(mctx, reinterpret_cast<DestroyBlobTask *>(task));
      break;
    }
    case Method::kTagBlob: {
      CHI_CLIENT->DelTask<TagBlobTask>(mctx, reinterpret_cast<TagBlobTask *>(task));
      break;
    }
    case Method::kBlobHasTag: {
      CHI_CLIENT->DelTask<BlobHasTagTask>(mctx, reinterpret_cast<BlobHasTagTask *>(task));
      break;
    }
    case Method::kReorganizeBlob: {
      CHI_CLIENT->DelTask<ReorganizeBlobTask>(mctx, reinterpret_cast<ReorganizeBlobTask *>(task));
      break;
    }
    case Method::kReorganizeNode: {
      CHI_CLIENT->DelTask<ReorganizeNodeTask>(mctx, reinterpret_cast<ReorganizeNodeTask *>(task));
      break;
    }
    case Method::kFlushBlob: {
      CHI_CLIENT->DelTask<FlushBlobTask>(mctx, reinterpret_cast<FlushBlobTask *>(task));
      break;
    }
    case Method::kFlushData: {
      CHI_CLIENT->DelTask<FlushDataTask>(mctx, reinterpret_cast<FlushDataTask *>(task));
      break;
    }
    case Method::kPollBlobMetadata: {
      CHI_CLIENT->DelTask<PollBlobMetadataTask>(mctx, reinterpret_cast<PollBlobMetadataTask *>(task));
      break;
    }
    case Method::kPollTargetMetadata: {
      CHI_CLIENT->DelTask<PollTargetMetadataTask>(mctx, reinterpret_cast<PollTargetMetadataTask *>(task));
      break;
    }
    case Method::kPollTagMetadata: {
      CHI_CLIENT->DelTask<PollTagMetadataTask>(mctx, reinterpret_cast<PollTagMetadataTask *>(task));
      break;
    }
    case Method::kPollAccessPattern: {
      CHI_CLIENT->DelTask<PollAccessPatternTask>(mctx, reinterpret_cast<PollAccessPatternTask *>(task));
      break;
    }
    case Method::kRegisterStager: {
      CHI_CLIENT->DelTask<RegisterStagerTask>(mctx, reinterpret_cast<RegisterStagerTask *>(task));
      break;
    }
    case Method::kUnregisterStager: {
      CHI_CLIENT->DelTask<UnregisterStagerTask>(mctx, reinterpret_cast<UnregisterStagerTask *>(task));
      break;
    }
    case Method::kStageIn: {
      CHI_CLIENT->DelTask<StageInTask>(mctx, reinterpret_cast<StageInTask *>(task));
      break;
    }
    case Method::kStageOut: {
      CHI_CLIENT->DelTask<StageOutTask>(mctx, reinterpret_cast<StageOutTask *>(task));
      break;
    }
  }
}
/** Duplicate a task */
void CopyStart(u32 method, const Task *orig_task, Task *dup_task, bool deep) override {
  switch (method) {
    case Method::kCreate: {
      chi::CALL_COPY_START(
        reinterpret_cast<const CreateTask*>(orig_task), 
        reinterpret_cast<CreateTask*>(dup_task), deep);
      break;
    }
    case Method::kDestroy: {
      chi::CALL_COPY_START(
        reinterpret_cast<const DestroyTask*>(orig_task), 
        reinterpret_cast<DestroyTask*>(dup_task), deep);
      break;
    }
    case Method::kGetOrCreateTag: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetOrCreateTagTask*>(orig_task), 
        reinterpret_cast<GetOrCreateTagTask*>(dup_task), deep);
      break;
    }
    case Method::kGetTagId: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetTagIdTask*>(orig_task), 
        reinterpret_cast<GetTagIdTask*>(dup_task), deep);
      break;
    }
    case Method::kGetTagName: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetTagNameTask*>(orig_task), 
        reinterpret_cast<GetTagNameTask*>(dup_task), deep);
      break;
    }
    case Method::kDestroyTag: {
      chi::CALL_COPY_START(
        reinterpret_cast<const DestroyTagTask*>(orig_task), 
        reinterpret_cast<DestroyTagTask*>(dup_task), deep);
      break;
    }
    case Method::kTagAddBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagAddBlobTask*>(orig_task), 
        reinterpret_cast<TagAddBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kTagRemoveBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagRemoveBlobTask*>(orig_task), 
        reinterpret_cast<TagRemoveBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kTagClearBlobs: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagClearBlobsTask*>(orig_task), 
        reinterpret_cast<TagClearBlobsTask*>(dup_task), deep);
      break;
    }
    case Method::kTagGetSize: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagGetSizeTask*>(orig_task), 
        reinterpret_cast<TagGetSizeTask*>(dup_task), deep);
      break;
    }
    case Method::kTagUpdateSize: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagUpdateSizeTask*>(orig_task), 
        reinterpret_cast<TagUpdateSizeTask*>(dup_task), deep);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagGetContainedBlobIdsTask*>(orig_task), 
        reinterpret_cast<TagGetContainedBlobIdsTask*>(dup_task), deep);
      break;
    }
    case Method::kTagFlush: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagFlushTask*>(orig_task), 
        reinterpret_cast<TagFlushTask*>(dup_task), deep);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetOrCreateBlobIdTask*>(orig_task), 
        reinterpret_cast<GetOrCreateBlobIdTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlobId: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobIdTask*>(orig_task), 
        reinterpret_cast<GetBlobIdTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlobName: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobNameTask*>(orig_task), 
        reinterpret_cast<GetBlobNameTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlobSize: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobSizeTask*>(orig_task), 
        reinterpret_cast<GetBlobSizeTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlobScore: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobScoreTask*>(orig_task), 
        reinterpret_cast<GetBlobScoreTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlobBuffers: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobBuffersTask*>(orig_task), 
        reinterpret_cast<GetBlobBuffersTask*>(dup_task), deep);
      break;
    }
    case Method::kPutBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PutBlobTask*>(orig_task), 
        reinterpret_cast<PutBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kGetBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const GetBlobTask*>(orig_task), 
        reinterpret_cast<GetBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kAppendBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const AppendBlobTask*>(orig_task), 
        reinterpret_cast<AppendBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kTruncateBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TruncateBlobTask*>(orig_task), 
        reinterpret_cast<TruncateBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kDestroyBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const DestroyBlobTask*>(orig_task), 
        reinterpret_cast<DestroyBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kTagBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const TagBlobTask*>(orig_task), 
        reinterpret_cast<TagBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kBlobHasTag: {
      chi::CALL_COPY_START(
        reinterpret_cast<const BlobHasTagTask*>(orig_task), 
        reinterpret_cast<BlobHasTagTask*>(dup_task), deep);
      break;
    }
    case Method::kReorganizeBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const ReorganizeBlobTask*>(orig_task), 
        reinterpret_cast<ReorganizeBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kReorganizeNode: {
      chi::CALL_COPY_START(
        reinterpret_cast<const ReorganizeNodeTask*>(orig_task), 
        reinterpret_cast<ReorganizeNodeTask*>(dup_task), deep);
      break;
    }
    case Method::kFlushBlob: {
      chi::CALL_COPY_START(
        reinterpret_cast<const FlushBlobTask*>(orig_task), 
        reinterpret_cast<FlushBlobTask*>(dup_task), deep);
      break;
    }
    case Method::kFlushData: {
      chi::CALL_COPY_START(
        reinterpret_cast<const FlushDataTask*>(orig_task), 
        reinterpret_cast<FlushDataTask*>(dup_task), deep);
      break;
    }
    case Method::kPollBlobMetadata: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PollBlobMetadataTask*>(orig_task), 
        reinterpret_cast<PollBlobMetadataTask*>(dup_task), deep);
      break;
    }
    case Method::kPollTargetMetadata: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PollTargetMetadataTask*>(orig_task), 
        reinterpret_cast<PollTargetMetadataTask*>(dup_task), deep);
      break;
    }
    case Method::kPollTagMetadata: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PollTagMetadataTask*>(orig_task), 
        reinterpret_cast<PollTagMetadataTask*>(dup_task), deep);
      break;
    }
    case Method::kPollAccessPattern: {
      chi::CALL_COPY_START(
        reinterpret_cast<const PollAccessPatternTask*>(orig_task), 
        reinterpret_cast<PollAccessPatternTask*>(dup_task), deep);
      break;
    }
    case Method::kRegisterStager: {
      chi::CALL_COPY_START(
        reinterpret_cast<const RegisterStagerTask*>(orig_task), 
        reinterpret_cast<RegisterStagerTask*>(dup_task), deep);
      break;
    }
    case Method::kUnregisterStager: {
      chi::CALL_COPY_START(
        reinterpret_cast<const UnregisterStagerTask*>(orig_task), 
        reinterpret_cast<UnregisterStagerTask*>(dup_task), deep);
      break;
    }
    case Method::kStageIn: {
      chi::CALL_COPY_START(
        reinterpret_cast<const StageInTask*>(orig_task), 
        reinterpret_cast<StageInTask*>(dup_task), deep);
      break;
    }
    case Method::kStageOut: {
      chi::CALL_COPY_START(
        reinterpret_cast<const StageOutTask*>(orig_task), 
        reinterpret_cast<StageOutTask*>(dup_task), deep);
      break;
    }
  }
}
/** Duplicate a task */
void NewCopyStart(u32 method, const Task *orig_task, FullPtr<Task> &dup_task, bool deep) override {
  switch (method) {
    case Method::kCreate: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const CreateTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kDestroy: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const DestroyTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetOrCreateTag: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetOrCreateTagTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetTagId: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetTagIdTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetTagName: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetTagNameTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kDestroyTag: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const DestroyTagTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagAddBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagAddBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagRemoveBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagRemoveBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagClearBlobs: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagClearBlobsTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagGetSize: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagGetSizeTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagUpdateSize: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagUpdateSizeTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagGetContainedBlobIdsTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagFlush: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagFlushTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetOrCreateBlobIdTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlobId: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobIdTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlobName: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobNameTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlobSize: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobSizeTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlobScore: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobScoreTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlobBuffers: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobBuffersTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPutBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PutBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kGetBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const GetBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kAppendBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const AppendBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTruncateBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TruncateBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kDestroyBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const DestroyBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kTagBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const TagBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kBlobHasTag: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const BlobHasTagTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kReorganizeBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const ReorganizeBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kReorganizeNode: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const ReorganizeNodeTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kFlushBlob: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const FlushBlobTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kFlushData: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const FlushDataTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPollBlobMetadata: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PollBlobMetadataTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPollTargetMetadata: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PollTargetMetadataTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPollTagMetadata: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PollTagMetadataTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kPollAccessPattern: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const PollAccessPatternTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kRegisterStager: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const RegisterStagerTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kUnregisterStager: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const UnregisterStagerTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kStageIn: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const StageInTask*>(orig_task), dup_task, deep);
      break;
    }
    case Method::kStageOut: {
      chi::CALL_NEW_COPY_START(reinterpret_cast<const StageOutTask*>(orig_task), dup_task, deep);
      break;
    }
  }
}
/** Serialize a task when initially pushing into remote */
void SaveStart(
    u32 method, BinaryOutputArchive<true> &ar,
    Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar << *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar << *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar << *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar << *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar << *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar << *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar << *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar << *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar << *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kTagGetSize: {
      ar << *reinterpret_cast<TagGetSizeTask*>(task);
      break;
    }
    case Method::kTagUpdateSize: {
      ar << *reinterpret_cast<TagUpdateSizeTask*>(task);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      ar << *reinterpret_cast<TagGetContainedBlobIdsTask*>(task);
      break;
    }
    case Method::kTagFlush: {
      ar << *reinterpret_cast<TagFlushTask*>(task);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar << *reinterpret_cast<GetOrCreateBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobId: {
      ar << *reinterpret_cast<GetBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobName: {
      ar << *reinterpret_cast<GetBlobNameTask*>(task);
      break;
    }
    case Method::kGetBlobSize: {
      ar << *reinterpret_cast<GetBlobSizeTask*>(task);
      break;
    }
    case Method::kGetBlobScore: {
      ar << *reinterpret_cast<GetBlobScoreTask*>(task);
      break;
    }
    case Method::kGetBlobBuffers: {
      ar << *reinterpret_cast<GetBlobBuffersTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kGetBlob: {
      ar << *reinterpret_cast<GetBlobTask*>(task);
      break;
    }
    case Method::kAppendBlob: {
      ar << *reinterpret_cast<AppendBlobTask*>(task);
      break;
    }
    case Method::kTruncateBlob: {
      ar << *reinterpret_cast<TruncateBlobTask*>(task);
      break;
    }
    case Method::kDestroyBlob: {
      ar << *reinterpret_cast<DestroyBlobTask*>(task);
      break;
    }
    case Method::kTagBlob: {
      ar << *reinterpret_cast<TagBlobTask*>(task);
      break;
    }
    case Method::kBlobHasTag: {
      ar << *reinterpret_cast<BlobHasTagTask*>(task);
      break;
    }
    case Method::kReorganizeBlob: {
      ar << *reinterpret_cast<ReorganizeBlobTask*>(task);
      break;
    }
    case Method::kReorganizeNode: {
      ar << *reinterpret_cast<ReorganizeNodeTask*>(task);
      break;
    }
    case Method::kFlushBlob: {
      ar << *reinterpret_cast<FlushBlobTask*>(task);
      break;
    }
    case Method::kFlushData: {
      ar << *reinterpret_cast<FlushDataTask*>(task);
      break;
    }
    case Method::kPollBlobMetadata: {
      ar << *reinterpret_cast<PollBlobMetadataTask*>(task);
      break;
    }
    case Method::kPollTargetMetadata: {
      ar << *reinterpret_cast<PollTargetMetadataTask*>(task);
      break;
    }
    case Method::kPollTagMetadata: {
      ar << *reinterpret_cast<PollTagMetadataTask*>(task);
      break;
    }
    case Method::kPollAccessPattern: {
      ar << *reinterpret_cast<PollAccessPatternTask*>(task);
      break;
    }
    case Method::kRegisterStager: {
      ar << *reinterpret_cast<RegisterStagerTask*>(task);
      break;
    }
    case Method::kUnregisterStager: {
      ar << *reinterpret_cast<UnregisterStagerTask*>(task);
      break;
    }
    case Method::kStageIn: {
      ar << *reinterpret_cast<StageInTask*>(task);
      break;
    }
    case Method::kStageOut: {
      ar << *reinterpret_cast<StageOutTask*>(task);
      break;
    }
  }
}
/** Deserialize a task when popping from remote queue */
TaskPointer LoadStart(    u32 method, BinaryInputArchive<true> &ar) override {
  TaskPointer task_ptr;
  switch (method) {
    case Method::kCreate: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<CreateTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<CreateTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kDestroy: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<DestroyTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<DestroyTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetOrCreateTag: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetOrCreateTagTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetOrCreateTagTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetTagId: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetTagIdTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetTagIdTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetTagName: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetTagNameTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetTagNameTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kDestroyTag: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<DestroyTagTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<DestroyTagTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagAddBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagAddBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagAddBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagRemoveBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagRemoveBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagRemoveBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagClearBlobs: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagClearBlobsTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagClearBlobsTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagGetSize: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagGetSizeTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagGetSizeTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagUpdateSize: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagUpdateSizeTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagUpdateSizeTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagGetContainedBlobIdsTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagGetContainedBlobIdsTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagFlush: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagFlushTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagFlushTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetOrCreateBlobIdTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetOrCreateBlobIdTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlobId: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobIdTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobIdTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlobName: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobNameTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobNameTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlobSize: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobSizeTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobSizeTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlobScore: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobScoreTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobScoreTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlobBuffers: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobBuffersTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobBuffersTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPutBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PutBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PutBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kGetBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<GetBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<GetBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kAppendBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<AppendBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<AppendBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTruncateBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TruncateBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TruncateBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kDestroyBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<DestroyBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<DestroyBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kTagBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<TagBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<TagBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kBlobHasTag: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<BlobHasTagTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<BlobHasTagTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kReorganizeBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<ReorganizeBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<ReorganizeBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kReorganizeNode: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<ReorganizeNodeTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<ReorganizeNodeTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kFlushBlob: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<FlushBlobTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<FlushBlobTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kFlushData: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<FlushDataTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<FlushDataTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPollBlobMetadata: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PollBlobMetadataTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PollBlobMetadataTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPollTargetMetadata: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PollTargetMetadataTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PollTargetMetadataTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPollTagMetadata: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PollTagMetadataTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PollTagMetadataTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kPollAccessPattern: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<PollAccessPatternTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<PollAccessPatternTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kRegisterStager: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<RegisterStagerTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<RegisterStagerTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kUnregisterStager: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<UnregisterStagerTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<UnregisterStagerTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kStageIn: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<StageInTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<StageInTask*>(task_ptr.ptr_);
      break;
    }
    case Method::kStageOut: {
      task_ptr.ptr_ = CHI_CLIENT->NewEmptyTask<StageOutTask>(
             HSHM_DEFAULT_MEM_CTX, task_ptr.shm_);
      ar >> *reinterpret_cast<StageOutTask*>(task_ptr.ptr_);
      break;
    }
  }
  return task_ptr;
}
/** Serialize a task when returning from remote queue */
void SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar << *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar << *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar << *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar << *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar << *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar << *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar << *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar << *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar << *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kTagGetSize: {
      ar << *reinterpret_cast<TagGetSizeTask*>(task);
      break;
    }
    case Method::kTagUpdateSize: {
      ar << *reinterpret_cast<TagUpdateSizeTask*>(task);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      ar << *reinterpret_cast<TagGetContainedBlobIdsTask*>(task);
      break;
    }
    case Method::kTagFlush: {
      ar << *reinterpret_cast<TagFlushTask*>(task);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar << *reinterpret_cast<GetOrCreateBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobId: {
      ar << *reinterpret_cast<GetBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobName: {
      ar << *reinterpret_cast<GetBlobNameTask*>(task);
      break;
    }
    case Method::kGetBlobSize: {
      ar << *reinterpret_cast<GetBlobSizeTask*>(task);
      break;
    }
    case Method::kGetBlobScore: {
      ar << *reinterpret_cast<GetBlobScoreTask*>(task);
      break;
    }
    case Method::kGetBlobBuffers: {
      ar << *reinterpret_cast<GetBlobBuffersTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kGetBlob: {
      ar << *reinterpret_cast<GetBlobTask*>(task);
      break;
    }
    case Method::kAppendBlob: {
      ar << *reinterpret_cast<AppendBlobTask*>(task);
      break;
    }
    case Method::kTruncateBlob: {
      ar << *reinterpret_cast<TruncateBlobTask*>(task);
      break;
    }
    case Method::kDestroyBlob: {
      ar << *reinterpret_cast<DestroyBlobTask*>(task);
      break;
    }
    case Method::kTagBlob: {
      ar << *reinterpret_cast<TagBlobTask*>(task);
      break;
    }
    case Method::kBlobHasTag: {
      ar << *reinterpret_cast<BlobHasTagTask*>(task);
      break;
    }
    case Method::kReorganizeBlob: {
      ar << *reinterpret_cast<ReorganizeBlobTask*>(task);
      break;
    }
    case Method::kReorganizeNode: {
      ar << *reinterpret_cast<ReorganizeNodeTask*>(task);
      break;
    }
    case Method::kFlushBlob: {
      ar << *reinterpret_cast<FlushBlobTask*>(task);
      break;
    }
    case Method::kFlushData: {
      ar << *reinterpret_cast<FlushDataTask*>(task);
      break;
    }
    case Method::kPollBlobMetadata: {
      ar << *reinterpret_cast<PollBlobMetadataTask*>(task);
      break;
    }
    case Method::kPollTargetMetadata: {
      ar << *reinterpret_cast<PollTargetMetadataTask*>(task);
      break;
    }
    case Method::kPollTagMetadata: {
      ar << *reinterpret_cast<PollTagMetadataTask*>(task);
      break;
    }
    case Method::kPollAccessPattern: {
      ar << *reinterpret_cast<PollAccessPatternTask*>(task);
      break;
    }
    case Method::kRegisterStager: {
      ar << *reinterpret_cast<RegisterStagerTask*>(task);
      break;
    }
    case Method::kUnregisterStager: {
      ar << *reinterpret_cast<UnregisterStagerTask*>(task);
      break;
    }
    case Method::kStageIn: {
      ar << *reinterpret_cast<StageInTask*>(task);
      break;
    }
    case Method::kStageOut: {
      ar << *reinterpret_cast<StageOutTask*>(task);
      break;
    }
  }
}
/** Deserialize a task when popping from remote queue */
void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kCreate: {
      ar >> *reinterpret_cast<CreateTask*>(task);
      break;
    }
    case Method::kDestroy: {
      ar >> *reinterpret_cast<DestroyTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar >> *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar >> *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar >> *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar >> *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar >> *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar >> *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar >> *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kTagGetSize: {
      ar >> *reinterpret_cast<TagGetSizeTask*>(task);
      break;
    }
    case Method::kTagUpdateSize: {
      ar >> *reinterpret_cast<TagUpdateSizeTask*>(task);
      break;
    }
    case Method::kTagGetContainedBlobIds: {
      ar >> *reinterpret_cast<TagGetContainedBlobIdsTask*>(task);
      break;
    }
    case Method::kTagFlush: {
      ar >> *reinterpret_cast<TagFlushTask*>(task);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar >> *reinterpret_cast<GetOrCreateBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobId: {
      ar >> *reinterpret_cast<GetBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobName: {
      ar >> *reinterpret_cast<GetBlobNameTask*>(task);
      break;
    }
    case Method::kGetBlobSize: {
      ar >> *reinterpret_cast<GetBlobSizeTask*>(task);
      break;
    }
    case Method::kGetBlobScore: {
      ar >> *reinterpret_cast<GetBlobScoreTask*>(task);
      break;
    }
    case Method::kGetBlobBuffers: {
      ar >> *reinterpret_cast<GetBlobBuffersTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar >> *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kGetBlob: {
      ar >> *reinterpret_cast<GetBlobTask*>(task);
      break;
    }
    case Method::kAppendBlob: {
      ar >> *reinterpret_cast<AppendBlobTask*>(task);
      break;
    }
    case Method::kTruncateBlob: {
      ar >> *reinterpret_cast<TruncateBlobTask*>(task);
      break;
    }
    case Method::kDestroyBlob: {
      ar >> *reinterpret_cast<DestroyBlobTask*>(task);
      break;
    }
    case Method::kTagBlob: {
      ar >> *reinterpret_cast<TagBlobTask*>(task);
      break;
    }
    case Method::kBlobHasTag: {
      ar >> *reinterpret_cast<BlobHasTagTask*>(task);
      break;
    }
    case Method::kReorganizeBlob: {
      ar >> *reinterpret_cast<ReorganizeBlobTask*>(task);
      break;
    }
    case Method::kReorganizeNode: {
      ar >> *reinterpret_cast<ReorganizeNodeTask*>(task);
      break;
    }
    case Method::kFlushBlob: {
      ar >> *reinterpret_cast<FlushBlobTask*>(task);
      break;
    }
    case Method::kFlushData: {
      ar >> *reinterpret_cast<FlushDataTask*>(task);
      break;
    }
    case Method::kPollBlobMetadata: {
      ar >> *reinterpret_cast<PollBlobMetadataTask*>(task);
      break;
    }
    case Method::kPollTargetMetadata: {
      ar >> *reinterpret_cast<PollTargetMetadataTask*>(task);
      break;
    }
    case Method::kPollTagMetadata: {
      ar >> *reinterpret_cast<PollTagMetadataTask*>(task);
      break;
    }
    case Method::kPollAccessPattern: {
      ar >> *reinterpret_cast<PollAccessPatternTask*>(task);
      break;
    }
    case Method::kRegisterStager: {
      ar >> *reinterpret_cast<RegisterStagerTask*>(task);
      break;
    }
    case Method::kUnregisterStager: {
      ar >> *reinterpret_cast<UnregisterStagerTask*>(task);
      break;
    }
    case Method::kStageIn: {
      ar >> *reinterpret_cast<StageInTask*>(task);
      break;
    }
    case Method::kStageOut: {
      ar >> *reinterpret_cast<StageOutTask*>(task);
      break;
    }
  }
}

#endif  // CHI_HERMES_CORE_LIB_EXEC_H_