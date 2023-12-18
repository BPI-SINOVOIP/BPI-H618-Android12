
#include "DeviceConfig.h"

int main(int, char**)
{
    std::unique_ptr<sunxi::DeviceConfig> config = std::make_unique<sunxi::DeviceConfig>();
    if (config->init() != 0) {
        printf("sunxi::DeviceConfig init failed");
        return -1;
    }
    std::string dumpstr;
    config->dump(dumpstr);
    printf("%s\n", dumpstr.c_str());
    return 0;
}
