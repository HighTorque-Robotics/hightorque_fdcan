#include "canport.h"

#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>

std::atomic<bool> exitFlag(false);
void signalHandler(int signum) 
{
    exitFlag.store(true);
}


int main()
{
    canport port1(1);

    uint8_t tdata[64] = {0};
    for (uint8_t i = 0; i < sizeof(tdata); i++)
    {
        tdata[i] = i;
    }

    while (!exitFlag.load())
    {
        static uint32_t id = 0;
        port1.fdcan_tran_config(false, false, true);

        for (int i = 0; i < 20; i++)
        {
            if (port1.fdcan_tran_data(i, tdata, sizeof(tdata)))
            {
                break;
            }
        }
        port1.send();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}