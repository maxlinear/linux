// SPDX-License-Identifier: GPL-2.0-only
/*
 *  step_wise.c - A step-by-step Thermal throttling governor
 *
 *  Copyright (C) 2012 Intel Corp
 *  Copyright (C) 2012 Durgadoss R <durgadoss.r@intel.com>
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <linux/thermal.h>
#include <trace/events/thermal.h>

#include "thermal_core.h"

/*
 * If the temperature is higher than a trip point,
 *    a. if the trend is THERMAL_TREND_RAISING, use higher cooling
 *       state for this trip point
 *    b. if the trend is THERMAL_TREND_DROPPING, do nothing
 *    c. if the trend is THERMAL_TREND_RAISE_FULL, use upper limit
 *       for this trip point
 *    d. if the trend is THERMAL_TREND_DROP_FULL, use lower limit
 *       for this trip point
 * If the temperature is lower than a hysteresis temperature,
 *    a. if the trend is THERMAL_TREND_RAISING, do nothing
 *    b. if the trend is THERMAL_TREND_DROPPING, use lower cooling
 *       state for this trip point, if the cooling state already
 *       equals lower limit, deactivate the thermal instance
 *    c. if the trend is THERMAL_TREND_RAISE_FULL, do nothing
 *    d. if the trend is THERMAL_TREND_DROP_FULL, use lower limit,
 *       if the cooling state already equals lower limit,
 *       deactivate the thermal instance
 */
static unsigned long get_target_state(struct thermal_instance *instance,
				      enum thermal_trend trend, bool throttle)
{
	struct thermal_cooling_device *cdev = instance->cdev;
	unsigned long cur_state;
	unsigned long next_target;
	int trip_temp;

	instance->tz->ops->get_trip_temp(instance->tz, instance->trip,
					 &trip_temp);

	/*
	 * We keep this instance the way it is by default.
	 * Otherwise, we use the current state of the
	 * cdev in use to determine the next_target.
	 */
	cdev->ops->get_cur_state(cdev, &cur_state);
	next_target = instance->target;
	dev_dbg(&cdev->device, "cur_state=%ld\n", cur_state);

	if (!instance->initialized) {
		if (throttle) {
			next_target = (cur_state + 1) >= instance->upper ?
					instance->upper :
					((cur_state + 1) < instance->lower ?
					instance->lower : (cur_state + 1));
		} else {
			next_target = THERMAL_NO_TARGET;
		}

		return next_target;
	}

	switch (trend) {
	case THERMAL_TREND_RAISING:
		if (throttle) {
			if (instance->tz->temperature < trip_temp)
				break; /* don't change state when throttling,
					* below trip_temp and temp rising
					*/

			next_target = cur_state < instance->upper ?
				    (cur_state + 1) : instance->upper;
			if (next_target < instance->lower)
				next_target = instance->lower;
		}
		break;
	case THERMAL_TREND_RAISE_FULL:
		if (throttle)
			next_target = instance->upper;
		break;
	case THERMAL_TREND_DROPPING:
		if (cur_state <= instance->lower) {
			if (!throttle)
				next_target = THERMAL_NO_TARGET;
		} else {
			if (!throttle) {
				next_target = cur_state - 1;
				if (next_target > instance->upper)
					next_target = instance->upper;
			}
		}
		break;
	case THERMAL_TREND_DROP_FULL:
		if (cur_state == instance->lower) {
			if (!throttle)
				next_target = THERMAL_NO_TARGET;
		} else {
			next_target = instance->lower;
		}
		break;
	default:
		break;
	}

	return next_target;
}

static void update_passive_instance(struct thermal_zone_device *tz,
				    enum thermal_trip_type type, int value)
{
	/*
	 * If value is +1, activate a passive instance.
	 * If value is -1, deactivate a passive instance.
	 */
	if (type == THERMAL_TRIP_PASSIVE)
		tz->passive += value;
}

static void thermal_zone_trip_update(struct thermal_zone_device *tz, int trip)
{
	int trip_temp, hyst_temp;
	enum thermal_trip_type trip_type;
	enum thermal_trend trend;
	struct thermal_instance *instance;
	bool throttle = false;
	int old_target;
	unsigned long  cur_state;
	struct thermal_cooling_device *cdev;

	tz->ops->get_trip_temp(tz, trip, &trip_temp);
	hyst_temp = trip_temp;
	if (tz->ops->get_trip_hyst) {
		tz->ops->get_trip_hyst(tz, trip, &hyst_temp);
		hyst_temp = trip_temp - hyst_temp;
	}
	tz->ops->get_trip_type(tz, trip, &trip_type);

	trend = get_tz_trend(tz, trip);

	dev_dbg(&tz->device,
		"Zone %d Trip%d[type=%d,temp=%d,hyst=%d]: trend=%d, cur_temp=%d",
		tz->id, trip, trip_type, trip_temp, hyst_temp, trend,
		tz->temperature);

	mutex_lock(&tz->lock);

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if (instance->trip != trip)
			continue;

		cdev = instance->cdev;
		cdev->ops->get_cur_state(cdev, &cur_state);

		if (instance->target == THERMAL_NO_TARGET)
			old_target = instance->target;
		else
			old_target = cur_state;

		throttle = false;
		/*
		 * Lower the mitigation only if the temperature
		 * goes below the hysteresis temperature.
		 */
		if (tz->temperature >= trip_temp ||
		    (tz->temperature >= hyst_temp &&
		   old_target != THERMAL_NO_TARGET)) {
			throttle = true;
			trace_thermal_zone_trip(tz, trip, trip_type);
		}

		instance->target = get_target_state(instance, trend, throttle);
		dev_dbg(&instance->cdev->device,
			"cur_target=%lu old_target=%d target=%d\n",
			cur_state, old_target, (int)instance->target);
		dev_dbg(&instance->cdev->device,
			"throttle=%d lower %lu upper %lu\n", throttle,
			instance->lower, instance->upper);

		if (instance->initialized && old_target == instance->target &&
		    old_target != instance->upper) {
			/* don't update cdev when there is no state
			 * change for all states but not the
			 * instance->upper (max state)
			 */
			continue;
		} else {
			/* there is a state change OR we change to max state */
			if (cur_state == instance->target)
				/* target state already set no need
				 * to update cdev
				 */
				continue;
			/* else
			 *    update cdev as there is a state change OR (
			 *    there is no state change but we try to set
			 *    upper/max state)
			 */
		}

		/* Activate a passive thermal instance */
		if (old_target == THERMAL_NO_TARGET &&
		    instance->target != THERMAL_NO_TARGET)
			update_passive_instance(tz, trip_type, 1);
		/* Deactivate a passive thermal instance */
		else if (old_target != THERMAL_NO_TARGET &&
			 instance->target == THERMAL_NO_TARGET)
			update_passive_instance(tz, trip_type, -1);

		instance->initialized = true;
		mutex_lock(&instance->cdev->lock);
		instance->cdev->updated = false; /* cdev needs update */
		mutex_unlock(&instance->cdev->lock);
	}

	mutex_unlock(&tz->lock);
}

/**
 * step_wise_throttle - throttles devices associated with the given zone
 * @tz: thermal_zone_device
 * @trip: trip point index
 *
 * Throttling Logic: This uses the trend of the thermal zone to throttle.
 * If the thermal zone is 'heating up' this throttles all the cooling
 * devices associated with the zone and its particular trip point, by one
 * step. If the zone is 'cooling down' it brings back the performance of
 * the devices by one step.
 */
static int step_wise_throttle(struct thermal_zone_device *tz, int trip)
{
	struct thermal_instance *instance;

	thermal_zone_trip_update(tz, trip);

	mutex_lock(&tz->lock);

	list_for_each_entry(instance, &tz->thermal_instances, tz_node)
		thermal_cdev_update(instance->cdev);

	mutex_unlock(&tz->lock);

	return 0;
}

static struct thermal_governor thermal_gov_step_wise = {
	.name		= "step_wise",
	.throttle	= step_wise_throttle,
};

THERMAL_GOVERNOR_DECLARE(thermal_gov_step_wise);
