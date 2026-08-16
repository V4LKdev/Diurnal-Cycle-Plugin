#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "DiurnalCycleTypes.generated.h"

class UDiurnalSchedule;

#pragma region ConstantsAndValidation

namespace DiurnalCycle
{
	/** First public persistence schema for each independently restorable state. */
	constexpr int32 GCurrentStateVersion = 1;
	constexpr int32 GCurrentClockStateVersion = 1;
	constexpr int32 GCurrentScheduleStateVersion = 1;

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

#pragma region ScheduleIdentity

/**
 * Stable reference to one schedule entry.
 *
 * Authored entries are identified by schedule asset plus EntryId. Dynamic
 * runtime entries have no schedule asset and are identified by their
 * session-stable EntryId. Gameplay Tags are deliberately not part of identity.
 */
USTRUCT(BlueprintType, DisplayName = "Day Night Cycle Schedule Entry Reference")
struct DIURNALCYCLERUNTIME_API FDiurnalScheduleEntryReference
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Day Night Cycle")
	TSoftObjectPtr<UDiurnalSchedule> Schedule;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Day Night Cycle")
	FGuid EntryId;

	bool IsValid() const
	{
		// EntryId is the required exact identity. A null Schedule is intentional
		// for runtime-owned entries. Schedule presence alone never makes this valid.
		return EntryId.IsValid();
	}

	bool IsAuthoredAssetEntry() const
	{
		return !Schedule.IsNull();
	}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("%s:%s"),
			Schedule.IsNull()
				? TEXT("Runtime")
				: *Schedule.ToSoftObjectPath().ToString(),
			*EntryId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	}

	friend bool operator==(
		const FDiurnalScheduleEntryReference& Left,
		const FDiurnalScheduleEntryReference& Right)
	{
		return Left.EntryId == Right.EntryId
			&& Left.Schedule.ToSoftObjectPath()
				== Right.Schedule.ToSoftObjectPath();
	}

	friend bool operator!=(
		const FDiurnalScheduleEntryReference& Left,
		const FDiurnalScheduleEntryReference& Right)
	{
		return !(Left == Right);
	}
};

inline uint32 GetTypeHash(const FDiurnalScheduleEntryReference& Reference)
{
	return HashCombine(
		GetTypeHash(Reference.Schedule.ToSoftObjectPath()),
		GetTypeHash(Reference.EntryId));
}

#pragma endregion

#pragma region TimeOfDay

/**
 * Time within a game day without an associated day number.
 *
 * Use this type for recurring times of day such as 06:00 or 18:00. Use
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

#pragma region Recurrence

/** Whether a schedule entry occurs once or repeats at a day interval. */
UENUM(BlueprintType)
enum class EDiurnalRecurrenceMode : uint8
{
	Once UMETA(DisplayName = "Once"),
	Repeating UMETA(DisplayName = "Every N Days")
};

/** Shared day recurrence used by both instantaneous events and time ranges. */
USTRUCT(BlueprintType, DisplayName = "Day Night Cycle Recurrence")
struct DIURNALCYCLERUNTIME_API FDiurnalRecurrence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day Night Cycle", meta = (DisplayName = "Occurs"))
	EDiurnalRecurrenceMode Mode = EDiurnalRecurrenceMode::Repeating;

	/** Day of the one-off occurrence, or the first repeating occurrence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day Night Cycle", meta = (ClampMin = "1", UIMin = "1"))
	int32 AnchorDay = 1;

	/** Repetition interval in game days. Used only for Repeating. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day Night Cycle", meta = (ClampMin = "1", UIMin = "1", EditCondition = "Mode == EDiurnalRecurrenceMode::Repeating", EditConditionHides))
	int32 IntervalDays = 1;

	static FDiurnalRecurrence Once(const int32 Day)
	{
		FDiurnalRecurrence Result;
		Result.Mode = EDiurnalRecurrenceMode::Once;
		Result.AnchorDay = FMath::Max(1, Day);
		Result.IntervalDays = 1;
		return Result;
	}

	static FDiurnalRecurrence Repeating(const int32 FirstDay = 1, const int32 EveryDays = 1)
	{
		FDiurnalRecurrence Result;
		Result.Mode = EDiurnalRecurrenceMode::Repeating;
		Result.AnchorDay = FMath::Max(1, FirstDay);
		Result.IntervalDays = FMath::Max(1, EveryDays);
		return Result;
	}

	bool IsValid() const
	{
		if (AnchorDay < 1)
		{
			return false;
		}

		switch (Mode)
		{
		case EDiurnalRecurrenceMode::Once:
			return true;
		case EDiurnalRecurrenceMode::Repeating:
			return IntervalDays >= 1;
		default:
			return false;
		}
	}

	bool OccursOnDay(const int32 Day) const
	{
		if (!IsValid() || Day < AnchorDay) return false;
		return Mode == EDiurnalRecurrenceMode::Once
			? Day == AnchorDay
			: (Day - AnchorDay) % IntervalDays == 0;
	}
};

#pragma endregion

#pragma region TimeRanges

/**
 * Named recurring range with optional semantic Gameplay Tags.
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

	/** Stable authored identity used by editor tooling. Not a gameplay key. */
	UPROPERTY()
	FGuid EntryId;

	/** Human-facing authoring label. Never used as a runtime lookup key. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (DisplayName = "Name"))
	FName RangeName;

#if WITH_EDITORONLY_DATA
	/** Uses a custom authored color instead of the configured default. */
	UPROPERTY(EditAnywhere, Category = "Presentation", meta = (DisplayName = "Override Color"))
	bool bOverrideEditorColor = false;

	/** Editor-only planning color. It never participates in runtime behavior. */
	UPROPERTY(
		EditAnywhere,
		Category = "Presentation",
		meta = (EditCondition = "bOverrideEditorColor", DisplayName = "Color"))
	FLinearColor EditorColor = FLinearColor(0.18f, 0.55f, 0.48f, 1.0f);
#endif

	FDiurnalTimeRange() = default;

	FDiurnalTimeRange(
		const FGameplayTag InRangeTag,
		const FDiurnalTimeOfDay& InStartTime,
		const FDiurnalTimeOfDay& InEndTime)
		: RangeName(InRangeTag.IsValid() ? InRangeTag.GetTagLeafName() : NAME_None)
		, StartTime(InStartTime)
		, EndTime(InEndTime)
	{
		if (InRangeTag.IsValid())
		{
			RangeTags.AddTag(InRangeTag);
		}
	}

	/** Optional many-to-many semantic classification and query tags. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (
			DisplayName = "Tags",
			Categories = "DiurnalCycle.TimeRange"
		))
	FGameplayTagContainer RangeTags;

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

	/** Determines on which game days this range begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day Night Cycle")
	FDiurnalRecurrence Recurrence;

	/** Returns whether this range can be added to the runtime schedule. */
	bool IsValid() const
	{
		return StartTime.IsValid()
			&& EndTime.IsValid()
			&& StartTime != EndTime
			&& Recurrence.IsValid();
	}

	bool OccursOnDay(const int32 Day) const
	{
		return Recurrence.OccursOnDay(Day);
	}

	/** First semantic tag, used only for concise presentation. */
	FGameplayTag GetPrimaryTag() const
	{
		return !RangeTags.IsEmpty() ? RangeTags.First() : FGameplayTag();
	}

	/** Exact tag membership. This intentionally does not perform hierarchical matching. */
	bool HasTagExact(const FGameplayTag Tag) const
	{
		return Tag.IsValid() && RangeTags.HasTagExact(Tag);
	}

	/** Presentation label, or a generic label for unnamed transient values. */
	FName GetDisplayName() const
	{
		if (!RangeName.IsNone()) return RangeName;
		const FGameplayTag Tag = GetPrimaryTag();
		return Tag.IsValid() ? Tag.GetTagLeafName() : FName(TEXT("New Range"));
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

	/** Date-aware active check. Wrapped morning segments belong to the preceding start day. */
	bool ContainsOnDay(const int32 Day, const FDiurnalTimeOfDay& TimeOfDay) const
	{
		if (!Contains(TimeOfDay) || Day < 1) return false;
		const bool bWrapsMidnight = StartTime > EndTime;
		if (!bWrapsMidnight || TimeOfDay >= StartTime)
		{
			return OccursOnDay(Day);
		}
		return Day > 1 && OccursOnDay(Day - 1);
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
 * Named scheduled occurrence with optional semantic Gameplay Tags.
 *
 * Repeating events occur at their configured interval. Once events occur only
 * on their anchor day. Ordinary event occurrences are emitted only during forward
 * advancement. Teleporting or restoring state does not replay them.
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

	/** Stable authored identity used by editor tooling. Not a gameplay key. */
	UPROPERTY()
	FGuid EntryId;

	/** Human-facing authoring label. Never used as a runtime lookup key. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (DisplayName = "Name"))
	FName EventName;

#if WITH_EDITORONLY_DATA
	/** Uses a custom authored color instead of the configured default. */
	UPROPERTY(EditAnywhere, Category = "Presentation", meta = (DisplayName = "Override Color"))
	bool bOverrideEditorColor = false;

	/** Editor-only planning color. It never participates in runtime behavior. */
	UPROPERTY(
		EditAnywhere,
		Category = "Presentation",
		meta = (EditCondition = "bOverrideEditorColor", DisplayName = "Color"))
	FLinearColor EditorColor = FLinearColor(0.32f, 0.43f, 0.72f, 1.0f);
#endif

	FDiurnalTimeEvent() = default;

	FDiurnalTimeEvent(
		const FGameplayTag InEventTag,
		const FDiurnalTimeOfDay& InTimeOfDay,
		const FDiurnalRecurrence& InRecurrence = FDiurnalRecurrence::Repeating(),
		const EDiurnalTimeEventBehavior InBehavior =
			EDiurnalTimeEventBehavior::Notify)
		: EventName(InEventTag.IsValid() ? InEventTag.GetTagLeafName() : NAME_None)
		, TimeOfDay(InTimeOfDay)
		, Recurrence(InRecurrence)
		, Behavior(InBehavior)
	{
		if (InEventTag.IsValid())
		{
			EventTags.AddTag(InEventTag);
		}
	}

	/** Optional many-to-many semantic classification and occurrence-routing tags. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle",
		meta = (
			DisplayName = "Tags",
			Categories = "DiurnalCycle.TimeEvent"
		))
	FGameplayTagContainer EventTags;

	/** Exact time of day at which the occurrence is scheduled. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Day Night Cycle")
	FDiurnalTimeOfDay TimeOfDay =
		FDiurnalTimeOfDay::Noon();

	/** Determines whether this event occurs once or repeats every N game days. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Day Night Cycle")
	FDiurnalRecurrence Recurrence;

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
		return TimeOfDay.IsValid()
			&& Recurrence.IsValid();
	}

	/** First semantic tag, used only for concise presentation. */
	FGameplayTag GetPrimaryTag() const
	{
		return !EventTags.IsEmpty() ? EventTags.First() : FGameplayTag();
	}

	/** Exact tag membership. This intentionally does not perform hierarchical matching. */
	bool HasTagExact(const FGameplayTag Tag) const
	{
		return Tag.IsValid() && EventTags.HasTagExact(Tag);
	}

	/** Presentation label, or a generic label for unnamed transient values. */
	FName GetDisplayName() const
	{
		if (!EventName.IsNone()) return EventName;
		const FGameplayTag Tag = GetPrimaryTag();
		return Tag.IsValid() ? Tag.GetTagLeafName() : FName(TEXT("New Event"));
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
		return Recurrence.OccursOnDay(Day);
	}
};

#pragma endregion

#pragma region EventOccurrenceIdentity

/** Exact identity of one emitted event occurrence. */
USTRUCT(BlueprintType, DisplayName = "Day Night Cycle Event Occurrence Handle")
struct DIURNALCYCLERUNTIME_API FDiurnalEventOccurrenceHandle
{
	GENERATED_BODY()

	/** Authored/runtime entry that produced the occurrence. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Day Night Cycle")
	FDiurnalScheduleEntryReference Entry;

	/** Timestamp at which this particular occurrence was emitted. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Day Night Cycle")
	FDiurnalDateTime OccurrenceTime;

	/** Session-unique occurrence identity, required for exact gate acknowledgement. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Day Night Cycle")
	FGuid OccurrenceId;

	bool IsValid() const
	{
		return Entry.IsValid()
			&& OccurrenceTime.IsValid()
			&& OccurrenceId.IsValid();
	}

	friend bool operator==(
		const FDiurnalEventOccurrenceHandle& Left,
		const FDiurnalEventOccurrenceHandle& Right)
	{
		return Left.OccurrenceId == Right.OccurrenceId;
	}

	friend bool operator!=(
		const FDiurnalEventOccurrenceHandle& Left,
		const FDiurnalEventOccurrenceHandle& Right)
	{
		return !(Left == Right);
	}
};

#pragma endregion

#pragma region ResolvedSchedule

/** Runtime event plus the authored layer that contributed it. */
USTRUCT(BlueprintType, DisplayName = "Resolved Day Night Cycle Event")
struct DIURNALCYCLERUNTIME_API FDiurnalResolvedTimeEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	FDiurnalTimeEvent Event;

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	TSoftObjectPtr<UDiurnalSchedule> SourceSchedule;

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	bool bRuntimeAdded = false;

	FDiurnalScheduleEntryReference GetEntryReference() const
	{
		FDiurnalScheduleEntryReference Result;
		Result.Schedule = SourceSchedule;
		Result.EntryId = Event.EntryId;
		return Result;
	}
};

/** Runtime time range plus the authored layer that contributed it. */
USTRUCT(BlueprintType, DisplayName = "Resolved Day Night Cycle Time Range")
struct DIURNALCYCLERUNTIME_API FDiurnalResolvedTimeRange
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	FDiurnalTimeRange Range;

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	TSoftObjectPtr<UDiurnalSchedule> SourceSchedule;

	UPROPERTY(BlueprintReadOnly, Category = "Day Night Cycle")
	bool bRuntimeAdded = false;

	FDiurnalScheduleEntryReference GetEntryReference() const
	{
		FDiurnalScheduleEntryReference Result;
		Result.Schedule = SourceSchedule;
		Result.EntryId = Range.EntryId;
		return Result;
	}
};

#pragma endregion

#pragma region Persistence

/** Independently serializable clock state. */
USTRUCT(BlueprintType, DisplayName = "Day Night Cycle Clock State")
struct DIURNALCYCLERUNTIME_API FDiurnalClockState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	int32 Version = DiurnalCycle::GCurrentClockStateVersion;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	double TotalGameHours = 0.0;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	double TimeScale = DiurnalCycle::GDefaultTimeScale;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	double RealSecondsPerGameHour = DiurnalCycle::GDefaultRealSecondsPerGameHour;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	bool bPaused = false;
};

/** Independently serializable runtime schedule state. */
USTRUCT(BlueprintType, DisplayName = "Day Night Cycle Schedule Runtime State")
struct DIURNALCYCLERUNTIME_API FDiurnalScheduleRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	int32 Version = DiurnalCycle::GCurrentScheduleStateVersion;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<TSoftObjectPtr<UDiurnalSchedule>> ActiveSchedules;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<FDiurnalTimeEvent> RuntimeTimeEvents;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<FDiurnalTimeRange> RuntimeTimeRanges;

	/** Exact authored entries disabled by the mutable runtime overlay. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<FDiurnalScheduleEntryReference> DisabledEventEntries;

	/** Exact authored ranges disabled by the mutable runtime overlay. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<FDiurnalScheduleEntryReference> DisabledRangeEntries;

	/** Exact blocking occurrences that currently hold the clock. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	TArray<FDiurnalEventOccurrenceHandle> ActiveTimeGateOccurrences;

};

/** Complete aggregate state. Clock and schedule can also be restored alone. */
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

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	FDiurnalClockState ClockState;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Day Night Cycle")
	FDiurnalScheduleRuntimeState ScheduleState;

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
