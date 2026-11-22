#include "TimeService.h"

#include <Wire.h>
#include <I2C_BM8563.h>
#include <sys/time.h>
#include <esp_sntp.h>

#include "board/BoardConfig.h"

namespace {
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire1);
constexpr time_t MIN_VALID_EPOCH = 1600000000;
}

time_t TimeService::utcToEpoch(int year, int month, int day,
                               int hour, int minute, int second) {
    // Civil-date conversion; independent of the configured local timezone.
    const int y = year - (month <= 2 ? 1 : 0);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned mp = static_cast<unsigned>(month + (month > 2 ? -3 : 9));
    const unsigned doy = (153U * mp + 2U) / 5U + static_cast<unsigned>(day) - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    const int64_t days = static_cast<int64_t>(era) * 146097LL + doe - 719468LL;
    return static_cast<time_t>(days * 86400LL + hour * 3600LL + minute * 60LL + second);
}

bool TimeService::validRtcDateTime(int year, int month, int day,
                                   int hour, int minute, int second) {
    if (year < 2020 || year > 2099 || month < 1 || month > 12 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59 || day < 1 || day > 31) return false;
    static const uint8_t daysPerMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxDay = daysPerMonth[month - 1];
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month == 2 && leap) maxDay = 29;
    return day <= maxDay;
}

bool TimeService::readRtcUtc(time_t& epoch) {
    I2C_BM8563_DateTypeDef date;
    I2C_BM8563_TimeTypeDef time;
    rtc.getDate(&date);
    rtc.getTime(&time);
    if (!validRtcDateTime(date.year, date.month, date.date,
                          time.hours, time.minutes, time.seconds)) return false;
    epoch = utcToEpoch(date.year, date.month, date.date,
                       time.hours, time.minutes, time.seconds);
    return epoch >= MIN_VALID_EPOCH;
}

void TimeService::writeRtcUtc(time_t epoch) {
    struct tm utc{};
    gmtime_r(&epoch, &utc);
    I2C_BM8563_TimeTypeDef time;
    time.hours = utc.tm_hour;
    time.minutes = utc.tm_min;
    time.seconds = utc.tm_sec;
    rtc.setTime(&time);

    I2C_BM8563_DateTypeDef date;
    date.weekDay = utc.tm_wday;
    date.month = utc.tm_mon + 1;
    date.date = utc.tm_mday;
    date.year = utc.tm_year + 1900;
    rtc.setDate(&date);
}

void TimeService::begin() {
    setenv("TZ", BoardConfig::TIMEZONE, 1);
    tzset();

    Wire1.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);
    rtc.begin();
    Wire1.beginTransmission(I2C_BM8563_DEFAULT_ADDRESS);
    rtcAvailable_ = (Wire1.endTransmission() == 0);

    time_t rtcEpoch = 0;
    if (rtcAvailable_ && readRtcUtc(rtcEpoch) && time(nullptr) < MIN_VALID_EPOCH) {
        timeval tv{};
        tv.tv_sec = rtcEpoch;
        settimeofday(&tv, nullptr);
        Serial.println("[time] system clock initialized from BM8563 RTC (UTC)");
    }

    // SNTP runs asynchronously; the display loop is not blocked waiting for it.
    configTzTime(BoardConfig::TIMEZONE,
                 BoardConfig::NTP_SERVER_1,
                 BoardConfig::NTP_SERVER_2);
    Serial.printf("[time] timezone configured; RTC %s\n", rtcAvailable_ ? "available" : "not detected");
}

void TimeService::update() {
    if (!rtcAvailable_ || !isValid()) return;
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) return;
    if (lastRtcWriteMs_ == 0 || millis() - lastRtcWriteMs_ >= 3600000UL) {
        writeRtcUtc(time(nullptr));
        lastRtcWriteMs_ = millis();
        Serial.println("[time] BM8563 synchronized from system clock");
    }
}

bool TimeService::getLocalTime(struct tm& result) const {
    const time_t now = time(nullptr);
    if (now < MIN_VALID_EPOCH) return false;
    localtime_r(&now, &result);
    return true;
}

bool TimeService::isValid() const {
    return time(nullptr) >= MIN_VALID_EPOCH;
}

time_t TimeService::utcNow() const {
    const time_t now = time(nullptr);
    return now >= MIN_VALID_EPOCH ? now : 0;
}
