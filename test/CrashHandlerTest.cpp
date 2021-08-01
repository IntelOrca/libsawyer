#include <sawyer/CrashHandler.h>

static void forceCrash()
{
    static int* ptr = nullptr;
    *ptr = 0;
}

class MyCrashHandler : public cs::CrashHandler
{
protected:
    void onCrash(const fs::path& dumpPath, bool succeeded) override
    {
        std::printf("onCrash(\"%s\", %s)\n", dumpPath.u8string().c_str(), succeeded ? "true" : "false");

        std::error_code ec;
        if (fs::exists(dumpPath, ec))
        {
            if (fs::remove(dumpPath))
            {
                std::printf("Success\n");
                exit(0);
            }
            else
            {
                std::fprintf(stderr, "Unable to delete dump file\n");
            }
        }
        else
        {
            std::fprintf(stderr, "Unable to find created dump file\n");
        }
        exit(1);
    }
};

int main()
{
    std::printf("Testing crash handler...\n");
    auto cwd = fs::current_path();
    MyCrashHandler crashHandler;
    crashHandler.initialise(cwd);
    std::printf("Crash handler initialised\n");
    std::printf("Dump directory = %s\n", cwd.u8string().c_str());
    forceCrash();
    std::fprintf(stderr, "Reached end of main()\n");
    return 1;
}
