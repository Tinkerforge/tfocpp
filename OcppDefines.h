#pragma once

#define NUM_CONNECTORS 1

// Default configuration

#define DEFAULT_ALLOW_OFFLINE_TX_FOR_UNKNOWN_ID false
#define DEFAULT_AUTHORIZATION_CACHE_ENABLED false
#define DEFAULT_AUTHORIZE_REMOTE_TX_REQUESTS false
#define DEFAULT_BLINK_REPEAT 3
#define DEFAULT_CLOCK_ALIGNED_DATA_INTERVAL 0
#define DEFAULT_CONNECTION_TIME_OUT 60
#define DEFAULT_CONNECTOR_PHASE_ROTATION "1.Unknown"

#define DEFAULT_HEARTBEAT_INTERVAL_S 60
#define DEFAULT_LIGHT_INTENSITY 100
#define DEFAULT_LOCAL_AUTHORIZE_OFFLINE false
#define DEFAULT_LOCAL_PRE_AUTHORIZE false
#define DEFAULT_MAX_ENERGY_ON_INVALID_ID 0
#define DEFAULT_MESSAGE_TIMEOUT 10
#define DEFAULT_METER_VALUES_ALIGNED_DATA ""
#define METER_VALUES_ALIGNED_DATA_MAX_LENGTH 5
#define DEFAULT_METER_VALUES_SAMPLED_DATA ""
#define METER_VALUES_SAMPLED_DATA_MAX_LENGTH 5
#define DEFAULT_METER_VALUE_SAMPLE_INTERVAL 0
#define DEFAULT_MINIMUM_STATUS_DURATION 1
#define DEFAULT_RESET_RETRIES 1
#define DEFAULT_STOP_TRANSACTION_ON_EV_SIDE_DISCONNECT false
#define DEFAULT_STOP_TRANSACTION_ON_INVALID_ID false
#define STOP_TRANSACTION_MAX_METER_VALUES 2
//#define DEFAULT_STOP_TXN_ALIGNED_DATA ""
//#define STOP_TXN_ALIGNED_DATA_MAX_LENGTH 0
//#define DEFAULT_STOP_TXN_SAMPLED_DATA ""
//#define STOP_TXN_SAMPLED_DATA_MAX_LENGTH 0
#define SUPPORTED_FEATURE_PROFILES "Core,SmartCharging"
//#define SUPPORTED_FEATURE_PROFILES_MAX_LENGTH
#define DEFAULT_TRANSACTION_MESSAGE_ATTEMPTS 3
#define DEFAULT_TRANSACTION_MESSAGE_RETRY_INTERVAL 10
#define DEFAULT_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT false
#define DEFAULT_WEB_SOCKET_PING_INTERVAL 10

//#define DEFAULT_LOCAL_AUTH_LIST_ENABLED
//#define DEFAULT_LOCAL_AUTH_LIST_MAX_LENGTH
//#define DEFAULT_SEND_LOCAL_LIST_MAX_LENGTH

//#define DEFAULT_RESERVE_CONNECTOR_ZERO_SUPPORTED

#define CHARGE_PROFILE_MAX_STACK_LEVEL 3
#define CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT "Current"
#define CHARGING_SCHEDULE_MAX_PERIODS 3
#define CONNECTOR_SWITCH3TO1_PHASE_SUPPORTED false
#define MAX_CHARGING_PROFILES_INSTALLED 3
