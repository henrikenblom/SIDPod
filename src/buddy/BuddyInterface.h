//
// Created by Henrik Enblom on 2025-04-20.
//

#ifndef BUDDYINTERFACE_H
#define BUDDYINTERFACE_H

enum RequestType {
    RT_NONE = 0,
    RT_BT_LIST = 1,
    RT_BT_SELECT = 2,
    RT_BT_DISCONNECT = 3,
};

struct Request {
    RequestType type;
    char data[32];
};
#endif //BUDDYINTERFACE_H
