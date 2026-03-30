#include "corrupt.h"
#include "bucket.h"
#include "utility.h"
#include <iostream>
#include <thread>

corrupt::corrupt() = default;
corrupt::~corrupt() = default;

void corrupt::addToBucket(const std::string &name)
{
    // Get devname of the device the file is on
    struct stat st;

    // 1. Get the file status, which includes the device ID (st_dev)
    if (lstat(name.c_str(), &st) != 0)
    {
        std::cerr << "Error calling stat()\n";
        return;
    }
    dev_t id = st.st_dev;

    // Check to see if we already created a bucket for this file.
    for (auto &bucket : buckets)
    {
        if (bucket->getDeviceID() == id)
        {
            bucket->addFile(name);
            return;
        }
    }

    strategy strat = getStrategy(name.c_str());

    buckets.push_back(std::make_unique<bucket>(id, strat));
    buckets.back()->addFile(name);
}

bool corrupt::addFile(const std::string &name)
{
    bool result = check_file(name.c_str());
    if (result == false)
    {
        return false;
    }

    addToBucket(name);

    return true;
}

void corrupt::shred()
{
    std::stop_token stoken = master_stop_source.get_token();

    for (auto &bucket : buckets)
    {
        bucket->shred(stoken);
    }
}
