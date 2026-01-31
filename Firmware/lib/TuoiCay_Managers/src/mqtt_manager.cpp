/**
 * @file mqtt_manager.cpp
 * @brief Implementation of MQTT Client Manager
 * 
 * LOGIC:
 * - Connect with LWT: devices/{deviceId}/status
 * - Exponential backoff: 2s -> 4s -> 8s -> 16s -> 30s (max)
 * - Queue messages when offline, flush on reconnect
 * - Auto-resubscribe to all topics after reconnect
 * 
 * RULES: #MQTT(9) #ERROR(6)
 */

#include "mqtt_manager.h"
#include <logger.h>
#include <error_codes.h>
#include <ArduinoJson.h>

//=============================================================================
// STATIC INSTANCE POINTER
//=============================================================================
MqttManager* MqttManager::_instance = nullptr;

//=============================================================================
// MQTT MANAGER IMPLEMENTATION
//=============================================================================

MqttManager::MqttManager()
    : _client(_wifiClient)
    , _port(1883)
    , _state(MqttState::IDLE)
    , _msgCallback(nullptr)
    , _eventCallback(nullptr)
    , _connectStartTime(0)
    , _lastReconnectTime(0)
    , _reconnectDelay(MQTT_RECONNECT_MIN_MS)
    , _reconnectCount(0)
    , _initialized(false)
    , _hasCredentials(false)
    , _subscriptionCount(0)
{
    // Initialize queue
    for (int i = 0; i < MQTT_QUEUE_SIZE; i++) {
        _queue[i].used = false;
    }
    
    // Set static instance for callback
    _instance = this;
}

bool MqttManager::begin(const char* broker, uint16_t port, const char* deviceId) {
    if (broker == nullptr || strlen(broker) == 0) {
        LOG_ERR(MOD_MQTT, "begin", "Broker is empty!");
        return false;
    }
    
    if (deviceId == nullptr || strlen(deviceId) == 0) {
        LOG_ERR(MOD_MQTT, "begin", "Device ID is empty!");
        return false;
    }
    
    _broker = broker;
    _port = port;
    _deviceId = deviceId;
    
    // Configure PubSubClient
    _client.setServer(_broker.c_str(), _port);
    _client.setCallback(_staticCallback);
    _client.setBufferSize(512);  // Larger buffer for JSON
    _client.setKeepAlive(60);    // 60 second keepalive
    
    _initialized = true;
    _state = MqttState::IDLE;
    
    LOG_INF(MOD_MQTT, "init", "MQTT ready, broker=%s:%d, deviceId=%s", 
            broker, port, deviceId);
    
    return true;
}

void MqttManager::setCredentials(const char* username, const char* password) {
    _username = username ? username : "";
    _password = password ? password : "";
    _hasCredentials = (_username.length() > 0);
    
    if (_hasCredentials) {
        LOG_DBG(MOD_MQTT, "cred", "Credentials set");
    }
}

bool MqttManager::connect() {
    if (!_initialized) {
        LOG_ERR(MOD_MQTT, "conn", "Not initialized!");
        return false;
    }
    
    // Check WiFi first
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WRN(MOD_MQTT, "conn", "WiFi not connected");
        return false;
    }
    
    if (_state == MqttState::CONNECTED) {
        LOG_WRN(MOD_MQTT, "conn", "Already connected");
        return true;
    }
    
    LOG_INF(MOD_MQTT, "conn", "Connecting to %s:%d...", _broker.c_str(), _port);
    
    // Build LWT topic: devices/{deviceId}/status
    char lwtTopic[MQTT_TOPIC_MAX_LEN];
    snprintf(lwtTopic, sizeof(lwtTopic), "devices/%s/status", _deviceId.c_str());
    
    // LWT payload (offline)
    const char* lwtPayload = "{\"online\":false}";
    
    // Client ID = deviceId
    String clientId = "TC_" + _deviceId;
    
    bool connected = false;
    if (_hasCredentials) {
        connected = _client.connect(
            clientId.c_str(),
            _username.c_str(),
            _password.c_str(),
            lwtTopic,           // LWT topic
            1,                  // LWT QoS
            true,               // LWT retain
            lwtPayload          // LWT payload
        );
    } else {
        connected = _client.connect(
            clientId.c_str(),
            lwtTopic,           // LWT topic
            1,                  // LWT QoS
            true,               // LWT retain
            lwtPayload          // LWT payload
        );
    }
    
    if (connected) {
        _reconnectCount = 0;
        _reconnectDelay = MQTT_RECONNECT_MIN_MS;
        _setState(MqttState::CONNECTED);
        
        // Publish online status
        _publishOnlineStatus();
        
        // Resubscribe to all topics
        _resubscribeAll();
        
        // Flush queued messages
        _flushQueue();
        
        LOG_INF(MOD_MQTT, "conn", "Connected! Client=%s", clientId.c_str());
        return true;
    } else {
        int state = _client.state();
        LOG_WRN(MOD_MQTT, "conn", "Connection failed, state=%d", state);
        
        _reconnectCount++;
        _calculateBackoff();
        _lastReconnectTime = millis();
        _setState(MqttState::DISCONNECTED);
        
        return false;
    }
}

void MqttManager::disconnect() {
    LOG_INF(MOD_MQTT, "disc", "Disconnecting...");
    _client.disconnect();
    _setState(MqttState::IDLE);
    _reconnectCount = 0;
    _reconnectDelay = MQTT_RECONNECT_MIN_MS;
}

void MqttManager::update() {
    if (!_initialized) return;
    
    unsigned long now = millis();
    
    // Process incoming messages if connected
    if (_client.connected()) {
        _client.loop();
        
        // Check if we think we're connected but actually disconnected
        if (_state != MqttState::CONNECTED) {
            _setState(MqttState::CONNECTED);
        }
    } else {
        // Handle disconnection
        if (_state == MqttState::CONNECTED) {
            LOG_WRN(MOD_MQTT, "conn", "Connection lost!");
            _lastReconnectTime = now;
            _setState(MqttState::DISCONNECTED);
        }
        
        // Auto-reconnect if WiFi is available
        if (_state == MqttState::DISCONNECTED && WiFi.status() == WL_CONNECTED) {
            if (now - _lastReconnectTime >= _reconnectDelay) {
                LOG_INF(MOD_MQTT, "reconn", "Attempting reconnect #%d (delay=%lums)",
                        _reconnectCount + 1, _reconnectDelay);
                connect();
            }
        }
    }
}

bool MqttManager::publish(const char* topic, const char* payload, uint8_t qos, bool retain, bool addPrefix) {
    if (!_initialized) return false;
    
    // Build full topic
    char fullTopic[MQTT_TOPIC_MAX_LEN];
    if (addPrefix) {
        snprintf(fullTopic, sizeof(fullTopic), "devices/%s/%s", _deviceId.c_str(), topic);
    } else {
        strncpy(fullTopic, topic, sizeof(fullTopic) - 1);
        fullTopic[sizeof(fullTopic) - 1] = '\0';
    }
    
    // If connected, publish directly
    if (_client.connected()) {
        bool result = _client.publish(fullTopic, payload, retain);
        if (result) {
            LOG_DBG(MOD_MQTT, "pub", "-> %s", fullTopic);
        } else {
            LOG_WRN(MOD_MQTT, "pub", "FAILED: %s", fullTopic);
        }
        return result;
    }
    
    // If disconnected, queue message (only QoS > 0 messages)
    if (qos > 0) {
        return _queueMessage(fullTopic, payload, qos, retain);
    }
    
    // QoS 0 messages are dropped when offline
    LOG_DBG(MOD_MQTT, "pub", "Dropped (offline, QoS=0): %s", fullTopic);
    return false;
}

bool MqttManager::subscribe(const char* topic, uint8_t qos, bool addPrefix) {
    if (!_initialized) return false;
    
    // Build full topic
    char fullTopic[MQTT_TOPIC_MAX_LEN];
    if (addPrefix) {
        snprintf(fullTopic, sizeof(fullTopic), "devices/%s/%s", _deviceId.c_str(), topic);
    } else {
        strncpy(fullTopic, topic, sizeof(fullTopic) - 1);
        fullTopic[sizeof(fullTopic) - 1] = '\0';
    }
    
    // Save subscription for resubscribe
    if (_subscriptionCount < MAX_SUBSCRIPTIONS) {
        bool alreadyExists = false;
        for (uint8_t i = 0; i < _subscriptionCount; i++) {
            if (_subscriptions[i] == fullTopic) {
                alreadyExists = true;
                _subscriptionQos[i] = qos;
                break;
            }
        }
        if (!alreadyExists) {
            _subscriptions[_subscriptionCount] = fullTopic;
            _subscriptionQos[_subscriptionCount] = qos;
            _subscriptionCount++;
        }
    }
    
    // If connected, subscribe now
    if (_client.connected()) {
        bool result = _client.subscribe(fullTopic, qos);
        if (result) {
            LOG_INF(MOD_MQTT, "sub", "<- %s (QoS=%d)", fullTopic, qos);
        } else {
            LOG_WRN(MOD_MQTT, "sub", "FAILED: %s", fullTopic);
        }
        return result;
    }
    
    // Will subscribe when connected
    LOG_DBG(MOD_MQTT, "sub", "Queued: %s", fullTopic);
    return true;
}

bool MqttManager::isConnected() const {
    return _state == MqttState::CONNECTED;
}

const char* MqttManager::getStateString() const {
    switch (_state) {
        case MqttState::IDLE:         return "IDLE";
        case MqttState::CONNECTING:   return "CONNECTING";
        case MqttState::CONNECTED:    return "CONNECTED";
        case MqttState::DISCONNECTED: return "DISCONNECTED";
        default:                      return "UNKNOWN";
    }
}

uint8_t MqttManager::getQueuedCount() const {
    uint8_t count = 0;
    for (int i = 0; i < MQTT_QUEUE_SIZE; i++) {
        if (_queue[i].used) count++;
    }
    return count;
}

void MqttManager::buildTopic(const char* topic, char* buffer, size_t bufSize) {
    snprintf(buffer, bufSize, "devices/%s/%s", _deviceId.c_str(), topic);
}

//=============================================================================
// PRIVATE METHODS
//=============================================================================

void MqttManager::_setState(MqttState newState) {
    if (_state != newState) {
        _state = newState;
        
        LOG_DBG(MOD_MQTT, "state", "-> %s", getStateString());
        
        if (_eventCallback) {
            _eventCallback(newState);
        }
    }
}

void MqttManager::_calculateBackoff() {
    // Exponential backoff: 2s -> 4s -> 8s -> 16s -> 30s (max)
    _reconnectDelay *= 2;
    if (_reconnectDelay > MQTT_RECONNECT_MAX_MS) {
        _reconnectDelay = MQTT_RECONNECT_MAX_MS;
    }
}

bool MqttManager::_queueMessage(const char* topic, const char* payload, uint8_t qos, bool retain) {
    // Find empty slot
    for (int i = 0; i < MQTT_QUEUE_SIZE; i++) {
        if (!_queue[i].used) {
            strncpy(_queue[i].topic, topic, MQTT_TOPIC_MAX_LEN - 1);
            _queue[i].topic[MQTT_TOPIC_MAX_LEN - 1] = '\0';
            
            strncpy(_queue[i].payload, payload, MQTT_PAYLOAD_MAX - 1);
            _queue[i].payload[MQTT_PAYLOAD_MAX - 1] = '\0';
            
            _queue[i].qos = qos;
            _queue[i].retain = retain;
            _queue[i].used = true;
            
            LOG_DBG(MOD_MQTT, "queue", "Queued [%d]: %s", i, topic);
            return true;
        }
    }
    
    LOG_WRN(MOD_MQTT, "queue", "Queue full! Dropping: %s", topic);
    return false;
}

void MqttManager::_flushQueue() {
    uint8_t flushed = 0;
    
    for (int i = 0; i < MQTT_QUEUE_SIZE; i++) {
        if (_queue[i].used) {
            if (_client.publish(_queue[i].topic, _queue[i].payload, _queue[i].retain)) {
                LOG_DBG(MOD_MQTT, "flush", "Sent [%d]: %s", i, _queue[i].topic);
                _queue[i].used = false;
                flushed++;
            } else {
                LOG_WRN(MOD_MQTT, "flush", "Failed [%d]: %s", i, _queue[i].topic);
            }
        }
    }
    
    if (flushed > 0) {
        LOG_INF(MOD_MQTT, "flush", "Flushed %d queued messages", flushed);
    }
}

void MqttManager::_resubscribeAll() {
    for (uint8_t i = 0; i < _subscriptionCount; i++) {
        _client.subscribe(_subscriptions[i].c_str(), _subscriptionQos[i]);
        LOG_DBG(MOD_MQTT, "resub", "<- %s", _subscriptions[i].c_str());
    }
    
    if (_subscriptionCount > 0) {
        LOG_INF(MOD_MQTT, "resub", "Resubscribed to %d topics", _subscriptionCount);
    }
}

void MqttManager::_publishOnlineStatus() {
    // Build online status JSON
    JsonDocument doc;
    doc["online"] = true;
    doc["ip"] = WiFi.localIP().toString();
    doc["fw"] = FW_VERSION;
    doc["rssi"] = WiFi.RSSI();
    
    char payload[128];
    serializeJson(doc, payload, sizeof(payload));
    
    // Publish to status topic (with retain)
    char topic[MQTT_TOPIC_MAX_LEN];
    snprintf(topic, sizeof(topic), "devices/%s/status", _deviceId.c_str());
    
    _client.publish(topic, payload, true);  // retain=true
    LOG_DBG(MOD_MQTT, "lwt", "Online status published");
}

void MqttManager::_staticCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (_instance && _instance->_msgCallback) {
        _instance->_msgCallback(topic, payload, length);
    }
}
