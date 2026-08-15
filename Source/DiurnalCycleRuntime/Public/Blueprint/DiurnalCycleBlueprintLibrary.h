#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DiurnalCycleTypes.h"
#include "DiurnalCycleWorldSettings.h"

#include "DiurnalCycleBlueprintLibrary.generated.h"

class UDiurnalCycleBlueprintSubsystem;
class UDiurnalCycleSubsystem;
class UDiurnalCycleWorldSubsystem;

/**
 * Designer-facing Blueprint access to the day-night-cycle runtime.
 *
 * The library resolves the clock associated with the supplied world context.
 * Mutation functions return false when no valid runtime exists or when the
 * supplied value is rejected by native validation.
 */
UCLASS()
class DIURNALCYCLERUNTIME_API UDiurnalCycleBlueprintLibrary final
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region Availability

	/** Returns whether a day-night-cycle clock exists for this world context. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Day Night Cycle Available"
		))
	static bool IsDayNightCycleAvailable(
		const UObject* WorldContextObject);

#pragma endregion

#pragma region ClockControl

	/** Sets whether automatic clock advancement is explicitly paused. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Set Day Night Cycle Paused"
		))
	static bool SetDayNightCyclePaused(
		const UObject* WorldContextObject,
		bool bPaused);

	/**
	 * Sets the clock-speed multiplier.
	 *
	 * Zero prevents automatic advancement without changing the pause state.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Set Day Night Cycle Time Scale"
		))
	static bool SetDayNightCycleTimeScale(
		const UObject* WorldContextObject,
		double NewTimeScale);

	/**
	 * Teleports the clock to NewDateTime.
	 *
	 * Ordinary scheduled occurrences between the previous and destination times
	 * are not replayed. Ranges and exact-destination time gates are reconciled.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Set Day Night Cycle Date Time"
		))
	static bool SetDayNightCycleDateTime(
		const UObject* WorldContextObject,
		const FDiurnalDateTime& NewDateTime);

	/**
	 * Advances the clock by a positive number of game hours.
	 *
	 * Advancement stops at the first blocking occurrence when a time gate is
	 * encountered.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Advance Day Night Cycle Hours"
		))
	static bool AdvanceDayNightCycleHours(
		const UObject* WorldContextObject,
		double GameHours);

#pragma endregion

#pragma region ClockQueries

	/** Returns whether automatic advancement is explicitly paused. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Day Night Cycle Paused"
		))
	static bool IsDayNightCyclePaused(
		const UObject* WorldContextObject);

	/**
	 * Returns the current date and time.
	 *
	 * Returns Day 1 at 00:00:00 when the clock is unavailable.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Date Time"
		))
	static FDiurnalDateTime GetDayNightCycleDateTime(
		const UObject* WorldContextObject);

	/** Returns the current exact time of day, or 00:00:00 when unavailable. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Time Of Day"
		))
	static FDiurnalTimeOfDay GetDayNightCycleTimeOfDay(
		const UObject* WorldContextObject);

	/** Returns the current one-based game day, or one when unavailable. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Current Day"
		))
	static int32 GetDayNightCycleCurrentDay(
		const UObject* WorldContextObject);

	/** Returns the current fractional hour in [0, 24). */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Time Of Day Hours"
		))
	static double GetDayNightCycleTimeOfDayHours(
		const UObject* WorldContextObject);

	/** Returns normalized progress through the current day in [0, 1). */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Day Progress"
		))
	static double GetDayNightCycleDayProgress(
		const UObject* WorldContextObject);

	/** Returns elapsed game hours since Day 1 at 00:00. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Total Game Hours"
		))
	static double GetDayNightCycleTotalGameHours(
		const UObject* WorldContextObject);

	/** Returns the current clock-speed multiplier. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Time Scale"
		))
	static double GetDayNightCycleTimeScale(
		const UObject* WorldContextObject);

	/** Returns the configured real seconds per game hour at time scale one. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Base Rate"
		))
	static double GetDayNightCycleBaseRate(
		const UObject* WorldContextObject);

	/**
	 * Returns the effective real seconds per game hour during automatic
	 * advancement.
	 *
	 * Returns zero while automatic advancement is stopped by pause, zero time
	 * scale, an active time gate, or the current world's advancement policy.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Clock",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Effective Rate"
		))
	static double GetDayNightCycleEffectiveRate(
		const UObject* WorldContextObject);

#pragma endregion

#pragma region WorldPolicy

	/** Returns whether the current world permits automatic clock advancement. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|World",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Does Current World Advance Day Night Cycle"
		))
	static bool DoesCurrentWorldAdvanceDayNightCycle(
		const UObject* WorldContextObject);

	/**
	 * Returns the effective time policy for the current world.
	 *
	 * Returns Use Project Default when no valid world policy runtime exists.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|World",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle World Time Policy"
		))
	static EDiurnalCycleWorldTimePolicy
	GetDayNightCycleWorldTimePolicy(
		const UObject* WorldContextObject);

	/** Returns whether the current world has a transient runtime policy override. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|World",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Has Day Night Cycle World Time Policy Override"
		))
	static bool HasDayNightCycleWorldTimePolicyOverride(
		const UObject* WorldContextObject);

	/**
	 * Sets a transient policy override for the current world.
	 *
	 * Use Project Default clears the runtime override.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|World",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Set Day Night Cycle World Time Policy Override"
		))
	static bool SetDayNightCycleWorldTimePolicyOverride(
		const UObject* WorldContextObject,
		EDiurnalCycleWorldTimePolicy Policy);

	/** Clears the transient policy override for the current world. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|World",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Clear Day Night Cycle World Time Policy Override"
		))
	static bool ClearDayNightCycleWorldTimePolicyOverride(
		const UObject* WorldContextObject);

#pragma endregion

#pragma region Events

	/** Returns a copy of the current runtime event schedule. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Events"
		))
	static TArray<FDiurnalTimeEvent> GetDayNightCycleEvents(
		const UObject* WorldContextObject);

	/** Finds the runtime event identified by EventTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Event"
		))
	static bool GetDayNightCycleEvent(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag,
		FDiurnalTimeEvent& OutTimeEvent);

	/** Returns whether the runtime event schedule contains EventTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Has Day Night Cycle Event"
		))
	static bool HasDayNightCycleEvent(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag);

	/** Adds a validated event with a unique gameplay tag. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Add Day Night Cycle Event"
		))
	static bool AddDayNightCycleEvent(
		const UObject* WorldContextObject,
		const FDiurnalTimeEvent& TimeEvent);

	/** Removes the event identified by EventTag. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Remove Day Night Cycle Event"
		))
	static bool RemoveDayNightCycleEvent(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag);

	/**
	 * Finds the next scheduled occurrence strictly after the current clock time.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Next Day Night Cycle Event"
		))
	static bool GetNextDayNightCycleEvent(
		const UObject* WorldContextObject,
		FDiurnalTimeEvent& OutTimeEvent,
		FDiurnalDateTime& OutOccurrenceTime);

	/**
	 * Finds the next occurrence of EventTag strictly after the current clock time.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Next Day Night Cycle Event Occurrence"
		))
	static bool GetNextDayNightCycleEventOccurrence(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag,
		FDiurnalDateTime& OutOccurrenceTime);

	/** Creates a recurring daily event definition. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			DisplayName = "Make Daily Time Event"
		))
	static FDiurnalTimeEvent MakeDailyTimeEvent(
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag,
		const FDiurnalTimeOfDay& TimeOfDay,
		EDiurnalTimeEventBehavior Behavior =
			EDiurnalTimeEventBehavior::Notify);

	/** Creates an event definition that occurs only on EventDay. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			DisplayName = "Make Dated Time Event"
		))
	static FDiurnalTimeEvent MakeDatedTimeEvent(
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag EventTag,
		int32 EventDay,
		const FDiurnalTimeOfDay& TimeOfDay,
		EDiurnalTimeEventBehavior Behavior =
			EDiurnalTimeEventBehavior::Notify);

	/** Returns whether an event definition satisfies runtime requirements. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Events",
		meta = (
			DisplayName = "Is Day Night Cycle Event Valid"
		))
	static bool IsDayNightCycleEventValid(
		const FDiurnalTimeEvent& TimeEvent);

#pragma endregion

#pragma region TimeRanges

	/** Returns a copy of the current runtime time-range schedule. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Time Ranges"
		))
	static TArray<FDiurnalTimeRange>
	GetDayNightCycleTimeRanges(
		const UObject* WorldContextObject);

	/** Finds the runtime range identified by RangeTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Time Range"
		))
	static bool GetDayNightCycleTimeRange(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag,
		FDiurnalTimeRange& OutTimeRange);

	/** Returns whether the runtime time-range schedule contains RangeTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Has Day Night Cycle Time Range"
		))
	static bool HasDayNightCycleTimeRange(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag);

	/** Adds a validated range with a unique gameplay tag. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Add Day Night Cycle Time Range"
		))
	static bool AddDayNightCycleTimeRange(
		const UObject* WorldContextObject,
		const FDiurnalTimeRange& TimeRange);

	/** Removes the range identified by RangeTag. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Remove Day Night Cycle Time Range"
		))
	static bool RemoveDayNightCycleTimeRange(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag);

	/** Returns whether TimeOfDay lies inside the range identified by RangeTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Time Of Day In Range"
		))
	static bool IsTimeOfDayInRange(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag,
		const FDiurnalTimeOfDay& TimeOfDay);

	/** Returns whether the current clock time lies inside RangeTag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Current Time In Range"
		))
	static bool IsCurrentTimeInRange(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag);

	/** Returns all ranges active at the current clock time. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Active Day Night Cycle Time Ranges"
		))
	static TArray<FGameplayTag>
	GetActiveDayNightCycleTimeRanges(
		const UObject* WorldContextObject);

	/** Constructs a recurring time-range definition. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			DisplayName = "Make Day Night Cycle Time Range"
		))
	static FDiurnalTimeRange MakeDayNightCycleTimeRange(
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeRange"
			))
		FGameplayTag RangeTag,
		const FDiurnalTimeOfDay& StartTime,
		const FDiurnalTimeOfDay& EndTime);

	/** Returns whether a time-range definition is valid. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Ranges",
		meta = (
			DisplayName = "Is Day Night Cycle Time Range Valid"
		))
	static bool IsDayNightCycleTimeRangeValid(
		const FDiurnalTimeRange& TimeRange);

#pragma endregion

#pragma region TimeGates

	/** Returns whether one or more time gates currently block advancement. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Gates",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Day Night Cycle Blocked By Time Gate"
		))
	static bool IsDayNightCycleBlockedByTimeGate(
		const UObject* WorldContextObject);

	/** Returns every currently active time-gate tag. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Gates",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Active Day Night Cycle Time Gates"
		))
	static TArray<FGameplayTag> GetActiveDayNightCycleTimeGates(
		const UObject* WorldContextObject);

	/** Returns whether GateTag is currently active. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time Gates",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Is Day Night Cycle Time Gate Active"
		))
	static bool IsDayNightCycleTimeGateActive(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag GateTag);

	/** Releases one active time gate. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Time Gates",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Release Day Night Cycle Time Gate"
		))
	static bool ReleaseDayNightCycleTimeGate(
		const UObject* WorldContextObject,
		UPARAM(
			meta = (
				Categories = "DiurnalCycle.TimeEvent"
			))
		FGameplayTag GateTag);

	/** Releases every currently active time gate and returns the number released. */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Time Gates",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Release All Day Night Cycle Time Gates"
		))
	static int32 ReleaseAllDayNightCycleTimeGates(
		const UObject* WorldContextObject);

#pragma endregion

#pragma region Persistence

	/**
	 * Captures the mutable runtime clock state.
	 *
	 * Returns false and resets OutState when the clock is unavailable.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Persistence",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Capture Day Night Cycle State"
		))
	static bool CaptureDayNightCycleState(
		const UObject* WorldContextObject,
		FDiurnalCycleState& OutState);

	/**
	 * Restores a captured state without replaying ordinary event occurrences.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Day Night Cycle|Persistence",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Restore Day Night Cycle State"
		))
	static bool RestoreDayNightCycleState(
		const UObject* WorldContextObject,
		const FDiurnalCycleState& State);

#pragma endregion

#pragma region Notifications

	/**
	 * Returns the persistent Blueprint notification source for this game instance.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Notifications",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Get Day Night Cycle Notifications"
		))
	static UDiurnalCycleBlueprintSubsystem*
	GetDayNightCycleNotifications(
		const UObject* WorldContextObject);

#pragma endregion

#pragma region TimeUtilities

	/** Returns midnight, 00:00:00. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get 00:00 Time Of Day",
			CompactNodeTitle = "00:00"
		))
	static FDiurnalTimeOfDay GetMidnightTimeOfDay();

	/** Returns 06:00:00. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get 06:00 Time Of Day",
			CompactNodeTitle = "06:00"
		))
	static FDiurnalTimeOfDay GetSixAMTimeOfDay();

	/** Returns 12:00:00. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get 12:00 Time Of Day",
			CompactNodeTitle = "12:00"
		))
	static FDiurnalTimeOfDay GetNoonTimeOfDay();

	/** Returns 18:00:00. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get 18:00 Time Of Day",
			CompactNodeTitle = "18:00"
		))
	static FDiurnalTimeOfDay GetSixPMTimeOfDay();

	/** Extracts the time-of-day portion of DateTime. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Time Of Day From Date Time"
		))
	static FDiurnalTimeOfDay GetTimeOfDayFromDateTime(
		const FDiurnalDateTime& DateTime);

	/** Replaces the time-of-day portion while preserving the day number. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Set Date Time Time Of Day"
		))
	static FDiurnalDateTime SetDateTimeTimeOfDay(
		const FDiurnalDateTime& DateTime,
		const FDiurnalTimeOfDay& TimeOfDay);

	/** Returns whether every date-time component is valid. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Is Day Night Cycle Date Time Valid"
		))
	static bool IsDayNightCycleDateTimeValid(
		const FDiurnalDateTime& DateTime);

	/** Returns whether every time-of-day component is valid. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Is Time Of Day Valid"
		))
	static bool IsTimeOfDayValid(
		const FDiurnalTimeOfDay& TimeOfDay);

	/** Returns whether both absolute timestamps are equal. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Are Date Times Equal",
			CompactNodeTitle = "=="
		))
	static bool AreDateTimesEqual(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right);

	/** Returns whether Left occurs before Right. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Is Date Time Before",
			CompactNodeTitle = "<"
		))
	static bool IsDateTimeBefore(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right);

	/** Returns whether Left occurs after Right. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Is Date Time After",
			CompactNodeTitle = ">"
		))
	static bool IsDateTimeAfter(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right);

	/** Returns whether CurrentDateTime has reached or passed TargetDateTime. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Has Date Time Passed"
		))
	static bool HasDateTimePassed(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalDateTime& TargetDateTime);

	/**
	 * Returns signed game seconds from FromDateTime to ToDateTime.
	 *
	 * The result is negative when ToDateTime occurs before FromDateTime.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Seconds Between Date Times"
		))
	static int64 GetGameSecondsBetweenDateTimes(
		const FDiurnalDateTime& FromDateTime,
		const FDiurnalDateTime& ToDateTime);

	/** Returns signed game hours from FromDateTime to ToDateTime. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Hours Between Date Times"
		))
	static double GetGameHoursBetweenDateTimes(
		const FDiurnalDateTime& FromDateTime,
		const FDiurnalDateTime& ToDateTime);

	/** Returns game seconds remaining until TargetDateTime, clamped to zero. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Seconds Remaining"
		))
	static int64 GetGameSecondsRemaining(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalDateTime& TargetDateTime);

	/** Returns game hours remaining until TargetDateTime, clamped to zero. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Hours Remaining"
		))
	static double GetGameHoursRemaining(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalDateTime& TargetDateTime);

	/** Returns whether TargetTime has been reached during the current day. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Has Time Of Day Passed Today"
		))
	static bool HasTimeOfDayPassedToday(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalTimeOfDay& TargetTime);

	/**
	 * Returns game seconds until the next occurrence of TargetTime.
	 *
	 * The result wraps across midnight and is zero when the current time exactly
	 * matches TargetTime.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Seconds Until Time Of Day"
		))
	static int32 GetGameSecondsUntilTimeOfDay(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalTimeOfDay& TargetTime);

	/** Returns game hours until the next occurrence of TargetTime. */
	UFUNCTION(
		BlueprintPure,
		Category = "Day Night Cycle|Time",
		meta = (
			DisplayName = "Get Game Hours Until Time Of Day"
		))
	static double GetGameHoursUntilTimeOfDay(
		const FDiurnalDateTime& CurrentDateTime,
		const FDiurnalTimeOfDay& TargetTime);

#pragma endregion

private:
#pragma region Resolution

	static UDiurnalCycleSubsystem* ResolveSubsystem(
		const UObject* WorldContextObject);

	static UDiurnalCycleWorldSubsystem* ResolveWorldSubsystem(
		const UObject* WorldContextObject);

#pragma endregion
};