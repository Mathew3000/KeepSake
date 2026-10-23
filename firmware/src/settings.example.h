#pragma once
// Copy to firmware/main/settings.h and edit locally (this file is gitignored).
#define KS_WIFI_SSID "your-ssid"
#define KS_WIFI_PASSWORD "your-password"
#define KS_BACKUP_BASE_URL "https://backup.example.org:8443/backup"
#define KS_DEVICE_ID "offsite-01"
#define KS_SYNC_SECONDS 86400ULL
#define KS_SD_CLK 36
#define KS_SD_CMD 35
#define KS_SD_D0 37
#define KS_MAX_BUNDLE_BYTES (16 * 1024 * 1024)
