#include "bucket.h"
#include <iostream>
#include <thread>

bucket::bucket(dev_t id, struct strategy strategy) : deviceID(id), strategy(strategy)
{
}

bucket::~bucket()
{
}

dev_t bucket::getDeviceID()
{
    return deviceID;
}

bool bucket::addFile(const std::string &name)
{
    // Add check to make sure the file matches the rules of the strategy.
    files.push_back(name);
    return true;
}

void bucket::shred(std::stop_token stoken)
{
    for (int i = 0; i < strategy.thread_count; i++)
    {
        worker_threads.emplace_back([this, stoken]()
        {
            while (!stoken.stop_requested())
            {
                std::string file_to_shred;
                {
                    std::lock_guard<std::mutex> lock(this->queue_mutex);

                    if (this->files.empty())
                    {
                        break;
                    }

                    file_to_shred = this->files.back();
                    this->files.pop_back();
                }
                corrupt_file(file_to_shred.c_str(), this->strategy);
            }
        });
    }
}
