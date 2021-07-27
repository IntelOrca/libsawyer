#ifdef CS_ENABLE_SCRIPTING

#include <duktape.h>
#include <gtest/gtest.h>

TEST(ScriptingTests, testBasic)
{
    auto ctx = duk_create_heap_default();
    duk_destroy_heap(ctx);
}

#endif
