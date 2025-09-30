#ifndef CHI_HERMES_CORE_METHODS_H_
#define CHI_HERMES_CORE_METHODS_H_

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kGetOrCreateTag = 10;
  TASK_METHOD_T kGetTagId = 11;
  TASK_METHOD_T kGetTagName = 12;
  TASK_METHOD_T kDestroyTag = 14;
  TASK_METHOD_T kTagAddBlob = 15;
  TASK_METHOD_T kTagRemoveBlob = 16;
  TASK_METHOD_T kTagClearBlobs = 17;
  TASK_METHOD_T kTagGetSize = 18;
  TASK_METHOD_T kTagUpdateSize = 19;
  TASK_METHOD_T kTagGetContainedBlobIds = 20;
  TASK_METHOD_T kTagFlush = 21;
  TASK_METHOD_T kGetOrCreateBlobId = 30;
  TASK_METHOD_T kGetBlobId = 31;
  TASK_METHOD_T kGetBlobName = 32;
  TASK_METHOD_T kGetBlobSize = 34;
  TASK_METHOD_T kGetBlobScore = 35;
  TASK_METHOD_T kGetBlobBuffers = 36;
  TASK_METHOD_T kPutBlob = 37;
  TASK_METHOD_T kGetBlob = 38;
  TASK_METHOD_T kAppendBlob = 39;
  TASK_METHOD_T kTruncateBlob = 40;
  TASK_METHOD_T kDestroyBlob = 41;
  TASK_METHOD_T kTagBlob = 42;
  TASK_METHOD_T kBlobHasTag = 43;
  TASK_METHOD_T kReorganizeBlob = 44;
  TASK_METHOD_T kReorganizeNode = 45;
  TASK_METHOD_T kFlushBlob = 46;
  TASK_METHOD_T kFlushData = 47;
  TASK_METHOD_T kPollBlobMetadata = 50;
  TASK_METHOD_T kPollTargetMetadata = 51;
  TASK_METHOD_T kPollTagMetadata = 52;
  TASK_METHOD_T kPollAccessPattern = 53;
  TASK_METHOD_T kRegisterStager = 60;
  TASK_METHOD_T kUnregisterStager = 61;
  TASK_METHOD_T kStageIn = 62;
  TASK_METHOD_T kStageOut = 63;
  TASK_METHOD_T kCount = 64;
};

#endif  // CHI_HERMES_CORE_METHODS_H_