#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "core/order.hpp"

using exchange::core::AlignedOrder;

int main()
{
    // TODO: Make this more robust
    int fd = open("/home/theta/Hari/coding/stock_exchange_v2/replay_log/18749297956937.log", O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    AlignedOrder o;
    size_t offset = 0;
    while (true)
    {
        const auto bytesRead = pread(fd, &o, sizeof(o), offset);
        if (bytesRead == -1)
        {
            perror("pread");
            return 1;
        }
        else if (bytesRead == 0)
        {
            break;
        }
        std::cout << "Read back order " << o << std::endl;
        offset += bytesRead;
    }

    if (close(fd) < 0)
    {
        perror("close");
        return 1;
    }
}