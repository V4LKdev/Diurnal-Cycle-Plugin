#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "DiurnalCycleTypes.generated.h"

#pragma region ConstantsAndValidation

namespace DiurnalCycle
{
	/** Current serialized FDiurnalCycleState layout. */
	constexpr int32 GCurrentStateVersion = 4;

	constexpr double GMinutesPerHour = 60.0;
	constexpr double GSecondsPerHour = 3600.0;
	constexpr double GHoursPerDay = 24.0;

	constexpr int64 GSecondsPerMinute = 60;
	constexpr int64 GSecondsPerHourInteger = 3600;
	constexpr int64 GSecondsPerDay = 86400;

	// Configuration defaults.
	constexpr double GDefaultRealSecondsPerGameHour = 60.0;
	constexpr double GDefaultTimeScale = 1.0;

	// Supported runtime ranges.
	constexpr double GMinimumRealSecondsPerGameHour = 0.001;
	constexpr double GMinimumTimeScale = 0.0;
	constexpr double GMaximumTimeScale = 4096.0;

	constexpr int64 MaximumTotalGameSeconds =
		static_cast<int64>(MAX_int32)
		* GSecondsPerDay
		- 1;

	constexpr double MaximumTotalGameHours =
		static_cast<double>(MaximumTotalGameSeconds)
		/ GSecondsPerHour;

	/** Returns whether Value is a supported base clock rate. */
	inline bool IsValidRealSecondsPerGameHour(
		const double Value)
	{
		return FMath::IsFinite(Value)
			&& Value >= GMinimumRealSecondsPerGameHour;
	}

	/** Returns whether Value is a supported time-scale multiplier. */
	inline bool IsValidTimeScale(
		const double Value)
	{
		return FMath::IsFinite(Value)
			&& Value >= GMinimumTimeScale
			&& Value <= GMaximumTimeScale;
	}

	/** Returns whether Value can be represented by FDiurnalDateTime. */
	inline bool IsValidTotalGameHours(
		const double Value)
	{
		return FMath::IsFinite(Value)
			&& Value >= 0.0
			&& Value <= MaximumTotalGameHours;
	}
}

#pragma endregion

#pragma region TimeOfDay

/**
 * Time within a game day without an associated day number.
 *
 * Use this type for recurring daily times such as 06:00 or 18:00. Use
 * FDiurnalDateTime when referring to one absolute point on the game timeline.
 */
USTRUCT(
	BlueprintType,
	DisplayName = "Day Night Cycle Time Of Day")
struct DIURNALCYCLERUNTIME_API FDiurnalTimeOfDay
{
	GENERATED_BODY()

	FDiurnalTimeOfDay() = default;

	explicit FDiurnalTimeOfDay(
		const int32 InHour,
		const int32 InMinute = 0,
		const int32 InSecond = 0)
		: Hour(InHour)
		, Minute(InMinute)
		, Second(InSecond)
	{
	}

	/** Whole hour in the range [0, 23]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "23",
			UIMin = "0",
			UIMax = "23"
		))
	int32 Hour = 0;

	/** Whole minute in the range [0, 59]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "59",
			UIMin = "0",
			UIMax = "59"
		))
	int32 Minute = 0;

	/** Whole second in the range [0, 59]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "59",
			UIMin = "0",
			UIMax = "59"
		))
	int32 Second = 0;

	/** Returns whether every component is within its supported range. */
	bool IsValid() const
	{
		return Hour >= 0
			&& Hour < 24
			&& Minute >= 0
			&& Minute < 60
			&& Second >= 0
			&& Second < 60;
	}

	/** Returns the whole-second offset from midnight. */
	int32 ToSecondsIntoDay() const
	{
		checkf(
			IsValid(),
			TEXT(
				"Invalid time of day: "
				"Hour=%d Minute=%d Second=%d"),
			Hour,
			Minute,
			Second);

		return Hour
				* static_cast<int32>(
					DiurnalCycle::GSecondsPerHourInteger)
			+ Minute
				* static_cast<int32>(
					DiurnalCycle::GSecondsPerMinute)
			+ Second;
	}

	/** Returns the fractional hour in the range [0, 24). */
	double ToHours() const
	{
		return static_cast<double>(
				ToSecondsIntoDay())
			/ DiurnalCycle::GSecondsPerHour;
	}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("%02d:%02d:%02d"),
			Hour,
			Minute,
			Second);
	}

	// Helpful Template Times
	static FDiurnalTimeOfDay Midnight()
	{
		return FDiurnalTimeOfDay(0);
	}

	static FDiurnalTimeOfDay SixAM()
	{
		return FDiurnalTimeOfDay(6);
	}

	static FDiurnalTimeOfDay Noon()
	{
		return FDiurnalTimeOfDay(12);
	}

	static FDiurnalTimeOfDay SixPM()
	{
		return FDiurnalTimeOfDay(18);
	}

	// Time Operations
	friend bool operator==(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return Left.Hour == Right.Hour
			&& Left.Minute == Right.Minute
			&& Left.Second == Right.Second;
	}

	friend bool operator!=(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return !(Left == Right);
	}

	friend bool operator<(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return Left.ToSecondsIntoDay()
			< Right.ToSecondsIntoDay();
	}

	friend bool operator<=(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return !(Right < Left);
	}

	friend bool operator>(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return Right < Left;
	}

	friend bool operator>=(
		const FDiurnalTimeOfDay& Left,
		const FDiurnalTimeOfDay& Right)
	{
		return !(Left < Right);
	}
};

#pragma endregion

#pragma region TimeRanges

/**
 * Gameplay-tagged recurring range within a game day.
 *
 * Ranges are start-inclusive and end-exclusive: [StartTime, EndTime).
 * When EndTime is earlier than StartTime, the range wraps across midnight.
 */
USTRUCT(
	BlueprintType,
	DisplayName = "Day Night Cycle Time Range")
struct DIURNALCYCLERUNTIME_API FDiurnalTimeRange
{
	GENERATED_BODY()

	FDiurnalTimeRange() = default;

	FDiurnalTimeRange(
		const FGameplayTag InRangeTag,
		const FDiurnalTimeOfDay& InStartTime,
		const FDiurnalTimeOfDay& InEndTime)
		: RangeTag(InRangeTag)
		, StartTime(InStartTime)
		, EndTime(InEndTime)
	{
	}

	/** Semantic identifier used by gameplay systems. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (
			Categories = "DiurnalCycle.TimeRange"
		))
	FGameplayTag RangeTag;

	/** Inclusive start of the range. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	FDiurnalTimeOfDay StartTime;

	/** Exclusive end of the range. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	FDiurnalTimeOfDay EndTime;

	/** Returns whether this range can be added to the runtime schedule. */
	bool IsValid() const
	{
		return RangeTag.IsValid()
			&& StartTime.IsValid()
			&& EndTime.IsValid()
			&& StartTime != EndTime;
	}

	/**
	 * Returns whether TimeOfDay lies within this range.
	 *
	 * Midnight-wrapping ranges are handled automatically.
	 */
	bool Contains(
		const FDiurnalTimeOfDay& TimeOfDay) const
	{
		if (!IsValid()
			|| !TimeOfDay.IsValid())
		{
			return false;
		}

		const int32 TimeSeconds =
			TimeOfDay.ToSecondsIntoDay();

		const int32 StartSeconds =
			StartTime.ToSecondsIntoDay();

		const int32 EndSeconds =
			EndTime.ToSecondsIntoDay();

		if (StartSeconds < EndSeconds)
		{
			return TimeSeconds >= StartSeconds
				&& TimeSeconds < EndSeconds;
		}

		return TimeSeconds >= StartSeconds
			|| TimeSeconds < EndSeconds;
	}
};

#pragma endregion

#pragma region DateTime

/**
 * Calendar-like game timestamp.
 *
 * Day numbering begins at one. The type represents an elapsed game day and time of day.
 */
USTRUCT(
	BlueprintType,
	DisplayName = "Day Night Cycle Date Time")
struct DIURNALCYCLERUNTIME_API FDiurnalDateTime
{
	GENERATED_BODY()

	FDiurnalDateTime() = default;

	FDiurnalDateTime(
		const int32 InDay,
		const int32 InHour,
		const int32 InMinute = 0,
		const int32 InSecond = 0)
		: Day(InDay)
		, Hour(InHour)
		, Minute(InMinute)
		, Second(InSecond)
	{
	}

	FDiurnalDateTime(
		const int32 InDay,
		const FDiurnalTimeOfDay& InTimeOfDay)
		: Day(InDay)
		, Hour(InTimeOfDay.Hour)
		, Minute(InTimeOfDay.Minute)
		, Second(InTimeOfDay.Second)
	{
	}

	/** One-based game day. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "1",
			UIMin = "1"
		))
	int32 Day = 1;

	/** Whole hour of the day in the range [0, 23]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "23",
			UIMin = "0",
			UIMax = "23"
		))
	int32 Hour = 0;

	/** Whole minute of the hour in the range [0, 59]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "59",
			UIMin = "0",
			UIMax = "59"
		))
	int32 Minute = 0;

	/** Whole second of the minute in the range [0, 59]. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Day Night Cycle",
		meta = (
			ClampMin = "0",
			ClampMax = "59",
			UIMin = "0",
			UIMax = "59"
		))
	int32 Second = 0;

	/** Returns whether every component is within its supported range. */
	bool IsValid() const
	{
		return Day >= 1
			&& Hour >= 0
			&& Hour < 24
			&& Minute >= 0
			&& Minute < 60
			&& Second >= 0
			&& Second < 60;
	}

	/** Returns elapsed game hours since Day 1 at 00:00. */
	double ToTotalHours() const
	{
		return static_cast<double>(
				ToTotalSeconds())
			/ DiurnalCycle::GSecondsPerHour;
	}

	/** Returns elapsed whole game seconds since Day 1 at 00:00. */
	int64 ToTotalSeconds() const
	{
		checkf(
			IsValid(),
			TEXT(
				"Invalid day-night-cycle date-time: "
				"Day=%d Hour=%d Minute=%d Second=%d"),
			Day,
			Hour,
			Minute,
			Second);

		return static_cast<int64>(Day - 1)
				* DiurnalCycle::GSecondsPerDay
			+ static_cast<int64>(Hour)
				* DiurnalCycle::GSecondsPerHourInteger
			+ static_cast<int64>(Minute)
				* DiurnalCycle::GSecondsPerMinute
			+ static_cast<int64>(Second);
	}

	/**
	 * Creates a timestamp from elapsed game hours since Day 1 at 00:00.
	 *
	 * Sub-second precision is truncated toward the previous whole game second.
	 */
	static FDiurnalDateTime FromTotalHours(
		const double TotalHours)
	{
		checkf(
			DiurnalCycle::IsValidTotalGameHours(
				TotalHours),
			TEXT(
				"Invalid total game hours: %g"),
			TotalHours);

		const int64 TotalSeconds =
			static_cast<int64>(
				FMath::Floor(
					TotalHours
					* DiurnalCycle::GSecondsPerHour));

		return FromTotalSeconds(
			TotalSeconds);
	}

	/**
	 * Creates a timestamp from elapsed whole game seconds since Day 1 at 00:00.
	 */
	static FDiurnalDateTime FromTotalSeconds(
		const int64 TotalSeconds)
	{
		checkf(
			TotalSeconds >= 0
				&& TotalSeconds
					<= DiurnalCycle::MaximumTotalGameSeconds,
			TEXT(
				"Invalid total game seconds: %lld"),
			TotalSeconds);

		const int64 DayIndex =
			TotalSeconds
			/ DiurnalCycle::GSecondsPerDay;

		const int64 SecondsIntoDay =
			TotalSeconds
			% DiurnalCycle::GSecondsPerDay;

		FDiurnalDateTime Result;

		Result.Day =
			static_cast<int32>(
				DayIndex + 1);

		Result.Hour =
			static_cast<int32>(
				SecondsIntoDay
				/ DiurnalCycle::GSecondsPerHourInteger);

		Result.Minute =
			static_cast<int32>(
				(SecondsIntoDay
					% DiurnalCycle::GSecondsPerHourInteger)
				/ DiurnalCycle::GSecondsPerMinute);

		Result.Second =
			static_cast<int32>(
				SecondsIntoDay
				% DiurnalCycle::GSecondsPerMinute);

		return Result;
	}

	/** Returns the time-of-day portion of this timestamp. */
	FDiurnalTimeOfDay GetTimeOfDay() const
	{
		return FDiurnalTimeOfDay(
			Hour,
			Minute,
			Second);
	}

	/** Returns this timestamp with its time-of-day portion replaced. */
	FDiurnalDateTime WithTimeOfDay(
		const FDiurnalTimeOfDay& TimeOfDay) const
	{
		checkf(
			TimeOfDay.IsValid(),
			TEXT(
				"WithTimeOfDay requires a valid time of day."));

		return FDiurnalDateTime(
			Day,
			TimeOfDay);
	}

	/** Returns a compact Day N, HH:MM:SS representation. */
	FString ToString() const
	{
		return FString::Printf(
			TEXT(
				"Day %d, %02d:%02d:%02d"),
			Day,
			Hour,
			Minute,
			Second);
	}

	friend bool operator==(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return Left.Day == Right.Day
			&& Left.Hour == Right.Hour
			&& Left.Minute == Right.Minute
			&& Left.Second == Right.Second;
	}

	friend bool operator!=(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return !(Left == Right);
	}

	friend bool operator<(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return Left.ToTotalSeconds()
			< Right.ToTotalSeconds();
	}

	friend bool operator<=(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return !(Right < Left);
	}

	friend bool operator>(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return Right < Left;
	}

	friend bool operator>=(
		const FDiurnalDateTime& Left,
		const FDiurnalDateTime& Right)
	{
		return !(Left < Right);
	}
};

#pragma endregion

#pragma region TimeEvents

/**
 * Determines the runtime consequence of a scheduled occurrence.
 *
 * Blocking events remain ordinary events: their occurrence notification still
 * fires, but reaching them additionally creates a persistent time-gate block.
 */
UENUM(BlueprintType)
enum class EDiurnalTimeEventBehavior : uint8
{
	Notify UMETA(DisplayName = "Notify"),
	BlockTime UMETA(DisplayName = "Block Time")
};

/**
 * Gameplay-tagged scheduled occurrence.
 *
 * Daily events occur once on every crossed game day. Dated events occur only
 * on EventDay. Ordinary event occurrences are emitted only during forward
 * advancement; teleporting or restoring state does not replay them.
 *
 * BlockTime events additionally act as time gates. Multiple gates may become
 * active at the same timestamp and remain active until explicitly released.
 */
USTRUCT(
	BlueprintType,
	DisplayName = "Day Night Cycle Event")
struct DIURNALCYCLERUNTIME_API FDiurnalTimeEvent
{
	GENERATED_BODY()

	FDiurnalTimeEvent() = default;

	FDiurnalTimeEvent(
		const FGameplayTag InEventTag,
		const FDiurnalTimeOfDay& InTimeOfDay,
		const bool bInDatedEvent = false,
		const int32 InEventDay = 1,
		const EDiurnalTimeEventBehavior InBehavior =
			EDiurnalTimeEventBehavior::Notify)
		: EventTag(InEventTag)
		, TimeOfDay(InTimeOfDay)
		, bDatedEvent(bInDatedEvent)
		, EventDay(InEventDay)
		, Behavior(InBehavior)
	{
	}

	/** Semantic identifier used to route the occurrence to gameplay systems. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (
			Categories = "DiurnalCycle.TimeEvent"
		))
	FGameplayTag EventTag;

	/** Exact time of day at which the occurrence is scheduled. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	FDiurnalTimeOfDay TimeOfDay =
		FDiurnalTimeOfDay::Noon();

	/** Whether this event occurs only on one specific game day. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		AdvancedDisplay)
	bool bDatedEvent = false;

	/** One-based game day used when bDatedEvent is enabled. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (
			EditCondition = "bDatedEvent",
			EditConditionHides,
			ClampMin = "1",
			UIMin = "1"
		))
	int32 EventDay = 1;

	/** Consequence applied when this event occurrence is reached. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	EDiurnalTimeEventBehavior Behavior =
		EDiurnalTimeEventBehavior::Notify;

	/** Returns whether this event can be added to the runtime schedule. */
	bool IsValid() const
	{
		return EventTag.IsValid()
			&& TimeOfDay.IsValid()
			&& (!bDatedEvent
				|| EventDay >= 1);
	}

	/** Returns whether reaching this event creates a time gate. */
	bool IsBlocking() const
	{
		return Behavior
			== EDiurnalTimeEventBehavior::BlockTime;
	}

	/** Returns whether this event has an occurrence on Day. */
	bool OccursOnDay(
		const int32 Day) const
	{
		return Day >= 1
			&& (!bDatedEvent
				|| EventDay == Day);
	}
};

#pragma endregion

#pragma region Persistence

/**
 * Serializable mutable state of a day-night-cycle clock.
 *
 * Store this struct inside a game-owned USaveGame class. The configured base
 * rate and explicit pause state are intentionally not included.
 *
 * Restoring state does not replay ordinary event occurrences. Runtime ranges
 * and active time gates are restored/reconciled as persistent temporal state.
 */
USTRUCT(
	BlueprintType,
	DisplayName = "Day Night Cycle State")
struct DIURNALCYCLERUNTIME_API FDiurnalCycleState
{
	GENERATED_BODY()

	/** State layout version used to reject incompatible future data. */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	int32 Version =
		DiurnalCycle::GCurrentStateVersion;

	/** Exact elapsed game hours, including sub-second precision. */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	double TotalGameHours = 0.0;

	/** Clock-speed multiplier active when the state was captured. */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	double TimeScale =
		DiurnalCycle::GDefaultTimeScale;

	/** Complete mutable runtime event schedule. */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	TArray<FDiurnalTimeEvent> TimeEvents;

	/** Complete mutable runtime time-range schedule. */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	TArray<FDiurnalTimeRange> TimeRanges;

	/**
	 * Unique tags of all time gates currently blocking advancement.
	 *
	 * Every tag must identify a blocking event whose occurrence matches the
	 * saved clock position. An empty array means the clock is not gate-blocked.
	 */
	UPROPERTY(
		BlueprintReadWrite,
		SaveGame,
		Category = "Day Night Cycle")
	TArray<FGameplayTag> ActiveTimeGates;
};

#pragma endregion

#pragma region TimeChangeNotifications

/** Describes how the clock reached a new time. */
UENUM(BlueprintType)
enum class EDiurnalTimeChangeReason : uint8
{
	/** Advanced automatically from the owning world's delta time. */
	AutomaticAdvance UMETA(DisplayName = "Automatic Advance"),

	/** Advanced through TryAdvanceHours(). */
	ManualAdvance UMETA(DisplayName = "Manual Advance"),

	/**
	 * Teleported through TrySetDateTime() without replaying ordinary scheduled
	 * occurrences.
	 */
	DateTimeSet UMETA(DisplayName = "Date Time Set"),

	/**
	 * Replaced through TryRestoreState() without replaying ordinary scheduled
	 * occurrences.
	 */
	StateRestored UMETA(DisplayName = "State Restored")
};

/** Describes one externally observable clock change. */
struct DIURNALCYCLERUNTIME_API FDiurnalTimeChange
{
	/** Clock value before the operation. */
	FDiurnalDateTime PreviousDateTime;

	/** Clock value after the operation. */
	FDiurnalDateTime CurrentDateTime;

	/** Elapsed game hours before the operation. */
	double PreviousTotalGameHours = 0.0;

	/** Elapsed game hours after the operation. */
	double CurrentTotalGameHours = 0.0;

	/** Operation that produced this change. */
	EDiurnalTimeChangeReason Reason =
		EDiurnalTimeChangeReason::AutomaticAdvance;
};

#pragma endregion
