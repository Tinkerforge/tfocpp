#pragma once

#include <stdint.h>

#include "Defines.h"
#include "Types.h"
#include "Messages.h"

#include <limits>

class PeriodItem {
public:
    int32_t startPeriod;
    float limit;
    Option<int32_t> numberPhases;
};

class ChargingSchedule {
public:
    Option<int32_t> duration;
    Option<time_t> startSchedule;
    ChargingRateUnit unit;
    PeriodItem chargingSchedulePeriod[OCPP_CHARGING_SCHEDULE_MAX_PERIODS];
    size_t chargingSchedulePeriodCount;
    Option<float> minChargingRate;
};

struct EvalChargingProfileResult {
    bool applied = false;
    time_t nextCheck = std::numeric_limits<time_t>::max();
    float currentLimit = std::numeric_limits<float>::max();
    float minChargingCurrent = 0;
    int32_t numberPhases = 3;
};

class ChargingProfile {
public:
    int32_t id;
    Option<int32_t> transactionId;
    int32_t stackLevel;
    ChargingProfilePurpose chargingProfilePurpose;
    ChargingProfileKind chargingProfileKind;
    Option<RecurrencyKind> recurrencyKind;
    Option<time_t> validFrom;
    Option<time_t> validTo;
    ChargingSchedule chargingSchedule;

    ChargingProfile() {}

    ChargingProfile(SetChargingProfileCsChargingProfilesEntriesView view);

    ChargingProfile(RemoteStartTransactionChargingProfileEntriesView view);

    EvalChargingProfileResult eval(Option<time_t> startTxnTime, time_t now);
};
