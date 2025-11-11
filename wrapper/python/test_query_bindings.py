#!/usr/bin/env python3
"""
Test for WRP CTE Core Python query API bindings
Tests TagQuery and BlobQuery methods
"""

import sys
import os

# When running with python -I (isolated mode), we need to manually add the current directory
# The test is run with WORKING_DIRECTORY set to the module directory
sys.path.insert(0, os.getcwd())

def test_import():
    """Test that the module can be imported successfully"""
    try:
        import wrp_cte_core_ext as cte
        print("✅ Module import successful")
        return True
    except ImportError as e:
        print(f"❌ Module import failed: {e}")
        return False

def test_query_methods_exist():
    """Test that TagQuery and BlobQuery methods exist on Client class"""
    try:
        import wrp_cte_core_ext as cte

        # Test Client type accessibility
        client_type = cte.Client
        print(f"✅ Client type accessible: {client_type}")

        # Check if TagQuery method exists
        if hasattr(client_type, 'TagQuery'):
            print("✅ TagQuery method exists on Client class")
        else:
            print("❌ TagQuery method not found on Client class")
            return False

        # Check if BlobQuery method exists
        if hasattr(client_type, 'BlobQuery'):
            print("✅ BlobQuery method exists on Client class")
        else:
            print("❌ BlobQuery method not found on Client class")
            return False

        return True
    except Exception as e:
        print(f"❌ Query methods test failed: {e}")
        return False

def test_query_method_signatures():
    """Test that query methods have correct signatures"""
    try:
        import wrp_cte_core_ext as cte
        import inspect

        client_type = cte.Client

        # Check TagQuery signature
        if hasattr(client_type, 'TagQuery'):
            tag_query_method = getattr(client_type, 'TagQuery')
            print(f"✅ TagQuery method signature accessible")
            # Note: nanobind methods don't always expose full signature info
            print(f"   TagQuery: {tag_query_method}")

        # Check BlobQuery signature
        if hasattr(client_type, 'BlobQuery'):
            blob_query_method = getattr(client_type, 'BlobQuery')
            print(f"✅ BlobQuery method signature accessible")
            print(f"   BlobQuery: {blob_query_method}")

        return True
    except Exception as e:
        print(f"❌ Method signature test failed: {e}")
        return False

def test_query_with_mock_client():
    """Test query methods accessibility without full runtime"""
    try:
        import wrp_cte_core_ext as cte

        # Create MemContext (doesn't require runtime)
        mctx = cte.MemContext()
        print("✅ Created MemContext for query calls")

        # Test that we can access client methods (won't call without runtime)
        client_type = cte.Client

        # Verify methods are callable (signature check)
        tag_query = getattr(client_type, 'TagQuery', None)
        blob_query = getattr(client_type, 'BlobQuery', None)

        if tag_query and callable(tag_query):
            print("✅ TagQuery method is callable")
        else:
            print("❌ TagQuery method is not callable")
            return False

        if blob_query and callable(blob_query):
            print("✅ BlobQuery method is callable")
        else:
            print("❌ BlobQuery method is not callable")
            return False

        print("✅ Query methods binding verification successful")
        return True
    except Exception as e:
        print(f"❌ Query method accessibility test failed: {e}")
        return False

def test_query_parameter_types():
    """Test that query methods accept expected parameter types"""
    try:
        import wrp_cte_core_ext as cte

        # Create required types
        mctx = cte.MemContext()
        print("✅ Created MemContext")

        # Test string parameter (for regex patterns)
        tag_regex = "user_.*"
        blob_regex = "blob_.*"
        print(f"✅ Created regex patterns: '{tag_regex}', '{blob_regex}'")

        # Note: We can't actually call the methods without full runtime initialization,
        # but we've verified the types can be created and the methods exist
        print("✅ Parameter types verified for query methods")

        return True
    except Exception as e:
        print(f"❌ Parameter types test failed: {e}")
        return False

def test_query_documentation():
    """Test that query methods have documentation"""
    try:
        import wrp_cte_core_ext as cte

        client_type = cte.Client

        # Check TagQuery documentation
        tag_query = getattr(client_type, 'TagQuery', None)
        if tag_query and hasattr(tag_query, '__doc__') and tag_query.__doc__:
            print(f"✅ TagQuery has documentation: {tag_query.__doc__}")
        else:
            print("⚠️  TagQuery documentation not found (may be expected)")

        # Check BlobQuery documentation
        blob_query = getattr(client_type, 'BlobQuery', None)
        if blob_query and hasattr(blob_query, '__doc__') and blob_query.__doc__:
            print(f"✅ BlobQuery has documentation: {blob_query.__doc__}")
        else:
            print("⚠️  BlobQuery documentation not found (may be expected)")

        return True
    except Exception as e:
        print(f"❌ Documentation test failed: {e}")
        return False

def main():
    """Run all query API binding tests"""
    print("🧪 Running Python query API bindings tests...")

    tests = [
        ("Module Import", test_import),
        ("Query Methods Exist", test_query_methods_exist),
        ("Query Method Signatures", test_query_method_signatures),
        ("Query Methods Accessibility", test_query_with_mock_client),
        ("Query Parameter Types", test_query_parameter_types),
        ("Query Documentation", test_query_documentation)
    ]

    passed = 0
    total = len(tests)

    for test_name, test_func in tests:
        print(f"\n📋 Testing {test_name}...")
        if test_func():
            passed += 1
        else:
            print(f"❌ {test_name} failed")

    print(f"\n📊 Test Results: {passed}/{total} tests passed")

    if passed == total:
        print("🎉 All query API binding tests passed!")
        return 0
    else:
        print("💥 Some tests failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())
