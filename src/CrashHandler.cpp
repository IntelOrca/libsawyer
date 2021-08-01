#include "CrashHandler.h"

#if defined(CS_ENABLE_BREAKPAD)
#include <client/windows/handler/exception_handler.h>
#endif // CS_ENABLE_BREAKPAD

using namespace cs;

// Using non-null pipe name here lets breakpad try setting OOP crash handling
constexpr const wchar_t* PipeName = L"sawyer-bpad";

void CrashHandler::initialise(const fs::path& dumpDirectory)
{
#ifdef CS_ENABLE_BREAKPAD
    auto wDumpDirectory = dumpDirectory.native();
    _handler = new google_breakpad::ExceptionHandler(
        wDumpDirectory, 0, [](const wchar_t* dumpPath, const wchar_t* miniDumpId, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded) -> bool {
            auto handler = reinterpret_cast<CrashHandler*>(context);
            auto fullDumpPath = (fs::path(dumpPath) / miniDumpId).replace_extension(".dmp");
            handler->onCrash(fullDumpPath, succeeded);
            return succeeded;
        },
        this,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpWithDataSegs,
        PipeName,
        0);
#endif
}

void CrashHandler::onCrash(const fs::path& dumpPath, bool succeeded)
{
}

CrashHandler::~CrashHandler()
{
#ifdef CS_ENABLE_BREAKPAD
    delete reinterpret_cast<google_breakpad::ExceptionHandler*>(_handler);
#endif
}
