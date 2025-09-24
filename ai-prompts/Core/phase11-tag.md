@CLAUDE.md

We need to make a class called Tag. This is a wrapper around the core CTE tag + blob operations.

The api is roughly as follows:
```cpp
class Tag {
private:
  TagId tag_id_;
  std::string tag_name_;

public:
  // Call the WRP_CTE client GetOrCreateTag function.
  Tag(const std::string &tag_name);

  // Does not call WRP_CTE client function, just sets the TagId variable
  Tag(const TagId &tag_id);

  // PutBlob. Allocates a SHM pointer and then calls PutBlob (SHM)
  void PutBlob(const std::string &blob_name, const char *data, size_t data_size, size_t off = 0);

  // PutBlob (SHM)
  void PutBlob(const std::string &blob_name, const hipc::Pointer &data, size_t data_size, size_t off = 0, float score = 1)

  // Asynchrounous PutBlob
  FullPtr<PutBlobTask> AsyncPutBlob(const std::string &blob_name, const char *data,  size_t data_size, size_t off = 0, float score = 1);

  // Asynchronous PutBlob (SHM)
  FullPtr<PutBlobTask> AsyncPutBlob(const std::string &blob_name, const hipc::Pointer &data,  size_t data_size, size_t off = 0, float score = 1);

  // Pointer does not need to exist. If data size is 0, Getblob should allocate a new pointer 
  void GetBlob(const std::string &blob_name, hipc::Pointer data, size_t data_size, size_t off = 0);

  // Get blob score
  void GetBlobScore(const std::string &blob_name);
};
```

We need to implement a new GetBlobSCore api in the runtime. It needs to be added to the chimaera_mod.yaml file. It also needs to be added to all other implemention files. Check @docs/chimaera/MODULE_DEVELOPMENT_GUIDE.md to see how to add new methods. Use /home/llogan/.scspkg/packages/iowarp-runtime/bin/chi_refresh_repo for chi_refresh_repo.