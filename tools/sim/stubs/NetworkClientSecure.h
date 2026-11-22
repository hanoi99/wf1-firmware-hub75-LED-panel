#pragma once
#include "WiFi.h"
class NetworkClientSecure : public NetworkClient {
public:
    void setInsecure() {}
    void setTimeout(uint32_t seconds) {}
};
