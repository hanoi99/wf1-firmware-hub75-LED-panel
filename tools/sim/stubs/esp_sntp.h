#pragma once
typedef enum { SNTP_SYNC_STATUS_RESET, SNTP_SYNC_STATUS_COMPLETED, SNTP_SYNC_STATUS_IN_PROGRESS } sntp_sync_status_t;
inline sntp_sync_status_t sntp_get_sync_status() { return SNTP_SYNC_STATUS_COMPLETED; }
inline void configTzTime(const char*, const char*, const char* = nullptr, const char* = nullptr) {}
