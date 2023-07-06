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
    Opt<int32_t> numberPhases;
};

class ChargingSchedule {
public:
    Opt<int32_t> duration;
    Opt<time_t> startSchedule;
    ChargingRateUnit unit;
    PeriodItem chargingSchedulePeriod[OCPP_CHARGING_SCHEDULE_MAX_PERIODS];
    size_t chargingSchedulePeriodCount;
    Opt<float> minChargingRate;
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
    Opt<int32_t> transactionId;
    int32_t stackLevel;
    ChargingProfilePurpose chargingProfilePurpose;
    ChargingProfileKind chargingProfileKind;
    Opt<RecurrencyKind> recurrencyKind;
    Opt<time_t> validFrom;
    Opt<time_t> validTo;
    ChargingSchedule chargingSchedule;

    ChargingProfile() {}

    ChargingProfile(SetChargingProfileCsChargingProfilesEntriesView view);

    ChargingProfile(RemoteStartTransactionChargingProfileEntriesView view);

    EvalChargingProfileResult eval(Opt<time_t> startTxnTime, time_t now);
};
