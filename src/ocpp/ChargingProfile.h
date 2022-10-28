#pragma once

#include <stdint.h>

#include "Defines.h"
#include "Tools.h"

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
    PeriodItem chargingSchedulePeriod[CHARGING_SCHEDULE_MAX_PERIODS];
    size_t chargingSchedulePeriodCount;
    Opt<float> minChargingRate;
};

struct EvalChargingProfileResult {
    bool applied = false;
    time_t nextCheck = std::numeric_limits<time_t>::max();
    float limit = std::numeric_limits<float>::max();
    float minChargingRate = 0;
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

    ChargingProfile(SetChargingProfileCsChargingProfilesEntriesView view) {
        this->id = view.chargingProfileId();
        this->transactionId = view.transactionId();
        this->stackLevel = view.stackLevel();
        this->chargingProfilePurpose = view.chargingProfilePurpose();
        this->chargingProfileKind = view.chargingProfileKind();
        this->recurrencyKind = view.recurrencyKind();
        this->validFrom = view.validFrom();
        this->validTo = view.validTo();
        this->chargingSchedule.duration = view.chargingSchedule().duration();
        this->chargingSchedule.startSchedule = view.chargingSchedule().startSchedule();
        this->chargingSchedule.unit = view.chargingSchedule().chargingRateUnit();
        this->chargingSchedule.chargingSchedulePeriodCount = view.chargingSchedule().chargingSchedulePeriod_count();
        for (int i = 0; i < view.chargingSchedule().chargingSchedulePeriod_count(); ++i) {
            this->chargingSchedule.chargingSchedulePeriod[i].startPeriod = view.chargingSchedule().chargingSchedulePeriod(i).startPeriod();
            this->chargingSchedule.chargingSchedulePeriod[i].limit = view.chargingSchedule().chargingSchedulePeriod(i).limit();
            this->chargingSchedule.chargingSchedulePeriod[i].numberPhases = view.chargingSchedule().chargingSchedulePeriod(i).numberPhases();
        }
        this->chargingSchedule.minChargingRate = view.chargingSchedule().minChargingRate();
    }

    EvalChargingProfileResult eval(Opt<time_t> startTxnTime, time_t now) {
        EvalChargingProfileResult result;

        /*
        At any point in time
        the prevailing charging profile SHALL be the charging profile with the highest stackLevel among the profiles that
        are valid at that point in time, as determined by their validFrom and validTo parameters.
        */
        if (this->validFrom.is_set() && this->validFrom.get() > now) {
            // This profile is not yet valid. Skip it and note that we have to check again if it becomes valid;
            result.nextCheck = this->validFrom.get();
            return result;
        }

        if (this->validTo.is_set() && this->validTo.get() < now) {
            // This profile is not valid anymore.
            return result;
        }

        if (this->validTo.is_set())
            result.nextCheck = std::min(result.nextCheck, this->validTo.get());

        auto &sched = this->chargingSchedule;

        time_t schedStart;

        if (sched.startSchedule.is_set() &&
            /* Schedule periods are relative to a fixed point in time defined in the schedule. */
            (this->chargingProfileKind == ChargingProfileKind::ABSOLUTE ||
             /* The schedule restarts periodically at the first schedule period. */
             this->chargingProfileKind == ChargingProfileKind::RECURRING)) {
            schedStart = sched.startSchedule.get();
        } else {
            // Either this is a relative charging profile, or startSchedule is missing:
            /*  If absent the schedule will be relative to start of charging. */

            /* Schedule periods are relative to a situation-specific start point (such as the start of a Transaction) that is determined by the charge point. */
            if (!startTxnTime.is_set()) {
                // No transaction is running -> this profile does not apply.
                // TODO: Make sure this is correct.
                //       If for example we want to limit the current permanently,
                //       we could set a ChargePointMaxProfile of kind Recurring without start?
                return result;
            }

            /* Schedule periods are relative to a situation-specific start point (such as the start of a Transaction) that is determined by the charge point. */
            schedStart = startTxnTime.get();
        }

        if (now < schedStart) {
            // This schedule does not apply yet.
            result.nextCheck = std::min(result.nextCheck, schedStart);
            return result;
        }

        if (this->chargingProfileKind == ChargingProfileKind::RECURRING && this->recurrencyKind.is_set()) {
            // If this is a recurring schedule, move the schedStart forward until schedStart <= time <= schedStart + recurrenceKind is true
            auto blockSize = this->recurrencyKind.get() == RecurrencyKind::WEEKLY ? (7 * 24 * 60 * 60) : (24 * 60 * 60);
            schedStart = now - now % blockSize + (schedStart % blockSize);

            // We have to check again when the next block starts.
            result.nextCheck = std::min(result.nextCheck, schedStart + blockSize);
        }

        if (sched.duration.is_set()) {
            if (now >= (schedStart + sched.duration.get()))
                return result;

            // We have to check again when the duration elapses.
            result.nextCheck = std::min(result.nextCheck, schedStart + sched.duration.get());
        }
        /* If the
           duration is left empty, the last period will continue indefinitely or
           until end of the transaction in case startSchedule is absent. */

        auto &period = sched.chargingSchedulePeriod[0];

        for(size_t i = 1; i < sched.chargingSchedulePeriodCount; ++i) {
            if (schedStart + sched.chargingSchedulePeriod[i].startPeriod > now) {
                result.nextCheck = std::min(result.nextCheck, schedStart + sched.chargingSchedulePeriod[i].startPeriod);
                break;
            }
            period = sched.chargingSchedulePeriod[i];
        }

        result.applied = true;
        result.limit = period.limit;
        if (period.numberPhases.is_set())
            result.numberPhases = period.numberPhases.get();
        if (sched.minChargingRate.is_set())
            result.minChargingRate = sched.minChargingRate.get();

        return result;
    }
};
