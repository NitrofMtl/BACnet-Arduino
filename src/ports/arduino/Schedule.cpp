#include "Schedule.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/ms-input.h"


constexpr uint32_t secPerDay = 24*60*60;

BACnetSchedule* BACnetSchedule::BS_Instances[BACNET_SCHEDULE_INSTANCES] = {};
void (*BACnetSchedule::saveCallback)(uint32_t inst, const SchedulePersistentData &data) = nullptr;
bool (*BACnetSchedule::restoreCallback)(uint32_t inst, SchedulePersistentData *data) = nullptr;


BACnetSchedule::BACnetSchedule(uint8_t inst) : alarm(), instance(inst)
{
    if (instance >= BACNET_SCHEDULE_INSTANCES) return; //dot set instance bigger than the array size
    if (!Schedule_Object(instance)) {
        Serial.println("Warning: Schedule instance exceeds stack MAX_SCHEDULES");
        return;
    }
    BS_Instances[instance] = this;
    set_Schedule_Write_Callback(BACnetSchedule::schedulePropertyWriteCallback);
    ScheduleCallback sc;
    sc.schedule = this;
    alarm.add(sc);
}


BACnetSchedule::~BACnetSchedule()
{
    if (instance < BACNET_SCHEDULE_INSTANCES) {
        BS_Instances[instance] = nullptr;
    }
    alarm.remove();
}


void BACnetSchedule::begin()
{
    for (uint8_t i = 0; i < BACNET_SCHEDULE_INSTANCES; ++i) {
        if (BS_Instances[i]) {
            BS_Instances[i]->restore();
            recalculate_PV(i);
            BS_Instances[i]->applyPresentValue(Schedule_Object(i));
            BS_Instances[i]->scheduleNextEvent();
            BS_Instances[i]->arm();
        }
    }
}


void ::BACnetSchedule::setEffectivePeriod(uint16_t startYear, uint8_t startMonth, uint8_t startDay,
                            uint16_t endYear, uint8_t endMonth, uint8_t endDay)
{
    if (Schedule_Object(instance) == nullptr) {
        Serial.println("Error: Schedule object not found for instance " + String(instance));
        return;
    }

    BACNET_DATE start = {}, end = {};

    start.year = startYear - 1900;
    start.month = startMonth;
    start.day = startDay;
    start.wday = 0;

    end.year = endYear - 1900;  
    end.month = endMonth; 
    end.day = endDay;   
    end.wday = 0;

    Schedule_Effective_Period_Set(instance, &start, &end);
}


void BACnetSchedule::setDefaultBool(bool value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    desc->Schedule_Default.type.Boolean = value;
    desc->Present_Value = desc->Schedule_Default;
}


void BACnetSchedule::setDefaultUnsigned(uint16_t value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    desc->Schedule_Default.type.Unsigned_Int = value;
    desc->Present_Value = desc->Schedule_Default;
}


void BACnetSchedule::setDefaultSigned(int16_t value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    desc->Schedule_Default.type.Signed_Int = value;
    desc->Present_Value = desc->Schedule_Default;
}


void BACnetSchedule::setDefaultReal(float value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_REAL;
    desc->Schedule_Default.type.Real = value;
    desc->Present_Value = desc->Schedule_Default;
}


void BACnetSchedule::setDefaultDouble(double value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_DOUBLE;
    desc->Schedule_Default.type.Double = value;
    desc->Present_Value = desc->Schedule_Default;
}


void BACnetSchedule::setDefaultEnum(uint32_t value)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    desc->Schedule_Default.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    desc->Schedule_Default.type.Enumerated = value;
    desc->Present_Value = desc->Schedule_Default;
}


BACNET_TIME BACnetSchedule::toBACnetTime(const tmElements_t &t)
{
    BACNET_TIME bacnetTime;
    bacnetTime.hour = t.Hour;
    bacnetTime.min = t.Minute;
    bacnetTime.sec = t.Second;
    bacnetTime.hundredths = 0;
    return bacnetTime;
}


BACNET_DATE BACnetSchedule::toBACnetDate(const tmElements_t &t)
{
    BACNET_DATE bacnetDate;
    bacnetDate.year = t.Year + 1970; // bacnet-stack year = real year
    bacnetDate.month = t.Month;
    bacnetDate.day = t.Day;
    if (t.Wday == dowInvalid) bacnetDate.wday = 0; // unspecified
    else if (t.Wday == dowSunday) bacnetDate.wday = 7;
    else bacnetDate.wday = t.Wday - 1;
    return bacnetDate;
}


tmElements_t BACnetSchedule::toTimeElements(const BACNET_DATE &date, const BACNET_TIME &time)
{
    tmElements_t t;
    t.Year = date.year - 1970; // Convert from bacnet-stack real year representation
    t.Month = date.month;
    t.Day = date.day;
    if (date.wday == 0) t.Wday = dowInvalid;
    else if (date.wday == 7) t.Wday = dowSunday;
    else t.Wday = date.wday + 1;
    t.Hour = time.hour;
    t.Minute = time.min;
    t.Second = time.sec;
    return t;
}


uint8_t BACnetSchedule::toBACnetScheduleDayIndex(timeDayOfWeek_t day) {
    // BACnet weekly schedule: 0=Monday ... 6=Sunday
    // TimeLib/WA: 1=Sunday ... 7=Saturday
    return (day == dowSunday) ? 6 : day - 2;
}


void BACnetSchedule::schedulePropertyWriteCallback(uint32_t instance)
{
    BACnetSchedule* BS = getBS_Instance(instance);
    if (!BS) return;
    SCHEDULE_DESCR *desc = Schedule_Object(BS->getInstance());
    if (!desc) return;

    BACNET_APPLICATION_DATA_VALUE oldValue = desc->Present_Value;
    recalculate_PV(instance);
    if (BS->Schedule_Value_Changed(oldValue, desc->Present_Value))
    {
        BS->applyPresentValue(desc);
    }

    BS->scheduleNextEvent();
    BS->arm();
}


void BACnetSchedule::arm()
{
    alarm.arm();
}


void BACnetSchedule::set_Object_Property_Reference(BACnetObjectType type, uint32_t objectInstance)
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE ref = {};

    ref.objectIdentifier.type = type;
    ref.objectIdentifier.instance = objectInstance;

    ref.propertyIdentifier = PROP_PRESENT_VALUE;
    ref.arrayIndex = BACNET_ARRAY_ALL;
    ref.deviceIdentifier.type = OBJECT_DEVICE;
    ref.deviceIdentifier.instance = BACNET_MAX_INSTANCE;

    uint8_t refIndex = desc->obj_prop_ref_cnt;
    if (desc->obj_prop_ref_cnt >= BACNET_SCHEDULE_OBJ_PROP_REF_SIZE) return;

    if (Schedule_List_Of_Object_Property_References_Set(instance, refIndex, &ref)) {
        desc->obj_prop_ref_cnt++;
    }
}


void BACnetSchedule::recalculate_PV(uint32_t inst)
{
    SCHEDULE_DESCR *desc = Schedule_Object(inst);
    if (!desc) return;

    tmElements_t t = {};
    breakTime(now(), t);
    BACNET_DATE today = toBACnetDate(t);
    BACNET_TIME now_time = toBACnetTime(t);

    //today
    if (Schedule_Recalculate_PV(desc, (BACNET_WEEKDAY)today.wday, &now_time)){
        Serial.println("Schedule value found for today");
        return;
    };

    BACNET_TIME endOfDay = {};
    endOfDay.hundredths = 99;
    endOfDay.sec = 59;
    endOfDay.min = 59;
    endOfDay.hour = 23;

    for (uint8_t day = (today.wday == 1) ? 7 : today.wday - 1; day != today.wday; day = (day == 1) ? 7 : day - 1) {
        if (Schedule_Recalculate_PV(desc, (BACNET_WEEKDAY)day, &endOfDay)) {
            return;
        }
    }
    // Nothing found in the last 7 days
    memcpy(&desc->Present_Value, &desc->Schedule_Default, sizeof(desc->Present_Value));
}





/*
 * This is based on the original bacnet-stack implementation of
 * Schedule_Recalculate_PV().
 *
 * The original stack implementation only evaluates the supplied weekday.
 * It applies Schedule_Default immediately when no value is found for that day,
 * which means previous days in a weekly schedule are not considered.
 *
 * This version returns true when a scheduled value is found and leaves the
 * default value handling to the caller. This allows the caller to search
 * previous days before falling back to Schedule_Default.
 */
/**
 * @brief Recalculate the Present Value of the Schedule object
 * @param desc - schedule descriptor
 * @param wday - day of the week
 * @param time - time of the day
 */
bool BACnetSchedule::Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, const BACNET_TIME *time)
{
    int i, current, diff;
    BACNET_TIME *tmptime;

    if (!desc || !time || (wday < 1) || (wday > 7)) {
        return false;
    }
    desc->Present_Value.tag = BACNET_APPLICATION_TAG_NULL;

    /* for future development, here should be the loop for Exception Schedule */

    /*  Note to developers: please ping Edward at info@connect-ex.com
        for a more complete schedule object implementation. */
    current = -1;
    tmptime = NULL;
    for (i = 0; i < desc->Weekly_Schedule[wday - 1].TV_Count; i++) {
        diff = datetime_wildcard_compare_time(
            time, &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time);
        if (diff >= 0) {
            if (tmptime == NULL) {
                tmptime = &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time;
                current = i;
            } else {
                diff = datetime_wildcard_compare_time(
                    &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time,
                    tmptime);
                if (diff >= 0) {
                    tmptime =
                        &desc->Weekly_Schedule[wday - 1].Time_Values[i].Time;
                    current = i;
                }
            }
        }
    }
    if (current >= 0) {
        bacnet_primitive_to_application_data_value(
            &desc->Present_Value,
            &desc->Weekly_Schedule[wday - 1].Time_Values[current].Value);
            return true;
    } else {
        return false;
        /*memcpy(
            &desc->Present_Value, &desc->Schedule_Default,
            sizeof(desc->Present_Value));*/
    }
}


void BACnetSchedule::ScheduleCallback::operator()()
{
    uint32_t inst = schedule->getInstance();
    SCHEDULE_DESCR *desc = Schedule_Object(inst);
    if (!desc) return;

    if (desc->Out_Of_Service) {
        schedule->scheduleNextEvent();
        return;
    }
    BACNET_APPLICATION_DATA_VALUE oldValue = desc->Present_Value;

    recalculate_PV(inst);

    if (schedule->Schedule_Value_Changed(oldValue, desc->Present_Value)) {
         Serial.println("Schedule value change !!!"); 
        schedule->applyPresentValue(desc);
    }
    schedule->scheduleNextEvent();
    Serial.println("Schedule callback !!!");   
}


uint32_t BACnetSchedule::getInstance() const
{ 
    return instance;
}


BACnetSchedule* BACnetSchedule::getBS_Instance(uint8_t inst)
{
    if (inst >= BACNET_SCHEDULE_INSTANCES) return nullptr;
    return BS_Instances[inst];
}


void BACnetSchedule::scheduleNextEvent()
{
    uint32_t nextTime = UINT32_MAX;

    uint32_t tNow = now();
    tmElements_t now = {};
    breakTime(tNow, now);

    for (uint8_t day = dowSunday; day <= dowSaturday; day++) {
        BACNET_DAILY_SCHEDULE *ds =
            &Schedule_Object(instance)->Weekly_Schedule[toBACnetScheduleDayIndex(
                static_cast<timeDayOfWeek_t>(day))];

        if (ds->TV_Count == 0) continue;

        for (uint8_t i = 0; i < ds->TV_Count; i++) {
            BACNET_TIME_VALUE &tv = ds->Time_Values[i];
            tmElements_t t = now;
            t.Hour   = tv.Time.hour;
            t.Minute = tv.Time.min;
            t.Second = 0;

            uint8_t dayOffset = (day - now.Wday + 7) % 7;
            uint32_t candidate = makeTime(t) + dayOffset * secPerDay;
            if (candidate <= tNow) candidate += secPerDay * 7; // already passed, next week

            if (candidate < nextTime) {
                nextTime = candidate;
            }
        }
    }

    if (nextTime != UINT32_MAX) {
        alarm.alarmOff();
        alarm.alarmOn();
        alarm.setSingleAlarm(nextTime);
    }
}


bool BACnetSchedule::Schedule_Apply_Object_Reference(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
    const BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority)
{
    if (!ref || !value) return false;
    if (ref->propertyIdentifier != PROP_PRESENT_VALUE) return false;

    switch (value->tag)
    {
    case BACNET_APPLICATION_TAG_BOOLEAN:
        return Schedule_Apply_Boolean(ref, value->type.Boolean, priority);
    case BACNET_APPLICATION_TAG_REAL:
        return Schedule_Apply_Real(ref, value->type.Real, priority);
    case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        return Schedule_Apply_Unsigned(ref, value->type.Unsigned_Int, priority);
    case BACNET_APPLICATION_TAG_ENUMERATED:
    return Schedule_Apply_Unsigned(ref, value->type.Enumerated, priority);   
    default:
        return false;
    }
}


bool BACnetSchedule::Schedule_Apply_Boolean(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
    bool value,
    uint8_t priority)
{
    BACNET_BINARY_PV pv = value ? BINARY_ACTIVE : BINARY_INACTIVE;

    switch (ref->objectIdentifier.type)
    {
    case OBJECT_BINARY_INPUT:
        return Binary_Input_Present_Value_Set(ref->objectIdentifier.instance, pv);
    case OBJECT_BINARY_OUTPUT:
        return Binary_Output_Present_Value_Set(ref->objectIdentifier.instance, pv, priority);
    case OBJECT_BINARY_VALUE:
        return Binary_Value_Present_Value_Set(ref->objectIdentifier.instance, pv);
    default:
        return false;
    }
}


bool BACnetSchedule::Schedule_Apply_Real(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
    float value,
    uint8_t priority)
{
    switch (ref->objectIdentifier.type)
    {
    case OBJECT_ANALOG_INPUT:
        Analog_Input_Present_Value_Set(ref->objectIdentifier.instance, value);
        return true;
    case OBJECT_ANALOG_OUTPUT:
        return Analog_Output_Present_Value_Set(ref->objectIdentifier.instance, value, priority);
    case OBJECT_ANALOG_VALUE:
        return Analog_Value_Present_Value_Set(ref->objectIdentifier.instance, value, priority);
    default:
        return false;
    }
}


bool BACnetSchedule::Schedule_Apply_Unsigned(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *ref,
    uint32_t value,
    uint8_t priority)
{
    switch (ref->objectIdentifier.type)
    {
    case OBJECT_MULTI_STATE_OUTPUT:
        return Multistate_Output_Present_Value_Set(ref->objectIdentifier.instance, value, priority);
    case OBJECT_MULTI_STATE_INPUT:
        return Multistate_Input_Present_Value_Set(ref->objectIdentifier.instance, value);
    case OBJECT_MULTI_STATE_VALUE:
        return Multistate_Value_Present_Value_Set(ref->objectIdentifier.instance, value);
    default:
        return false;
    }
}


void BACnetSchedule::applyPresentValue(SCHEDULE_DESCR *desc)
{
    for (uint8_t i = 0; i < desc->obj_prop_ref_cnt; i++)
    {
        Schedule_Apply_Object_Reference(
        &desc->Object_Property_References[i],
        &desc->Present_Value,
        desc->Priority_For_Writing);
    }   
}


bool BACnetSchedule::Schedule_Value_Changed(
    const BACNET_APPLICATION_DATA_VALUE &oldValue,
    const BACNET_APPLICATION_DATA_VALUE &newValue)
{
    if (oldValue.tag != newValue.tag)
        return true;

    switch (oldValue.tag)
    {
    case BACNET_APPLICATION_TAG_BOOLEAN:
        return oldValue.type.Boolean != newValue.type.Boolean;

    case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        return oldValue.type.Unsigned_Int != newValue.type.Unsigned_Int;

    case BACNET_APPLICATION_TAG_ENUMERATED:
        return oldValue.type.Enumerated != newValue.type.Enumerated;

    case BACNET_APPLICATION_TAG_REAL:
        return oldValue.type.Real != newValue.type.Real;

    case BACNET_APPLICATION_TAG_SIGNED_INT:
        return oldValue.type.Signed_Int != newValue.type.Signed_Int;

    default:
        return true;
    }
}


bool BACnetSchedule::setWritePriority(uint8_t priority)
{
    if (priority == 0 || priority > 16) return false;
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return false;
    desc->Priority_For_Writing = priority;
    return true;
}


uint8_t BACnetSchedule::getWritePriority() const
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return 0;
    return desc->Priority_For_Writing;
}


void BACnetSchedule::save()
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;

    SchedulePersistentData data = {};

    data.effective_period = { desc->Start_Date, desc->End_Date };
    for (uint8_t day = 0; day < 7; day++) {
        data.weekly_schedule[day] = desc->Weekly_Schedule[day];
    }
    data.default_value = desc->Schedule_Default;
    for (uint8_t ref = 0; ref < desc->obj_prop_ref_cnt; ref++) {
        data.references[ref] = desc->Object_Property_References[ref];
    }
    data.priority = desc->Priority_For_Writing;
    data.referencesCount = desc->obj_prop_ref_cnt;

    if (saveCallback) saveCallback(instance, data);
}


void BACnetSchedule::restore()
{
    SCHEDULE_DESCR *desc = Schedule_Object(instance);
    if (!desc) return;

    if (!restoreCallback) return;
    SchedulePersistentData data = {};
    if (!restoreCallback(instance, &data)) return;

    if (data.referencesCount > BACNET_SCHEDULE_OBJ_PROP_REF_SIZE) return;

    desc->Start_Date = data.effective_period.startdate;
    desc->End_Date = data.effective_period.enddate;
    for (uint8_t day = 0; day < 7; day++) {
        desc->Weekly_Schedule[day] = data.weekly_schedule[day];
    }
    desc->Schedule_Default = data.default_value;
    for (uint8_t ref = 0; ref < data.referencesCount; ref++) {
        desc->Object_Property_References[ref] = data.references[ref];
    }
    desc->Priority_For_Writing = data.priority;
    desc->obj_prop_ref_cnt = data.referencesCount;   
}


void BACnetSchedule::setSaveCallback(void (*callback)(uint32_t inst, const SchedulePersistentData &data))
{
    saveCallback = callback;
}


void BACnetSchedule::setRestoreCallback(bool (*callback)(uint32_t inst, SchedulePersistentData *data))
{
    restoreCallback = callback;
}