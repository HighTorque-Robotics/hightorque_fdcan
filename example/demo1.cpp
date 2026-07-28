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
    std::signal(SIGINT, signalHandler);
    canport port1(1);

    uint8_t tdata[4] = {0};
    for (uint8_t i = 0; i < sizeof(tdata); i++)
    {
        tdata[i] = i;
    }

    uint32_t id = 0;
    fdcan_msg_s fdcan_buf;
    while (!exitFlag.load())
    {

        // while (port1.fdcan_queue_pop(&fdcan_buf))
        // {
        //     printf("id = %X, len = %d\n", fdcan_buf.id, fdcan_buf.len);
        //     printf("fdcan = %d, brs = %d, id = %d\n", fdcan_buf.config.fdcan_flag, fdcan_buf.config.brs_flag, fdcan_buf.config.id_flag);

        //     for (int i = 0; i < fdcan_buf.len; i++)
        //     {
        //         printf("%02X ", fdcan_buf.data[i]);
        //         if (i == 31)
        //         {
        //             printf("\n");
        //         }
        //     }
        //     printf("\n\n\n");
        // }

        for (int i = 0; i < 20; i++)
        {
            if (port1.fdcan_data(i, tdata, sizeof(tdata), true, true, true))
            {
                break;
            }
        }
        port1.fdcan_send();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}