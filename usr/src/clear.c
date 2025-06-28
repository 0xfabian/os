#include <sys.h>

int main()
{
    ioctl(1, FBTERM_CLEAR, 0);
    return 0;
}
