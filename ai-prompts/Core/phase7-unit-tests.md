Let's build a simple unit test for the adapter codes. We should link directly to the adapters, so no LD_PRELOAD.

Create a subdirectory called test/unit/adapters for this.

# Test 1: Open - Write - Read - Close 

For now, let's focus only on posix. Create a subdirectory called test/unit/adapters/posix.

Basic test:
Open a file in the /tmp directory
Write 16MB to the file.
Read 16MB from the file
Verify the write and read have the same results.
Close the file.
Remove the file

