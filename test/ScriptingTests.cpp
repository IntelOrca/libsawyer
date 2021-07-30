#ifdef CS_ENABLE_SCRIPTING

#include <cmath>
#include <cstdio>
#include <gtest/gtest.h>
#include <sawyer/Scripting.h>
#include <stdexcept>

#ifndef DUK_USE_CPP_EXCEPTIONS
#error Expected C++ exceptions to be supported
#endif

#ifndef DUK_USE_EXEC_TIMEOUT_CHECK
#error Expected exec timeout to be supported
#endif

static constexpr const char* CUSTOM_ERROR_MESSAGE = "A custom error occured.";
static constexpr const char* CUSTOM_EXCEPTION_MESSAGE = "A custom exception occured.";

static bool _shouldTimeout;

static duk_ret_t throwError(duk_context* ctx)
{
    return duk_error(ctx, DUK_ERR_ERROR, CUSTOM_ERROR_MESSAGE);
}

static duk_ret_t thowException(duk_context* ctx)
{
    throw std::runtime_error(CUSTOM_EXCEPTION_MESSAGE);
}

duk_bool_t duk_exec_timeout_check(void*)
{
    return _shouldTimeout;
}

class ScriptingTests : public testing::Test
{
protected:
    void SetUp() override
    {
        _shouldTimeout = false;
    }
};

TEST_F(ScriptingTests, testBasic)
{
    auto ctx = duk_create_heap_default();
    duk_eval_string(ctx, "1 + 4");
    ASSERT_EQ(duk_get_type(ctx, -1), DUK_TYPE_NUMBER);
    ASSERT_EQ(duk_get_int(ctx, -1), 5);
    duk_destroy_heap(ctx);
}

TEST_F(ScriptingTests, testErrorHandling)
{
    auto ctx = duk_create_heap_default();
    duk_push_c_function(ctx, throwError, 0);
    duk_put_global_string(ctx, "throwError");
    duk_push_c_function(ctx, thowException, 0);
    duk_put_global_string(ctx, "throwException");

    // Check we can catch our own errors
    ASSERT_THROW(duk_eval_string(ctx, "throwError();"), std::runtime_error);

    // Check JS can catch our own errors
    duk_eval_string(ctx, "try { throwError(); } catch (err) { err.message; }");
    ASSERT_EQ(duk_get_type(ctx, -1), DUK_TYPE_STRING);
    ASSERT_STREQ(duk_get_string(ctx, -1), CUSTOM_ERROR_MESSAGE);

    // Check JS can catch our own exceptions
    duk_eval_string(ctx, "try { throwException(); } catch (err) { err.message; }");
    ASSERT_EQ(duk_get_type(ctx, -1), DUK_TYPE_STRING);

    // Check we can catch JS exceptions
    ASSERT_THROW(duk_eval_string(ctx, "throw new Error('user exception')"), std::runtime_error);
    ASSERT_THROW(duk_eval_string(ctx, "{}.test"), std::runtime_error);

    duk_destroy_heap(ctx);
}

TEST_F(ScriptingTests, testTimeout)
{
    auto ctx = duk_create_heap_default();
    _shouldTimeout = true;
    ASSERT_THROW(duk_eval_string(ctx, "while (true) { }"), std::runtime_error);
    duk_destroy_heap(ctx);
}

static double mag(const DukValue& a, const DukValue& b)
{
    auto ctx = a.context();
    try
    {
        auto aNumber = a.as_double();
        auto bNumber = b.as_double();
        auto cNumber = std::sqrt((aNumber * aNumber) + (bNumber * bNumber));
        return cNumber;
    }
    catch (const DukException&)
    {
        return duk_error(ctx, DUK_ERR_ERROR, "mag expects two numbers as arguments");
    }
}

TEST_F(ScriptingTests, dukGlue)
{
    auto ctx = duk_create_heap_default();
    dukglue_register_function(ctx, mag, "mag");

    // Valid call
    duk_eval_string(ctx, "mag(3, 4)");
    ASSERT_EQ(duk_get_type(ctx, -1), DUK_TYPE_NUMBER);
    ASSERT_EQ(duk_get_number(ctx, -1), 5);

    // Invalid call
    duk_eval_string(ctx, "try { mag(); } catch (err) { 999; }");
    ASSERT_EQ(duk_get_type(ctx, -1), DUK_TYPE_NUMBER);
    ASSERT_EQ(duk_get_number(ctx, -1), 999);

    duk_destroy_heap(ctx);
}

#endif
