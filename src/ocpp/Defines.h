#pragma once

#ifndef OCPP_NUM_CONNECTORS
#define OCPP_NUM_CONNECTORS 1
#endif

#ifndef OCPP_TRANSACTION_RELATED_MESSAGE_QUEUE_SIZE
#define OCPP_TRANSACTION_RELATED_MESSAGE_QUEUE_SIZE 5
#endif

#ifndef OCPP_CURRENT_REQUIRED_TO_START_CHARGING
#define OCPP_CURRENT_REQUIRED_TO_START_CHARGING 6
#endif

#ifndef OCPP_LINE_VOLTAGE
#define OCPP_LINE_VOLTAGE 230
#endif

#ifndef OCPP_MAX_CHARGING_CURRENT
#define OCPP_MAX_CHARGING_CURRENT 0xFFFFFFFF
#endif

#ifndef OCPP_RECONNECT_WEBSOCKET_INTERVAL_S
#define OCPP_RECONNECT_WEBSOCKET_INTERVAL_S 60
#endif

// Default configuration

#ifndef OCPP_DEFAULT_ALLOW_OFFLINE_TX_FOR_UNKNOWN_ID
#define OCPP_DEFAULT_ALLOW_OFFLINE_TX_FOR_UNKNOWN_ID false
#endif

#ifndef OCPP_DEFAULT_AUTHORIZATION_CACHE_ENABLED
#define OCPP_DEFAULT_AUTHORIZATION_CACHE_ENABLED false
#endif

#ifndef OCPP_DEFAULT_AUTHORIZE_REMOTE_TX_REQUESTS
#define OCPP_DEFAULT_AUTHORIZE_REMOTE_TX_REQUESTS false
#endif

#ifndef OCPP_DEFAULT_BLINK_REPEAT
#define OCPP_DEFAULT_BLINK_REPEAT 3
#endif

#ifndef OCPP_DEFAULT_CLOCK_ALIGNED_DATA_INTERVAL
#define OCPP_DEFAULT_CLOCK_ALIGNED_DATA_INTERVAL 0
#endif

#ifndef OCPP_DEFAULT_CONNECTION_TIME_OUT
#define OCPP_DEFAULT_CONNECTION_TIME_OUT 60
#endif

#ifndef OCPP_DEFAULT_CONNECTOR_PHASE_ROTATION
#define OCPP_DEFAULT_CONNECTOR_PHASE_ROTATION "1.Unknown"
#endif


#ifndef OCPP_DEFAULT_HEARTBEAT_INTERVAL_S
#define OCPP_DEFAULT_HEARTBEAT_INTERVAL_S 60
#endif

#ifndef OCPP_DEFAULT_LIGHT_INTENSITY
#define OCPP_DEFAULT_LIGHT_INTENSITY 100
#endif

#ifndef OCPP_DEFAULT_LOCAL_AUTHORIZE_OFFLINE
#define OCPP_DEFAULT_LOCAL_AUTHORIZE_OFFLINE false
#endif

#ifndef OCPP_DEFAULT_LOCAL_PRE_AUTHORIZE
#define OCPP_DEFAULT_LOCAL_PRE_AUTHORIZE false
#endif

#ifndef OCPP_DEFAULT_MAX_ENERGY_ON_INVALID_ID
#define OCPP_DEFAULT_MAX_ENERGY_ON_INVALID_ID 0
#endif

#ifndef OCPP_DEFAULT_MESSAGE_TIMEOUT
#define OCPP_DEFAULT_MESSAGE_TIMEOUT 10
#endif

#ifndef OCPP_DEFAULT_METER_VALUES_ALIGNED_DATA
#define OCPP_DEFAULT_METER_VALUES_ALIGNED_DATA ""
#endif

#ifndef OCPP_METER_VALUES_ALIGNED_DATA_MAX_LENGTH
#define OCPP_METER_VALUES_ALIGNED_DATA_MAX_LENGTH 5
#endif

#ifndef OCPP_DEFAULT_METER_VALUES_SAMPLED_DATA
#define OCPP_DEFAULT_METER_VALUES_SAMPLED_DATA ""
#endif

#ifndef OCPP_METER_VALUES_SAMPLED_DATA_MAX_LENGTH
#define OCPP_METER_VALUES_SAMPLED_DATA_MAX_LENGTH 5
#endif

#ifndef OCPP_DEFAULT_METER_VALUE_SAMPLE_INTERVAL
#define OCPP_DEFAULT_METER_VALUE_SAMPLE_INTERVAL 0
#endif

#ifndef OCPP_DEFAULT_MINIMUM_STATUS_DURATION
#define OCPP_DEFAULT_MINIMUM_STATUS_DURATION 1
#endif

#ifndef OCPP_DEFAULT_RESET_RETRIES
#define OCPP_DEFAULT_RESET_RETRIES 1
#endif

#ifndef OCPP_DEFAULT_STOP_TRANSACTION_ON_INVALID_ID
#define OCPP_DEFAULT_STOP_TRANSACTION_ON_INVALID_ID false
#endif

#ifndef OCPP_STOP_TRANSACTION_MAX_METER_VALUES
#define OCPP_STOP_TRANSACTION_MAX_METER_VALUES 2
#endif

//#ifndef OCPP_DEFAULT_STOP_TXN_ALIGNED_DATA
//#define OCPP_DEFAULT_STOP_TXN_ALIGNED_DATA ""
//#endif

//#ifndef OCPP_STOP_TXN_ALIGNED_DATA_MAX_LENGTH
//#define OCPP_STOP_TXN_ALIGNED_DATA_MAX_LENGTH 0
//#endif

//#ifndef OCPP_DEFAULT_STOP_TXN_SAMPLED_DATA
//#define OCPP_DEFAULT_STOP_TXN_SAMPLED_DATA ""
//#endif

//#ifndef OCPP_STOP_TXN_SAMPLED_DATA_MAX_LENGTH
//#define OCPP_STOP_TXN_SAMPLED_DATA_MAX_LENGTH 0
//#endif

#ifndef OCPP_SUPPORTED_FEATURE_PROFILES
#define OCPP_SUPPORTED_FEATURE_PROFILES "Core,SmartCharging"
#endif

//#ifndef OCPP_SUPPORTED_FEATURE_PROFILES_MAX_LENGTH
//#define OCPP_SUPPORTED_FEATURE_PROFILES_MAX_LENGTH
//#endif

#ifndef OCPP_DEFAULT_TRANSACTION_MESSAGE_ATTEMPTS
#define OCPP_DEFAULT_TRANSACTION_MESSAGE_ATTEMPTS 3
#endif

#ifndef OCPP_DEFAULT_TRANSACTION_MESSAGE_RETRY_INTERVAL
#define OCPP_DEFAULT_TRANSACTION_MESSAGE_RETRY_INTERVAL 10
#endif

#ifndef OCPP_DEFAULT_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT
#define OCPP_DEFAULT_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT false
#endif

#ifndef OCPP_DEFAULT_WEB_SOCKET_PING_INTERVAL
#define OCPP_DEFAULT_WEB_SOCKET_PING_INTERVAL 10
#endif

#ifndef OCPP_WEBSOCKET_PING_PONG_TIMEOUT
#define OCPP_WEBSOCKET_PING_PONG_TIMEOUT (OCPP_DEFAULT_WEB_SOCKET_PING_INTERVAL * 3 + OCPP_DEFAULT_WEB_SOCKET_PING_INTERVAL / 2)
#endif

//#ifndef OCPP_DEFAULT_LOCAL_AUTH_LIST_ENABLED
//#define OCPP_DEFAULT_LOCAL_AUTH_LIST_ENABLED
//#endif

//#ifndef OCPP_DEFAULT_LOCAL_AUTH_LIST_MAX_LENGTH
//#define OCPP_DEFAULT_LOCAL_AUTH_LIST_MAX_LENGTH
//#endif

//#ifndef OCPP_DEFAULT_SEND_LOCAL_LIST_MAX_LENGTH
//#define OCPP_DEFAULT_SEND_LOCAL_LIST_MAX_LENGTH
//#endif


//#ifndef OCPP_DEFAULT_RESERVE_CONNECTOR_ZERO_SUPPORTED
//#define OCPP_DEFAULT_RESERVE_CONNECTOR_ZERO_SUPPORTED
//#endif


#ifndef OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL
#define OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL 3
#endif

#ifndef OCPP_CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT
#define OCPP_CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT "Current"
#endif

#ifndef OCPP_CHARGING_SCHEDULE_MAX_PERIODS
#define OCPP_CHARGING_SCHEDULE_MAX_PERIODS 5
#endif

#ifndef OCPP_GET_COMPOSITE_SCHEDULE_MAX_PERIODS
#define OCPP_GET_COMPOSITE_SCHEDULE_MAX_PERIODS (OCPP_CHARGING_SCHEDULE_MAX_PERIODS * OCPP_NUM_CONNECTORS * OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL * 2)
#endif

#ifndef OCPP_CONNECTOR_SWITCH3TO1_PHASE_SUPPORTED
#define OCPP_CONNECTOR_SWITCH3TO1_PHASE_SUPPORTED false
#endif

#ifndef OCPP_MAX_CHARGING_PROFILES_INSTALLED
#define OCPP_MAX_CHARGING_PROFILES_INSTALLED ((OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL + 1) * 3)
#endif


#ifndef OCPP_AUTHORIZE_REMOTE_TX_REQUESTS_REQUIRES_REBOOT
#define OCPP_AUTHORIZE_REMOTE_TX_REQUESTS_REQUIRES_REBOOT false
#endif

#ifndef OCPP_CLOCK_ALIGNED_DATA_INTERVAL_REQUIRES_REBOOT
#define OCPP_CLOCK_ALIGNED_DATA_INTERVAL_REQUIRES_REBOOT false
#endif

#ifndef OCPP_CONNECTION_TIME_OUT_REQUIRES_REBOOT
#define OCPP_CONNECTION_TIME_OUT_REQUIRES_REBOOT false
#endif

#ifndef OCPP_CONNECTOR_PHASE_ROTATION_REQUIRES_REBOOT
#define OCPP_CONNECTOR_PHASE_ROTATION_REQUIRES_REBOOT false
#endif

#ifndef OCPP_HEARTBEAT_INTERVAL_S_REQUIRES_REBOOT
#define OCPP_HEARTBEAT_INTERVAL_S_REQUIRES_REBOOT false
#endif

#ifndef OCPP_LIGHT_INTENSITY_REQUIRES_REBOOT
#define OCPP_LIGHT_INTENSITY_REQUIRES_REBOOT false
#endif

#ifndef OCPP_LOCAL_AUTHORIZE_OFFLINE_REQUIRES_REBOOT
#define OCPP_LOCAL_AUTHORIZE_OFFLINE_REQUIRES_REBOOT false
#endif

#ifndef OCPP_LOCAL_PRE_AUTHORIZE_REQUIRES_REBOOT
#define OCPP_LOCAL_PRE_AUTHORIZE_REQUIRES_REBOOT false
#endif

#ifndef OCPP_MAX_ENERGY_ON_INVALID_ID_REQUIRES_REBOOT
#define OCPP_MAX_ENERGY_ON_INVALID_ID_REQUIRES_REBOOT false
#endif

#ifndef OCPP_MESSAGE_TIMEOUT_REQUIRES_REBOOT
#define OCPP_MESSAGE_TIMEOUT_REQUIRES_REBOOT false
#endif

/*
#ifndef OCPP_METER_VALUES_ALIGNED_DATA_REQUIRES_REBOOT
#define OCPP_METER_VALUES_ALIGNED_DATA_REQUIRES_REBOOT true
#endif

#ifndef OCPP_METER_VALUES_SAMPLED_DATA_REQUIRES_REBOOT
#define OCPP_METER_VALUES_SAMPLED_DATA_REQUIRES_REBOOT true
#endif
*/
#ifndef OCPP_METER_VALUE_SAMPLE_INTERVAL_REQUIRES_REBOOT
#define OCPP_METER_VALUE_SAMPLE_INTERVAL_REQUIRES_REBOOT false
#endif

#ifndef OCPP_MINIMUM_STATUS_DURATION_REQUIRES_REBOOT
#define OCPP_MINIMUM_STATUS_DURATION_REQUIRES_REBOOT false
#endif

#ifndef OCPP_RESET_RETRIES_REQUIRES_REBOOT
#define OCPP_RESET_RETRIES_REQUIRES_REBOOT false
#endif

#ifndef OCPP_STOP_TRANSACTION_ON_INVALID_ID_REQUIRES_REBOOT
#define OCPP_STOP_TRANSACTION_ON_INVALID_ID_REQUIRES_REBOOT false
#endif

#ifndef STOP_TXN_ALIGNED_DATA_REQUIRES_REBOOT
#define STOP_TXN_ALIGNED_DATA_REQUIRES_REBOOT false
#endif

#ifndef STOP_TXN_SAMPLED_DATA_REQUIRES_REBOOT
#define STOP_TXN_SAMPLED_DATA_REQUIRES_REBOOT false
#endif

#ifndef OCPP_TRANSACTION_MESSAGE_ATTEMPTS_REQUIRES_REBOOT
#define OCPP_TRANSACTION_MESSAGE_ATTEMPTS_REQUIRES_REBOOT false
#endif

#ifndef OCPP_TRANSACTION_MESSAGE_RETRY_INTERVAL_REQUIRES_REBOOT
#define OCPP_TRANSACTION_MESSAGE_RETRY_INTERVAL_REQUIRES_REBOOT false
#endif

#ifndef OCPP_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT_REQUIRES_REBOOT
#define OCPP_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT_REQUIRES_REBOOT false
#endif

#ifndef OCPP_WEB_SOCKET_PING_INTERVAL_REQUIRES_REBOOT
#define OCPP_WEB_SOCKET_PING_INTERVAL_REQUIRES_REBOOT false
#endif
