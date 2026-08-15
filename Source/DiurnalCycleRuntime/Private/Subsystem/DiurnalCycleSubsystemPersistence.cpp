#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

namespace
{
	bool IsGateValidAtSavedTime(
		const FDiurnalTimeEvent& Event,
		const double TotalGameHours)
	{
		if (!Event.IsBlocking()
			|| !DiurnalCycle::IsValidTotalGameHours(
				TotalGameHours))
		{
			return false;
		}

		const FDiurnalDateTime DateTime =
			FDiurnalDateTime::FromTotalHours(
				TotalGameHours);

		if (!Event.OccursOnDay(
				DateTime.Day)
			|| Event.TimeOfDay
				!= DateTime.GetTimeOfDay())
		{
			return false;
		}

		/*
		 * An active gate can only exist at its exact whole-second occurrence.
		 * This rejects malformed save data positioned fractionally after it.
		 */
		return FMath::IsNearlyEqual(
			TotalGameHours,
			DateTime.ToTotalHours(),
			1.0e-9);
	}
}

#pragma region Capture

FDiurnalCycleState UDiurnalCycleSubsystem::CaptureState() const
{
	FDiurnalCycleState State;

	State.Version =
		DiurnalCycle::GCurrentStateVersion;

	State.TotalGameHours =
		TotalGameHours;

	State.TimeScale =
		TimeScale;

	State.TimeEvents =
		TimeEvents;

	State.TimeRanges =
		TimeRanges;

	State.ActiveTimeGates =
		ActiveTimeGates;

	return State;
}

#pragma endregion

#pragma region Restore

bool UDiurnalCycleSubsystem::TryRestoreState(
	const FDiurnalCycleState& State)
{
	if (State.Version
		!= DiurnalCycle::GCurrentStateVersion)
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected diurnal state version %d. "
				"Expected version %d."),
			State.Version,
			DiurnalCycle::GCurrentStateVersion);

		return false;
	}

	if (!DiurnalCycle::IsValidTotalGameHours(
			State.TotalGameHours))
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected diurnal state with invalid "
				"total game hours: %g."),
			State.TotalGameHours);

		return false;
	}

	if (!DiurnalCycle::IsValidTimeScale(
			State.TimeScale))
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected diurnal state with invalid "
				"time scale: %g."),
			State.TimeScale);

		return false;
	}

	TSet<FGameplayTag> SeenEventTags;

	for (int32 Index = 0;
		 Index < State.TimeEvents.Num();
		 ++Index)
	{
		const FDiurnalTimeEvent& Event =
			State.TimeEvents[Index];

		if (!Event.IsValid())
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because "
					"TimeEvents[%d] is invalid."),
				Index);

			return false;
		}

		if (SeenEventTags.Contains(
				Event.EventTag))
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because event "
					"tag '%s' occurs more than once."),
				*Event.EventTag.ToString());

			return false;
		}

		SeenEventTags.Add(
			Event.EventTag);
	}

	TSet<FGameplayTag> SeenRangeTags;

	for (int32 Index = 0;
		 Index < State.TimeRanges.Num();
		 ++Index)
	{
		const FDiurnalTimeRange& Range =
			State.TimeRanges[Index];

		if (!Range.IsValid())
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because "
					"TimeRanges[%d] is invalid."),
				Index);

			return false;
		}

		if (SeenRangeTags.Contains(
				Range.RangeTag))
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because time range "
					"tag '%s' occurs more than once."),
				*Range.RangeTag.ToString());

			return false;
		}

		SeenRangeTags.Add(
			Range.RangeTag);
	}

	TSet<FGameplayTag> SeenActiveGateTags;

	for (int32 Index = 0;
		 Index < State.ActiveTimeGates.Num();
		 ++Index)
	{
		const FGameplayTag GateTag =
			State.ActiveTimeGates[Index];

		if (!GateTag.IsValid())
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because "
					"ActiveTimeGates[%d] is invalid."),
				Index);

			return false;
		}

		if (SeenActiveGateTags.Contains(
				GateTag))
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because active "
					"time gate '%s' occurs more than once."),
				*GateTag.ToString());

			return false;
		}

		const FDiurnalTimeEvent* GateEvent =
			State.TimeEvents.FindByPredicate(
				[GateTag](
					const FDiurnalTimeEvent& Event)
				{
					return Event.EventTag
						== GateTag;
				});

		if (!GateEvent
			|| !IsGateValidAtSavedTime(
				*GateEvent,
				State.TotalGameHours))
		{
			UE_LOG(
				LogDiurnalCycle,
				Warning,
				TEXT(
					"Rejected diurnal state because active "
					"time gate '%s' does not identify a blocking "
					"event at the saved timestamp."),
				*GateTag.ToString());

			return false;
		}

		SeenActiveGateTags.Add(
			GateTag);
	}

	const double PreviousHours =
		TotalGameHours;

	const double PreviousTimeScale =
		TimeScale;

	const FDiurnalTimeOfDay PreviousTimeOfDay =
		GetTimeOfDay();

	const TArray<FDiurnalTimeRange>
		PreviousActiveRanges =
			GetActiveTimeRangeDefinitions(
				PreviousTimeOfDay);

	/*
	 * Copy before assignment so validation remains atomic and callback-driven
	 * mutations cannot alias the supplied state.
	 */
	TArray<FDiurnalTimeEvent> RestoredEvents =
		State.TimeEvents;

	TArray<FDiurnalTimeRange> RestoredRanges =
		State.TimeRanges;

	TotalGameHours =
		State.TotalGameHours;

	TimeScale =
		State.TimeScale;

	TimeEvents =
		MoveTemp(
			RestoredEvents);

	TimeRanges =
		MoveTemp(
			RestoredRanges);

	SortTimeEvents();

	TArray<FGameplayTag> RestoredActiveGates;

	RestoredActiveGates.Reserve(
		State.ActiveTimeGates.Num());

	/*
	 * Normalize active-gate order to the restored runtime event schedule while
	 * preserving the saved active subset.
	 */
	for (const FDiurnalTimeEvent& Event :
		 TimeEvents)
	{
		if (State.ActiveTimeGates.Contains(
				Event.EventTag))
		{
			RestoredActiveGates.Add(
				Event.EventTag);
		}
	}

	LastBroadcastGameSecond =
		static_cast<int64>(
			FMath::Floor(
				TotalGameHours
					* DiurnalCycle::GSecondsPerHour));

	const FDiurnalDateTime RestoredDateTime =
		GetDateTime();

	/*
	 * Gate state is restored from the save, not inferred from the timestamp.
	 * A blocking event at this exact time may already have been released before
	 * the save was captured.
	 */
	SetActiveTimeGates(
		RestoredActiveGates,
		RestoredDateTime,
		true);

	if (PreviousTimeScale
		!= TimeScale)
	{
		TimeScaleChangedEvent.Broadcast(
			PreviousTimeScale,
			TimeScale);
	}

	const TArray<FDiurnalTimeRange>
		CurrentActiveRanges =
			GetActiveTimeRangeDefinitions(
				RestoredDateTime.GetTimeOfDay());

	BroadcastTimeRangeTransitions(
		PreviousActiveRanges,
		CurrentActiveRanges,
		RestoredDateTime);

	BroadcastTimeChanged(
		PreviousHours,
		TotalGameHours,
		EDiurnalTimeChangeReason::StateRestored);

	UE_LOG(
		LogDiurnalCycle,
		Log,
		TEXT(
			"Restored clock state at %s | Scale: %gx | "
			"Paused: %s | Events: %d | Ranges: %d | "
			"Active gates: %d."),
		*RestoredDateTime.ToString(),
		TimeScale,
		bPaused
			? TEXT("yes")
			: TEXT("no"),
		TimeEvents.Num(),
		TimeRanges.Num(),
		ActiveTimeGates.Num());

	return true;
}

#pragma endregion
