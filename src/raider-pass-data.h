#include "raider-window.h"

struct _pass_data
{
    RaiderWindow *window;
    GInputStream *stream;
    GDataInputStream *data_stream;
};
