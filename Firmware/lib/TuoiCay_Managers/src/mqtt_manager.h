/**
 * @file mqtt_manager.h
 * @brief MQTT Client Manager with auto-reconnect, LWT and offline queue
 * 
 * LOGIC:
 * - Connect to broker with LWT (Last Will Testament)
 * - Auto-reconnect with exponential backoff
 * - Offline message queue (max 10 messages)
 * - QoS support for publish/subscribe
 * 
 * RULES: #MQTT(9) #ERROR(6)
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <config.h>

//=============================================================================
// MQTT STATE ENUM
//=============================================================================
enum class MqttState : uint8_t {
    IDLE = 0,           // Not started
    CONNECTING = 1,     // Attempting connection
    CONNECTED = 2,      // Connected to broker
    DISCONNECTED = 3    // Was connected, now disconnected
};

//=============================================================================
// OFFLINE MESSAGE QUEUE
//=============================================================================
#define MQTT_QUEUE_SIZE     10      // Max messages in offline queue
#define MQTT_TOPIC_MAX_LEN  64      // Max topic length
#define MQTT_PAYLOAD_MAX    256     // Max payload length

struct QueuedMessage {
    char topic[MQTT_TOPIC_MAX_LEN];
    char payload[MQTT_PAYLOAD_MAX];
    uint8_t qos;
    bool retain;
    bool used;
};

//=============================================================================
// CALLBACK TYPES
//=============================================================================
typedef void (*MqttMessageCallback)(const char* topic, const uint8_t* payload, unsigned int length);
typedef void (*MqttEventCallback)(MqttState newState);

//=============================================================================
// MQTT MANAGER CLASS
//=============================================================================

/**
 * @class MqttManager
 * @brief Manages MQTT connection with auto-reconnect and offline queue
 */
class MqttManager {
public:
    /**
     * @brief Constructor
     */
    MqttManager();
    
    /**
     * @brief Initialize MQTT manager
     * @param broker Broker hostname/IP
     * @param port Broker port (default 1883)
     * @param deviceId Device ID for topics (MAC address)
     * @return true if parameters valid
     */
    bool begin(const char* broker, uint16_t port, const char* deviceId);
    
    /**
     * @brief Set credentials for authenticated brokers
     * @param username MQTT username
     * @param password MQTT password
     */
    void setCredentials(const char* username, const char* password);
    
    /**
     * @brief Start MQTT connection (non-blocking)
     * @return true if connection attempt started
     */
    bool connect();
    
    /**
     * @brief Disconnect from broker
     */
    void disconnect();
    
    /**
     * @brief Update MQTT state (call in loop)
     * Handles:
     * - Connection timeout
     * - Reconnection with exponential backoff
     * - Process incoming messages
     * - Flush offline queue when connected
     */
    void update();
    
    /**
     * @brief Alias for update() - compatibility with PubSubClient
     */
    void loop() { update(); }
    
    /**
     * @brief Publish message to topic
     * @param topic Topic string (without deviceId prefix)
     * @param payload Message payload
     * @param qos QoS level (0, 1, or 2)
     * @param retain Retain flag
     * @param addPrefix Add deviceId prefix to topic
     * @return true if published or queued (if offline)
     */
    bool publish(const char* topic, const char* payload, uint8_t qos = 0, bool retain = false, bool addPrefix = true);
    
    /**
     * @brief Subscribe to topic
     * @param topic Topic string (without deviceId prefix)
     * @param qos QoS level
     * @param addPrefix Add deviceId prefix to topic
     * @return true if subscribed successfully
     */
    bool subscribe(const char* topic, uint8_t qos = 0, bool addPrefix = true);
    
    /**
     * @brief Check if connected to broker
     */
    bool isConnected() const;
    
    /**
     * @brief Get current state
     */
    MqttState getState() const { return _state; }
    
    /**
     * @brief Get state as string
     */
    const char* getStateString() const;
    
    /**
     * @brief Get device ID
     */
    const char* getDeviceId() const { return _deviceId.c_str(); }
    
    /**
     * @brief Get number of queued messages
     */
    uint8_t getQueuedCount() const;
    
    /**
     * @brief Set callback for incoming messages
     */
    void setMessageCallback(MqttMessageCallback callback) { _msgCallback = callback; }
    
    /**
     * @brief Set callback for state changes
     */
    void setEventCallback(MqttEventCallback callback) { _eventCallback = callback; }
    
    /**
     * @brief Get reconnect attempt count
     */
    uint8_t getReconnectCount() const { return _reconnectCount; }
    
    /**
     * @brief Get PubSubClient for advanced usage
     */
    PubSubClient& getClient() { return _client; }

    /**
     * @brief Build full topic with device prefix
     * @param topic Topic suffix
     * @param buffer Output buffer
     * @param bufSize Buffer size
     */
    void buildTopic(const char* topic, char* buffer, size_t bufSize);

private:
    WiFiClient _wifiClient;
    PubSubClient _client;
    
    String _broker;
    uint16_t _port;
    String _deviceId;
    String _username;
    String _password;
    
    MqttState _state;
    MqttMessageCallback _msgCallback;
    MqttEventCallback _eventCallback;
    
    unsigned long _connectStartTime;    // When connection attempt started
    unsigned long _lastReconnectTime;   // Last reconnect attempt
    unsigned long _reconnectDelay;      // Current backoff delay
    uint8_t _reconnectCount;            // Number of reconnect attempts
    
    bool _initialized;
    bool _hasCredentials;
    
    // Offline message queue
    QueuedMessage _queue[MQTT_QUEUE_SIZE];
    
    // Topics to resubscribe after reconnect
    static const uint8_t MAX_SUBSCRIPTIONS = 10;
    String _subscriptions[MAX_SUBSCRIPTIONS];
    uint8_t _subscriptionQos[MAX_SUBSCRIPTIONS];
    uint8_t _subscriptionCount;
    
    /**
     * @brief Handle state transition
     */
    void _setState(MqttState newState);
    
    /**
     * @brief Calculate next backoff delay
     */
    void _calculateBackoff();
    
    /**
     * @brief Queue message for later sending
     */
    bool _queueMessage(const char* topic, const char* payload, uint8_t qos, bool retain);
    
    /**
     * @brief Flush queued messages
     */
    void _flushQueue();
    
    /**
     * @brief Resubscribe to all topics after reconnect
     */
    void _resubscribeAll();
    
    /**
     * @brief Publish LWT online status
     */
    void _publishOnlineStatus();
    
    /**
     * @brief Static callback wrapper for PubSubClient
     */
    static void _staticCallback(char* topic, uint8_t* payload, unsigned int length);
    
    /**
     * @brief Pointer to current instance for static callback
     */
    static MqttManager* _instance;
};

#endif // MQTT_MANAGER_H
