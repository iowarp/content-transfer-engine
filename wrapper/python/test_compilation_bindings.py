#!/usr/bin/env python3
"""
Compilation-focused test for WRP CTE Core Python bindings
Tests that all bindings compile and are accessible without full runtime initialization
"""

import sys

def test_import():
    """Test that the module can be imported successfully"""
    try:
        import wrp_cte_core_ext as cte
        print("✅ Module import successful")
        return True
    except ImportError as e:
        print(f"❌ Module import failed: {e}")
        return False

def test_basic_types():
    """Test that basic types can be created"""
    try:
        import wrp_cte_core_ext as cte
        
        # Test enum
        op = cte.CteOp.kPutBlob
        print(f"✅ CteOp enum created: {op}")
        
        # Test TagId
        tag_id = cte.TagId()
        null_tag = cte.TagId.GetNull()
        print(f"✅ TagId created, null check: {null_tag.IsNull()}")
        
        # Test BlobId
        blob_id = cte.BlobId()
        null_blob = cte.BlobId.GetNull()
        print(f"✅ BlobId created, null check: {null_blob.IsNull()}")
        
        # Test MemContext
        mctx = cte.MemContext()
        print("✅ MemContext created")
        
        # Test CteTelemetry
        telemetry = cte.CteTelemetry()
        print(f"✅ CteTelemetry created with op: {telemetry.op_}")
        
        # Test Client type accessibility (without instantiation)
        client_type = cte.Client
        print(f"✅ Client type accessible: {client_type}")
        
        return True
    except Exception as e:
        print(f"❌ Basic types test failed: {e}")
        return False

def test_functions_accessibility():
    """Test that module functions are accessible without calling them"""
    try:
        import wrp_cte_core_ext as cte
        
        # Test initialization functions exist
        runtime_init = cte.chimaera_runtime_init
        client_init = cte.chimaera_client_init
        cte_init = cte.initialize_cte
        print(f"✅ chimaera_runtime_init function accessible: {runtime_init}")
        print(f"✅ chimaera_client_init function accessible: {client_init}")
        print(f"✅ initialize_cte function accessible: {cte_init}")
        
        # Test client getter function exists
        client_getter = cte.get_cte_client
        print(f"✅ get_cte_client function accessible: {client_getter}")
        
        return True
    except Exception as e:
        print(f"❌ Functions accessibility test failed: {e}")
        return False

def test_poll_telemetry_log_binding():
    """Test PollTelemetryLog method binding without runtime initialization"""
    try:
        import wrp_cte_core_ext as cte
        
        # Test Client type has PollTelemetryLog method
        client_type = cte.Client
        
        # Check if PollTelemetryLog method exists on the type
        if hasattr(client_type, 'PollTelemetryLog'):
            print("✅ PollTelemetryLog method exists on Client class")
        else:
            print("❌ PollTelemetryLog method not found on Client class")
            return False
        
        # Test MemContext creation for method calls
        mctx = cte.MemContext()
        print("✅ Created MemContext for potential method calls")
        
        print("✅ PollTelemetryLog binding verification successful")
        return True
    except Exception as e:
        print(f"❌ PollTelemetryLog binding test failed: {e}")
        return False

def test_runtime_functions_signature():
    """Test that runtime initialization functions have correct signatures"""
    try:
        import wrp_cte_core_ext as cte
        import inspect
        
        # Check chimaera_runtime_init signature
        runtime_init_sig = inspect.signature(cte.chimaera_runtime_init)
        print(f"✅ chimaera_runtime_init signature: {runtime_init_sig}")
        
        # Check chimaera_client_init signature
        client_init_sig = inspect.signature(cte.chimaera_client_init)
        print(f"✅ chimaera_client_init signature: {client_init_sig}")
        
        # Check initialize_cte signature
        cte_init_sig = inspect.signature(cte.initialize_cte)
        print(f"✅ initialize_cte signature: {cte_init_sig}")
        
        # Check get_cte_client signature
        client_getter_sig = inspect.signature(cte.get_cte_client)
        print(f"✅ get_cte_client signature: {client_getter_sig}")
        
        return True
    except Exception as e:
        print(f"❌ Function signature test failed: {e}")
        return False

def main():
    """Run all compilation-focused tests"""
    print("🧪 Running Python bindings compilation verification tests...")
    
    tests = [
        ("Module Import", test_import),
        ("Basic Types", test_basic_types),
        ("Functions Accessibility", test_functions_accessibility),
        ("PollTelemetryLog Binding", test_poll_telemetry_log_binding),
        ("Runtime Functions Signature", test_runtime_functions_signature)
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
        print("🎉 All Python bindings compilation tests passed!")
        return 0
    else:
        print("💥 Some tests failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())