#include <sys.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        write(1, "usage: debug <ident>\n", 21);
        return 1;
    }

    debug(argv[1]);

    return 0;
}
