/**
 * @file wifi_manager.h
 * @brief WiFi Connection Manager with auto-reconnect
 * 
 * LOGIC:
 * - State machine: IDLE -> CONNECTING -> CONNECTED -> DISCONNECTED
 * - Auto-reconnect with exponential backoff
 * - LED status indicator
 * - Connection events for other modules
 * 
 * RULES: #WIFI(8) #ERROR(6)
 * 
 * Note: Using TCWiFiState to avoid conflict with ESP8266WiFiGeneric.h WiFiState
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <config.h>

//=============================================================================
// WIFI STATE ENUM (TC prefix = TuoiCay to avoid conflicts)
//=============================================================================
enum class TCWiFiState : uint8_t {
    IDLE = 0,           // Not started
    CONNECTING = 1,     // Attempting connection
    CONNECTED = 2,      // Connected to AP
    DISCONNECTED = 3    // Was connected, now disconnected
};

//=============================================================================
// CALLBACK TYPE
//=============================================================================
typedef void (*WiFiEventCallback)(TCWiFiState newState);

//=============================================================================
// WIFI MANAGER CLASS
//=============================================================================

/**
 * @class WiFiManager
 * @brief Manages WiFi connection with auto-reconnect
 */
class WiFiManager {
public:
    /**
     * @brief Constructor
     */
    WiFiManager();
    
    /**
     * @brief Initialize WiFi manager (does not connect yet)
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @return true if parameters valid
     */
    bool begin(const char* ssid, const char* password);
    
    /**
     * @brief Start WiFi connection (non-blocking)
     * @return true if connection attempt started
     */
    bool connect();
    
    /**
     * @brief Disconnect from WiFi
     */
    void disconnect();
    
    /**
     * @brief Update WiFi state (call in loop)
     * Handles:
     * - Connection timeout
     * - Reconnection with exponential backoff
     */
    void update();
    
    /**
     * @brief Check if connected to WiFi
     */
    bool isConnected() const;
    
    /**
     * @brief Get current state
     */
    TCWiFiState getState() const { return _state; }
    
    /**
     * @brief Get state as string
     */
    const char* getStateString() const;
    
    /**
     * @brief Get local IP address
     */
    IPAddress getIP() const;
    
    /**
     * @brief Get IP as string
     */
    String getIPString() const;
    
    /**
     * @brief Get RSSI (signal strength)
     */
    int32_t getRSSI() const;
    
    /**
     * @brief Get connected SSID
     */
    String getSSID() const { return _ssid; }
    
    /**
     * @brief Get MAC address as string
     */
    String getMACString() const;
    
    /**
     * @brief Get MAC address without colons (for device ID)
     */
    String getDeviceId() const;
    
    /**
     * @brief Set callback for state changes
     */
    void setCallback(WiFiEventCallback callback) { _callback = callback; }
    
    /**
     * @brief Get reconnect attempt count
     */
    uint8_t getReconnectCount() const { return _reconnectCount; }
    
    /**
     * @brief Set LED pin for status indicator
     * @param pin GPIO pin (active LOW)
     */
    void setStatusLED(int8_t pin);

private:
    String _ssid;
    String _password;
    TCWiFiState _state;
    WiFiEventCallback _callback;
    
    unsigned long _connectStartTime;    // When connection attempt started
    unsigned long _lastReconnectTime;   // Last reconnect attempt
    unsigned long _reconnectDelay;      // Current backoff delay
    uint8_t _reconnectCount;            // Number of reconnect attempts
    
    int8_t _ledPin;                     // Status LED pin (-1 = disabled)
    bool _initialized;
    
    /**
     * @brief Handle state transition
     */
    void _setState(TCWiFiState newState);
    
    /**
     * @brief Update LED based on state
     */
    void _updateLED();
    
    /**
     * @brief Calculate next backoff delay
     */
    void _calculateBackoff();
};

#endif // WIFI_MANAGER_H
