#include "CrashHandler.h"

#if defined(CS_ENABLE_BREADKPAD)
#include <client/windows/handler/exception_handler.h>
#endif // CS_ENABLE_BREADKPAD

// Using non-null pipe name here lets breakpad try setting OOP crash handling
constexpr const wchar_t* PipeName = L"sawyer-bpad";

CrashHandler::initialise(const fs::path& dumpDirectory)
{
#ifdef CS_ENABLE_BREADKPAD
    auto wDumpDirectory = dumpDirectory.native();
    _handler = new google_breakpad::ExceptionHandler(
        wDumpDirectory, 0, [](const wchar_t* dumpPath, const wchar_t* miniDumpId, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded) -> void {
            auto handler = reinterpret_cast<google_breakpad::ExceptionHandler*>(context);
            auto dumpPath = fs::path(dumpPath) / miniDumpId;
            handler->onCrash(dumpPath, succeeded);
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

void CrashHandler::dispose()
{
#ifdef CS_ENABLE_BREADKPAD
    delete reinterpret_cast<google_breakpad::ExceptionHandler*>(_handler);
#endif
}
