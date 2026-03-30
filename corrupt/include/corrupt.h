#ifndef CORRUPT_LIBRARY_H
#define CORRUPT_LIBRARY_H

#include <gtk/gtk.h>
#include <glib.h>

class bucket;

class corrupt
{
private:
    std::vector<std::unique_ptr<bucket>> buckets;
    std::stop_source master_stop_source;

    void addToBucket(const std::string &name);

public:
    corrupt();
    ~corrupt();

    bool addFile(const std::string &name);
    void shred();
};

#endif // CORRUPT_LIBRARY_H
