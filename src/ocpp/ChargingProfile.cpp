#include "ChargingProfile.h"

#include "Platform.h"

ChargingProfile::ChargingProfile(SetChargingProfileCsChargingProfilesEntriesView view) {
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
    for (size_t i = 0; i < view.chargingSchedule().chargingSchedulePeriod_count(); ++i) {
        this->chargingSchedule.chargingSchedulePeriod[i].startPeriod = view.chargingSchedule().chargingSchedulePeriod(i).startPeriod();
        this->chargingSchedule.chargingSchedulePeriod[i].limit = view.chargingSchedule().chargingSchedulePeriod(i).limit();
        this->chargingSchedule.chargingSchedulePeriod[i].numberPhases = view.chargingSchedule().chargingSchedulePeriod(i).numberPhases();
    }
    this->chargingSchedule.minChargingRate = view.chargingSchedule().minChargingRate();
}

ChargingProfile::ChargingProfile(RemoteStartTransactionChargingProfileEntriesView view) {
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
    for (size_t i = 0; i < view.chargingSchedule().chargingSchedulePeriod_count(); ++i) {
        this->chargingSchedule.chargingSchedulePeriod[i].startPeriod = view.chargingSchedule().chargingSchedulePeriod(i).startPeriod();
        this->chargingSchedule.chargingSchedulePeriod[i].limit = view.chargingSchedule().chargingSchedulePeriod(i).limit();
        this->chargingSchedule.chargingSchedulePeriod[i].numberPhases = view.chargingSchedule().chargingSchedulePeriod(i).numberPhases();
    }
    this->chargingSchedule.minChargingRate = view.chargingSchedule().minChargingRate();
}

EvalChargingProfileResult ChargingProfile::eval(Option<time_t> startTxnTime, time_t now) {
    log_trace("Evaluating charging profile %" PRId32, this->id);

    EvalChargingProfileResult result;

    /*
    At any point in time
    the prevailing charging profile SHALL be the charging profile with the highest stackLevel among the profiles that
    are valid at that point in time, as determined by their validFrom and validTo parameters.
    */
    if (this->validFrom.is_some() && this->validFrom.unwrap() > now) {
        // This profile is not yet valid. Skip it and note that we have to check again if it becomes valid;
        log_trace("    validFrom set and in the future. Not valid yet.");
        result.nextCheck = this->validFrom.unwrap();
        return result;
    }

    if (this->validTo.is_some() && this->validTo.unwrap() < now) {
        // This profile is not valid anymore.
        log_trace("    validTo set and in the past. Not valid anymore.");
        return result;
    }

    if (this->validTo.is_some())
        result.nextCheck = std::min(result.nextCheck, this->validTo.unwrap());

    auto &sched = this->chargingSchedule;

    time_t schedStart;

    if (sched.startSchedule.is_some() &&
        /* Schedule periods are relative to a fixed point in time defined in the schedule. */
        (this->chargingProfileKind == ChargingProfileKind::ABSOLUTE ||
            /* The schedule restarts periodically at the first schedule period. */
            this->chargingProfileKind == ChargingProfileKind::RECURRING)) {
        log_trace("    startSchedule set and kind is absolute or recurring. Using startSchedule.");
        schedStart = sched.startSchedule.unwrap();
    } else {
        // Either this is a relative charging profile, or startSchedule is missing:
        /*  If absent the schedule will be relative to start of charging. */
        log_trace("    startSchedule not set or kind is relative. Assuming schedule is relative to the start of a transaction.");

        /* Schedule periods are relative to a situation-specific start point (such as the start of a Transaction) that is determined by the charge point. */
        if (!startTxnTime.is_some()) {
            log_trace("    No transaction running. Schedule does not apply.");
            log_trace("    TODO HIT.");
            // No transaction is running -> this profile does not apply.
            // TODO: Make sure this is correct.
            //       If for example we want to limit the current permanently,
            //       we could set a ChargePointMaxProfile of kind Recurring without start?
            return result;
        }

        /* Schedule periods are relative to a situation-specific start point (such as the start of a Transaction) that is determined by the charge point. */
        schedStart = startTxnTime.unwrap();
        log_trace("    Transaction running. Setting scheduleStart to start of transaction.");
    }

    if (now < schedStart) {
        log_trace("    Current time is before schedule start. Schedule does not apply yet.");
        // This schedule does not apply yet.
        result.nextCheck = std::min(result.nextCheck, schedStart);
        return result;
    }

    if (this->chargingProfileKind == ChargingProfileKind::RECURRING && this->recurrencyKind.is_some()) {
        log_trace("    Kind is recurring and recurrency kind is set. Maybe moving schedule start forward.");
        // If this is a recurring schedule, move the schedStart forward until schedStart <= time <= schedStart + recurrenceKind is true
        auto blockSize = this->recurrencyKind.unwrap() == RecurrencyKind::WEEKLY ? (7 * 24 * 60 * 60) : (24 * 60 * 60);
        schedStart = now - now % blockSize + (schedStart % blockSize);

        // We have to check again when the next block starts.
        result.nextCheck = std::min(result.nextCheck, schedStart + blockSize);
    }

    if (sched.duration.is_some()) {
        log_trace("    Schedule duration is set.");
        if (now >= (schedStart + sched.duration.unwrap())) {
            log_trace("    Schedule duration is elapsed.");
            return result;
        }

        // We have to check again when the duration elapses.
        result.nextCheck = std::min(result.nextCheck, schedStart + sched.duration.unwrap());
    }
    /* If the
        duration is left empty, the last period will continue indefinitely or
        until end of the transaction in case startSchedule is absent. */
    log_trace("    Checking schedule periods.");
    auto &period = sched.chargingSchedulePeriod[0];

    for(size_t i = 1; i < sched.chargingSchedulePeriodCount; ++i) {
        if (schedStart + sched.chargingSchedulePeriod[i].startPeriod > now) {
            log_trace("    Period %zu not reached yet. Using period %zu", i, i - 1);
            result.nextCheck = std::min(result.nextCheck, schedStart + sched.chargingSchedulePeriod[i].startPeriod);
            break;
        }
        period = sched.chargingSchedulePeriod[i];
    }
    log_trace("    Schedule applied.");

    result.applied = true;

    if (period.numberPhases.is_some()) {
        result.numberPhases = period.numberPhases.unwrap();
        log_trace("    Setting number of phases to %" PRId32 ".", result.numberPhases);
    }

    result.currentLimit = period.limit / (sched.unit == ChargingRateUnit::A ? 1.0f : (OCPP_LINE_VOLTAGE * (float)result.numberPhases));
    log_trace("    Setting current limit to %.3f.", result.currentLimit);

    if (sched.minChargingRate.is_some()) {
        result.minChargingCurrent = sched.minChargingRate.unwrap() / (sched.unit == ChargingRateUnit::A ? 1 : (OCPP_LINE_VOLTAGE * (float)result.numberPhases));
        log_trace("    Setting min charging current to %.3f.", result.minChargingCurrent);
    }

    return result;
}
