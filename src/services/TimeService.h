#pragma once

#include <Arduino.h>
#include <time.h>

class TimeService {
public:
    void begin();
    void update();
    bool getLocalTime(struct tm& result) const;
    bool isValid() const;
    time_t utcNow() const;  // 0 while the clock is not set

private:
    bool rtcAvailable_ = false;
    unsigned long lastRtcWriteMs_ = 0;
    bool readRtcUtc(time_t& epoch);
    void writeRtcUtc(time_t epoch);
    static bool validRtcDateTime(int year, int month, int day,
                                 int hour, int minute, int second);
    static time_t utcToEpoch(int year, int month, int day,
                             int hour, int minute, int second);
};
