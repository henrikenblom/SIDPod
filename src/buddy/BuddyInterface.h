//
// Created by Henrik Enblom on 2025-04-20.
//

#ifndef BUDDYINTERFACE_H
#define BUDDYINTERFACE_H

#define BUDDY_ENABLE_PIN                    22

#define BUDDY_TAP_PIN                       21
#define BUDDY_VERTICAL_PIN             20
#define BUDDY_HORIZONTAL_PIN           19
#define BUDDY_ROTATE_PIN               18
#define BUDDY_MODIFIER1_PIN                 17
#define BUDDY_MODIFIER2_PIN                 16

#define BUDDY_BT_CONNECTED_PIN              15

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
