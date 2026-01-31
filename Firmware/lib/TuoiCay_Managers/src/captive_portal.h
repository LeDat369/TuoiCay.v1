/**
 * @file captive_portal.h
 * @brief WiFi Provisioning via Captive Portal - TuoiCay.v1
 * @version 1.0.0
 * 
 * Provides SoftAP mode with captive portal for first-time WiFi configuration.
 * When device cannot connect to WiFi, it starts an AP with a captive portal
 * that redirects all requests to a configuration page.
 */

#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <functional>

//=============================================================================
// CONFIGURATION
//=============================================================================

#ifndef CAPTIVE_PORTAL_SSID
#define CAPTIVE_PORTAL_SSID         "TuoiCay-Setup"
#endif

#ifndef CAPTIVE_PORTAL_PASSWORD
#define CAPTIVE_PORTAL_PASSWORD     ""          // Open network by default
#endif

#ifndef CAPTIVE_PORTAL_TIMEOUT
#define CAPTIVE_PORTAL_TIMEOUT      300000      // 5 minutes timeout
#endif

#ifndef DNS_PORT
#define DNS_PORT                    53
#endif

//=============================================================================
// CALLBACK TYPES
//=============================================================================

// Called when WiFi credentials are received
using WiFiCredentialsCallback = std::function<void(const String& ssid, const String& password)>;

// Called when MQTT config is received
using MqttConfigCallback = std::function<void(const String& server, uint16_t port, 
                                               const String& user, const String& pass)>;

// Called when portal times out
using PortalTimeoutCallback = std::function<void()>;

//=============================================================================
// CAPTIVE PORTAL CLASS
//=============================================================================

class CaptivePortal {
public:
    /**
     * @brief Constructor
     */
    CaptivePortal();
    
    /**
     * @brief Initialize captive portal
     * @param apSSID SSID for the access point
     * @param apPassword Password for AP (empty for open network)
     * @return true if started successfully
     */
    bool begin(const char* apSSID = CAPTIVE_PORTAL_SSID, 
               const char* apPassword = CAPTIVE_PORTAL_PASSWORD);
    
    /**
     * @brief Stop captive portal and restore previous WiFi mode
     */
    void stop();
    
    /**
     * @brief Update - must be called in loop()
     */
    void update();
    
    /**
     * @brief Check if portal is active
     * @return true if portal is running
     */
    bool isActive() const { return _isActive; }
    
    /**
     * @brief Check if configuration was received
     * @return true if user submitted config
     */
    bool hasConfig() const { return _hasConfig; }
    
    /**
     * @brief Get configured WiFi SSID
     * @return SSID string
     */
    String getConfiguredSSID() const { return _configuredSSID; }
    
    /**
     * @brief Get configured WiFi password
     * @return Password string
     */
    String getConfiguredPassword() const { return _configuredPassword; }
    
    /**
     * @brief Get configured MQTT server
     * @return Server address
     */
    String getConfiguredMqttServer() const { return _mqttServer; }
    
    /**
     * @brief Get configured MQTT port
     * @return Port number
     */
    uint16_t getConfiguredMqttPort() const { return _mqttPort; }
    
    /**
     * @brief Set timeout for portal (ms)
     * @param timeout Timeout in milliseconds
     */
    void setTimeout(uint32_t timeout) { _timeout = timeout; }
    
    /**
     * @brief Set callback for WiFi credentials
     * @param callback Function to call when credentials received
     */
    void onCredentialsReceived(WiFiCredentialsCallback callback) {
        _onCredentials = callback;
    }
    
    /**
     * @brief Set callback for MQTT config
     * @param callback Function to call when MQTT config received
     */
    void onMqttConfigReceived(MqttConfigCallback callback) {
        _onMqttConfig = callback;
    }
    
    /**
     * @brief Set callback for timeout
     * @param callback Function to call on timeout
     */
    void onTimeout(PortalTimeoutCallback callback) {
        _onTimeout = callback;
    }
    
    /**
     * @brief Get AP IP address
     * @return IP address string
     */
    String getAPIP() const;
    
    /**
     * @brief Get number of connected stations
     * @return Station count
     */
    uint8_t getStationCount() const;

private:
    // Internal handlers
    void _handleRoot();
    void _handleScan();
    void _handleSave();
    void _handleNotFound();
    void _handleStatus();
    
    // Generate HTML pages
    String _generateConfigPage();
    String _generateSuccessPage();
    String _generateScanResultsJSON();
    
    // HTML/CSS helpers
    String _getCSS();
    String _escapeHTML(const String& str);
    
    // State
    ESP8266WebServer* _server;
    DNSServer _dnsServer;
    
    bool _isActive;
    bool _hasConfig;
    unsigned long _startTime;
    uint32_t _timeout;
    
    // Configured values
    String _configuredSSID;
    String _configuredPassword;
    String _mqttServer;
    uint16_t _mqttPort;
    String _mqttUser;
    String _mqttPass;
    
    // Callbacks
    WiFiCredentialsCallback _onCredentials;
    MqttConfigCallback _onMqttConfig;
    PortalTimeoutCallback _onTimeout;
    
    // Scan results cache
    int _scanResultCount;
    unsigned long _lastScanTime;
};

#endif // CAPTIVE_PORTAL_H
