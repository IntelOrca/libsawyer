#pragma once

#include "FileSystem.hpp"

class CrashHandler
{
private:
    void* _handler{};

public:
    ~CrashHandler();
    void initialise(const fs::path& dumpDirectory);

protected:
    virtual void onCrash(const fs::path& dumpPath, bool succeeded);
};
