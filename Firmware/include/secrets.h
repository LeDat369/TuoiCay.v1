/**
 * @file secrets.h
 * @brief WiFi and MQTT credentials
 * 
 * WARNING: This file contains sensitive information!
 * DO NOT commit to version control!
 */

#ifndef SECRETS_H
#define SECRETS_H

//=============================================================================
// WIFI CREDENTIALS (from kehoach.md)
//=============================================================================
#define WIFI_SSID           "WIFI-IOT"
#define WIFI_PASSWORD       "hnh.2025"

//=============================================================================
// MQTT BROKER (from kehoach.md)
//=============================================================================
#define MQTT_BROKER         "192.168.221.5"
#define MQTT_PORT           1883
#define MQTT_USERNAME       ""
#define MQTT_PASSWORD       ""

//=============================================================================
// OTA PASSWORD
//=============================================================================
#define OTA_PASSWORD        "tuoicay2026"

#endif // SECRETS_H
