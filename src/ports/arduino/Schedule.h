/**
 * @file BACnetSchedule.h
 * @brief Arduino wrapper for the bacnet-stack Schedule object.
 *
 * This class provides an Arduino-friendly API for configuring BACnet
 * Schedule objects while preserving compatibility with bacnet-stack.
 *
 * Features:
 *  - Weekly schedules
 *  - Automatic value application
 *  - BACnet write priority support
 *  - Persistent storage callbacks
 *  - Arduino TimeLib compatible API
 */


#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <Arduino.h>

#include "bacnet/bacdef.h"
#include "bacnet/basic/object/schedule.h"

#include "WeeklyAlarm.h"

// Schedule.h - completely independent of stack's MAX_SCHEDULES:
#ifndef BACNET_SCHEDULE_INSTANCES
#define BACNET_SCHEDULE_INSTANCES 1
#endif


class BACnetSchedule
{
public:
    /**
     * @brief Create a BACnet Schedule wrapper.
     *
     * @param inst BACnet Schedule instance number (0 to
     * BACNET_SCHEDULE_INSTANCES - 1).
     */
    BACnetSchedule(uint8_t inst);

    ~BACnetSchedule();

    /**
     * @brief Restore this schedule from persistent storage.
     *
     * Invokes the registered restore callback. If valid schedule data is
     * returned, the schedule descriptor is restored.
     */
    static void begin();

    /**
     * @brief Convert Arduino time elements to a BACnet time.
     *
     * @param t Arduino-style time elements.
     * @return Equivalent BACnet time.
     */
    static BACNET_TIME toBACnetTime(const tmElements_t &t);

    /**
     * @brief Convert Arduino time elements to a BACnet date.
     *
     * Converts the weekday convention (Sunday=1 ... Saturday=7)
     * to the BACnet convention (Monday=1 ... Sunday=7).
     *
     * @param t Arduino-style time elements.
     * @return Equivalent BACnet date.
     */
    static BACNET_DATE toBACnetDate(const tmElements_t &t);

    /**
     * @brief Convert a BACnet date and time to Arduino time elements.
     *
     * Converts the BACnet weekday convention to the Arduino convention.
     *
     * @param date BACnet date.
     * @param time BACnet time.
     * @return Arduino-compatible time elements.
     */
    static tmElements_t toTimeElements(const BACNET_DATE &date, const BACNET_TIME &time = {0, 0, 0, 0});

    /**
     * @brief Convert an Arduino weekday to a BACnet weekly schedule index.
     *
     * @param day Arduino weekday (Sunday=1 ... Saturday=7).
     * @return BACnet weekday index (Monday=1 ... Sunday=7).
     */
    static uint8_t toBACnetScheduleDayIndex(timeDayOfWeek_t day);

    /**
     * @brief Set the schedule effective period.
     *
     * Outside this date range, the schedule uses its default value.
     *
     * @param startYear Start year.
     * @param startMonth Start month (1-12).
     * @param startDay Start day (1-31).
     * @param endYear End year.
     * @param endMonth End month (1-12).
     * @param endDay End day (1-31).
     */
    void setEffectivePeriod(uint16_t startYear, uint8_t startMonth, uint8_t startDay,
                            uint16_t endYear, uint8_t endMonth, uint8_t endDay);
    
    /**
     * @brief Set the schedule default Boolean value.
     *
     * This value is used when no scheduled event is active.
     *
     * @param value Default Boolean value.
     */
    void setDefaultBool(bool value);

    /**
     * @brief Set the schedule default unsigned integer value.
     *
     * @param value Default unsigned integer value.
     */
    void setDefaultUnsigned(uint16_t value);

    /**
     * @brief Set the schedule default signed integer value.
     *
     * @param value Default signed integer value.
     */
    void setDefaultSigned(int16_t value);

    /**
     * @brief Set the schedule default real value.
     *
     * @param value Default floating-point value.
     */
    void setDefaultReal(float value);

    /**
     * @brief Set the schedule default double value.
     *
     * @param value Default double-precision value.
     */
    void setDefaultDouble(double value);

    /**
     * @brief Set the schedule default enumerated value.
     *
     * @param value Default BACnet enumerated value.
     */
    void setDefaultEnum(uint32_t value);
    
    /**
    * @brief Configure a time-value entry for a specific day of the week.
    * Sets what value the schedule should output at a given time on a given day.
    * The value type must match the type set via setDefault<T>().
    * 
    * @tparam T value type (float, bool, uint32_t, int32_t, double)
    * @param dayOfWeek BACnet weekday index (dowSunday to dowSaturday)
    * @param index index of the time-value entry (0 to BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE-1)
    * @param hour hour of day (0-23)
    * @param min minute (0-59)
    * @param value the output value at this time
    * 
    * @note Only one time-value entry per day is supported in this implementation.
    *       Call this once per day you want to configure.
    */
    template<typename T>
    void setSchedule(uint8_t dayOfWeek, uint8_t index, uint8_t hour, uint8_t min, T value)
    {
        SCHEDULE_DESCR *desc = Schedule_Object(instance);
        if (!desc) return;

        BACNET_TIME_VALUE tv = {};
        switch (desc->Schedule_Default.tag) {
            case BACNET_APPLICATION_TAG_BOOLEAN:
                tv.Value.type.Boolean = static_cast<bool>(value);
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                tv.Value.type.Unsigned_Int = static_cast<uint32_t>(value);
                break;
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                tv.Value.type.Signed_Int = static_cast<int32_t>(value);
                break;
            case BACNET_APPLICATION_TAG_REAL:
                tv.Value.type.Real = static_cast<float>(value);
                break;
            case BACNET_APPLICATION_TAG_DOUBLE:
                tv.Value.type.Double = static_cast<double>(value);
                break;
            case BACNET_APPLICATION_TAG_ENUMERATED:
                tv.Value.type.Enumerated = static_cast<uint32_t>(value);
                break;
            default:
                // Unsupported type
                return;
        }
        tv.Value.tag = desc->Schedule_Default.tag;
            
        
        tv.Time.hour = hour;
        tv.Time.min = min;
        tv.Time.sec = 0;
        tv.Time.hundredths = 0;
        // bacnet schedule monday to sunday 0-6 
        uint8_t dowBacknet = toBACnetScheduleDayIndex(static_cast<timeDayOfWeek_t>(dayOfWeek));
        BACNET_DAILY_SCHEDULE ds = desc->Weekly_Schedule[dowBacknet];
        ds.Time_Values[index] = tv;

        if (index >= ds.TV_Count) ds.TV_Count = index + 1;

        Schedule_Weekly_Schedule_Set(instance, dowBacknet, &ds);
        BACNET_APPLICATION_DATA_VALUE oldValue = desc->Present_Value;
        recalculate_PV(instance);
        if (Schedule_Value_Changed(oldValue, desc->Present_Value))
        {
            applyPresentValue(desc);
        }
        scheduleNextEvent();
        arm();
        save();
    }

    /**
     * @brief Set the BACnet object referenced by this schedule.
     *
     * The scheduled Present_Value will be applied to the referenced object
     * when the schedule value changes.
     *
     * @param type BACnet object type to reference.
     * @param objectInstance BACnet object instance number.
     */
    void set_Object_Property_Reference(BACnetObjectType type, uint32_t objectInstance);

    /**
     * @brief Process pending schedule events.
     *
     * This function is called internally by the port timer implementation.
     * Users normally do not need to call this function directly.
     *
     * The handler dispatches pending WeeklyAlarm events and executes the
     * associated schedule callbacks.
     */
    static inline void handler(){ WeeklyAlarm::handler(); }

    /**
     * @brief Get the BACnet Schedule object instance number.
     *
     * @return BACnet Schedule instance associated with this object.
     */
    uint32_t getInstance() const;
    /**
     * @brief Set the BACnet write priority used when applying scheduled values.
     *
     * Valid priorities are 1-16 as defined by the BACnet standard.
     *
     * @param priority BACnet write priority.
     * @return true on success.
     * @return false if the priority is invalid.
     */
    bool setWritePriority(uint8_t priority);

    /**
     * @brief Get the BACnet priority used when applying scheduled values.
     *
     * The returned priority is used when the schedule writes its Present_Value
     * to referenced BACnet objects.
     *
     * @return BACnet write priority (1-16).
     */
    uint8_t getWritePriority() const;

    /**
     * @brief Persistent data representation of a BACnet Schedule object.
     *
     * This structure contains the schedule configuration required to restore
     * a Schedule object after a device restart.
     *
     * It is intended to be used by the user-provided save and restore callbacks.
     */
    struct SchedulePersistentData
    {
        BACNET_DATE_RANGE effective_period;
        BACNET_DAILY_SCHEDULE weekly_schedule[7];
        BACNET_APPLICATION_DATA_VALUE default_value;
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE references[BACNET_SCHEDULE_OBJ_PROP_REF_SIZE];
        uint8_t priority;
        uint8_t referencesCount;
        uint16_t version = 1;
    };
    friend struct SchedulePersistentData;

    /**
     * @brief Register the callback used to save schedule data.
     *
     * The callback is called whenever a schedule configuration must be
     * persisted. The application is responsible for storing the provided
     * SchedulePersistentData using its preferred storage method.
     *
     * @param callback Function called with the schedule instance number and
     *                 persistent schedule data.
     *
     * @note The library does not perform storage directly. The application
     *       must provide the storage implementation.
     */
    static void setSaveCallback(void (*callback)(uint32_t inst, const SchedulePersistentData &data));

    /**
     * @brief Register the callback used to restore schedule data.
     *
     * The callback is called during schedule initialization to retrieve
     * previously saved schedule configuration.
     *
     * The callback should fill the provided SchedulePersistentData structure
     * and return true when valid data is available.
     *
     * @param callback Function called with the schedule instance number and
     *                 destination data structure.
     *
     * @return The callback should return true if restoration succeeds,
     *         false if no valid stored data exists.
     *
     * @note The library does not perform storage directly. The application
     *       must provide the storage implementation.
     */
    static void setRestoreCallback(bool (*callback)(uint32_t inst, SchedulePersistentData *data));

private:
    static BACnetSchedule *BS_Instances[BACNET_SCHEDULE_INSTANCES];
    static void (*saveCallback)(uint32_t inst, const SchedulePersistentData &data);
    static bool (*restoreCallback)(uint32_t inst, SchedulePersistentData *data);
    WeeklyAlarm alarm;
    uint8_t instance; //Schedule_Object(instance)
    
    static void recalculate_PV(uint32_t inst);
    bool Schedule_Apply_Object_Reference(
        const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
        const BACNET_APPLICATION_DATA_VALUE *value,
        uint8_t priority);
    bool Schedule_Apply_Boolean(
        const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
        bool value,
        uint8_t priority);
    bool Schedule_Apply_Real(
        const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
        float value,
        uint8_t priority);
    bool Schedule_Apply_Unsigned(
        const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
        uint32_t value,
        uint8_t priority);
    void applyPresentValue(SCHEDULE_DESCR *desc);
    bool Schedule_Value_Changed(
        const BACNET_APPLICATION_DATA_VALUE &oldValue,
        const BACNET_APPLICATION_DATA_VALUE &newValue);
    void arm();
    static BACnetSchedule *getBS_Instance(uint8_t inst);
    struct ScheduleCallback {
        BACnetSchedule *schedule = nullptr;
        void operator()();
    };
    friend struct BACnetSchedule::ScheduleCallback;
    
    static void schedulePropertyWriteCallback(uint32_t instance);
    void scheduleNextEvent();
    void restore();
    void save();
    static bool Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, const BACNET_TIME *time);
};


#ifdef __cplusplus
extern "C" {
#endif
    void set_Schedule_Write_Callback(
        void (*callback)(uint32_t PROP_INSTANCE_OF));
#ifdef __cplusplus
}
#endif


#endif //SCHEDULE_H



/*
Small convenience APIs


- Apply immediately

Sometimes after editing a schedule you want

schedule.refresh();

instead of waiting for the callback chain.

Internally:

recalculate
apply
arm next
Force apply

Useful for commissioning.

schedule.apply();

Even if the value didn't change.


- Query current value

Something like

auto value = schedule.presentValue();

instead of reaching into SCHEDULE_DESCR.

Query next event

This one could be surprisingly useful.

- time_t next = schedule.nextEvent();

or

tmElements_t next = schedule.nextEventTime();

Especially for HMIs.

Enable/Disable

A wrapper around Out Of Service.

schedule.enable(false);

Not required, but Arduino users would probably expect it.
*/