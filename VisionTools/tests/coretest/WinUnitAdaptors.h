////////////////////////////////////////////////////////////
// Use the built in VS Test runner for versions of VS that support it.
// Fall back to WinUnit on older versions

#if _MSC_VER >= 1800

#pragma warning(disable:4505)		// VS Unit test framework generates 4505. Actual VT Build will flag this if it's ever an issue for us
#include "CppUnitTest.h"

#define BEGIN_TEST(x) TEST_METHOD(x) \
{
#define END_TEST }

#define BEGIN_TEST_FILE(x) TEST_CLASS(x) \
{

#define END_TEST_FILE };

#define WIN_ASSERT_FAIL(msg, ...) Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(msg, __VA_ARGS__);
#define WIN_ASSERT_TRUE(condition, ...) Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(condition, __VA_ARGS__);
#define WIN_ASSERT_EQUAL(expected, actual, ...) Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(expected, actual, __VA_ARGS__);
#define WIN_ASSERT_NOT_NULL(condition, ...) Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsNotNull(condition,  __VA_ARGS__);

#else

#include "WinUnit.h"

#define BEGIN_TEST_FILE(x)
#define END_TEST_FILE

#endif