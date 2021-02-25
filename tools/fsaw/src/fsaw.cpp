#include <cstdio>
#include <sawyer/SawyerStream.h>

int main(int argc, const char** argv)
{
    if (argc <= 1)
    {
        std::printf("usage: fsaw <file>");
        return 1;
    }

    try
    {
        cs::SawyerStreamReader reader(fs::u8path(argv[1]));
        auto valid = reader.validateChecksum();
        if (valid)
        {
            std::printf("Checksum valid!\n");
        }
        else
        {
            std::printf("Checksum invalid!\n");
        }
        return 0;
    }
    catch (...)
    {
        std::fprintf(stderr, "Unable to read %s\n", argv[1]);
        return 2;
    }
}
