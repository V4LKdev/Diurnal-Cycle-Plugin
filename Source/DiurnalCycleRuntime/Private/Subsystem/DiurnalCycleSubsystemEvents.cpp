#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region EventSchedule

bool UDiurnalCycleSubsystem::TryGetTimeEvent(
	const FGameplayTag EventTag,
	FDiurnalTimeEvent& OutTimeEvent) const
{
	if (!EventTag.IsValid())
	{
		return false;
	}

	const FDiurnalTimeEvent* TimeEvent =
		TimeEvents.FindByPredicate(
			[EventTag](
				const FDiurnalTimeEvent& Candidate)
			{
				return Candidate.EventTag
					== EventTag;
			});

	if (!TimeEvent)
	{
		return false;
	}

	OutTimeEvent =
		*TimeEvent;

	return true;
}

bool UDiurnalCycleSubsystem::HasTimeEvent(
	const FGameplayTag EventTag) const
{
	if (!EventTag.IsValid())
	{
		return false;
	}

	return TimeEvents.ContainsByPredicate(
		[EventTag](
			const FDiurnalTimeEvent& Event)
		{
			return Event.EventTag
				== EventTag;
		});
}

bool UDiurnalCycleSubsystem::TryAddTimeEvent(
	const FDiurnalTimeEvent& TimeEvent)
{
	if (!TimeEvent.IsValid())
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected invalid event: "
				"Tag='%s' Time=%s Dated=%s Day=%d."),
			*TimeEvent.EventTag.ToString(),
			*TimeEvent.TimeOfDay.ToString(),
			TimeEvent.bDatedEvent
				? TEXT("yes")
				: TEXT("no"),
			TimeEvent.EventDay);

		return false;
	}

	if (HasTimeEvent(
			TimeEvent.EventTag))
	{
		UE_LOG(
			LogDiurnalCycle,
			Warning,
			TEXT(
				"Rejected event '%s' because event tags "
				"must be unique."),
			*TimeEvent.EventTag.ToString());

		return false;
	}

	const FDiurnalTimeEvent AddedEvent =
		TimeEvent;

	TimeEvents.Add(
		AddedEvent);

	SortTimeEvents();

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Added event '%s' at %s."),
		*AddedEvent.EventTag.ToString(),
		*AddedEvent.TimeOfDay.ToString());

	TimeEventAddedEvent.Broadcast(
		AddedEvent);

	/*
	 * A blocking event added exactly at the current absolute timestamp becomes
	 * active immediately. Being somewhere within the same displayed second is
	 * not sufficient; the clock must be exactly on the occurrence.
	 */
	const FDiurnalDateTime CurrentDateTime =
		GetDateTime();

	const bool bAtExactWholeSecond =
		FMath::IsNearlyEqual(
			TotalGameHours,
			CurrentDateTime.ToTotalHours(),
			1.0e-9);

	if (AddedEvent.IsBlocking()
		&& bAtExactWholeSecond
		&& AddedEvent.OccursOnDay(
			CurrentDateTime.Day)
		&& AddedEvent.TimeOfDay
			== CurrentDateTime.GetTimeOfDay())
	{
		TArray<FGameplayTag> NewActiveGates =
			ActiveTimeGates;

		NewActiveGates.AddUnique(
			AddedEvent.EventTag);

		SetActiveTimeGates(
			NewActiveGates,
			CurrentDateTime,
			true);
	}

	return true;
}

bool UDiurnalCycleSubsystem::RemoveTimeEvent(
	const FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return false;
	}

	const int32 EventIndex =
		TimeEvents.IndexOfByPredicate(
			[EventTag](
				const FDiurnalTimeEvent& Event)
			{
				return Event.EventTag
					== EventTag;
			});

	if (EventIndex == INDEX_NONE)
	{
		return false;
	}

	const FDiurnalTimeEvent RemovedEvent =
		TimeEvents[EventIndex];

	if (IsTimeGateActive(
			EventTag))
	{
		ReleaseTimeGate(
			EventTag);
	}

	TimeEvents.RemoveAt(
		EventIndex);

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Removed event '%s'."),
		*RemovedEvent.EventTag.ToString());

	TimeEventRemovedEvent.Broadcast(
		RemovedEvent);

	return true;
}

#pragma endregion

#pragma region OccurrenceQueries

bool UDiurnalCycleSubsystem::TryGetNextTimeEvent(
	FDiurnalTimeEvent& OutEvent,
	FDiurnalDateTime& OutOccurrenceTime) const
{
	double BestOccurrenceHours =
		TNumericLimits<double>::Max();

	const FDiurnalTimeEvent* BestEvent =
		nullptr;

	for (const FDiurnalTimeEvent& Event :
		 TimeEvents)
	{
		double OccurrenceHours = 0.0;

		if (!TryGetNextOccurrenceHours(
				Event,
				TotalGameHours,
				OccurrenceHours))
		{
			continue;
		}

		if (OccurrenceHours
			>= BestOccurrenceHours)
		{
			continue;
		}

		BestOccurrenceHours =
			OccurrenceHours;

		BestEvent =
			&Event;
	}

	if (!BestEvent)
	{
		return false;
	}

	OutEvent =
		*BestEvent;

	OutOccurrenceTime =
		FDiurnalDateTime::FromTotalHours(
			BestOccurrenceHours);

	return true;
}

bool UDiurnalCycleSubsystem::TryGetNextOccurrence(
	const FGameplayTag EventTag,
	FDiurnalDateTime& OutOccurrenceTime) const
{
	FDiurnalTimeEvent TimeEvent;

	if (!TryGetTimeEvent(
			EventTag,
			TimeEvent))
	{
		return false;
	}

	double OccurrenceHours = 0.0;

	if (!TryGetNextOccurrenceHours(
			TimeEvent,
			TotalGameHours,
			OccurrenceHours))
	{
		return false;
	}

	OutOccurrenceTime =
		FDiurnalDateTime::FromTotalHours(
			OccurrenceHours);

	return true;
}

bool UDiurnalCycleSubsystem::TryGetNextOccurrenceHours(
	const FDiurnalTimeEvent& TimeEvent,
	const double FromHours,
	double& OutOccurrenceHours) const
{
	checkf(
		TimeEvent.IsValid(),
		TEXT(
			"TryGetNextOccurrenceHours requires a valid event."));

	checkf(
		DiurnalCycle::IsValidTotalGameHours(
			FromHours),
		TEXT(
			"TryGetNextOccurrenceHours requires valid source hours."));

	const double TimeOfDayHours =
		TimeEvent.TimeOfDay.ToHours();

	double CandidateHours = 0.0;

	if (TimeEvent.bDatedEvent)
	{
		CandidateHours =
			static_cast<double>(
				TimeEvent.EventDay - 1)
				* DiurnalCycle::GHoursPerDay
			+ TimeOfDayHours;
	}
	else
	{
		const int64 CurrentDayIndex =
			static_cast<int64>(
				FMath::Floor(
					FromHours
						/ DiurnalCycle::GHoursPerDay));

		CandidateHours =
			static_cast<double>(
				CurrentDayIndex)
				* DiurnalCycle::GHoursPerDay
			+ TimeOfDayHours;

		if (CandidateHours
			<= FromHours)
		{
			CandidateHours +=
				DiurnalCycle::GHoursPerDay;
		}
	}

	if (CandidateHours
			<= FromHours
		|| !DiurnalCycle::IsValidTotalGameHours(
			CandidateHours))
	{
		return false;
	}

	OutOccurrenceHours =
		CandidateHours;

	return true;
}

#pragma endregion

#pragma region EventProcessing

void UDiurnalCycleSubsystem::SortTimeEvents()
{
	TimeEvents.StableSort(
		[](
			const FDiurnalTimeEvent& Left,
			const FDiurnalTimeEvent& Right)
		{
			return Left.TimeOfDay
				< Right.TimeOfDay;
		});
}

void UDiurnalCycleSubsystem::DispatchCrossedTimeEvents(
	const double PreviousHours,
	const double CurrentHours)
{
	checkf(
		CurrentHours >= PreviousHours,
		TEXT(
			"DispatchCrossedTimeEvents requires forward time."));

	if (TimeEvents.IsEmpty()
		|| CurrentHours == PreviousHours)
	{
		return;
	}

	struct FPendingDiurnalOccurrence
	{
		FDiurnalTimeEvent Event;
		FDiurnalDateTime OccurrenceTime;
	};

	TArray<FPendingDiurnalOccurrence>
		PendingOccurrences;

	const int64 FirstDayIndex =
		static_cast<int64>(
			FMath::Floor(
				PreviousHours
					/ DiurnalCycle::GHoursPerDay));

	const int64 LastDayIndex =
		static_cast<int64>(
			FMath::Floor(
				CurrentHours
					/ DiurnalCycle::GHoursPerDay));

	for (int64 DayIndex = FirstDayIndex;
		 DayIndex <= LastDayIndex;
		 ++DayIndex)
	{
		const int32 OccurrenceDay =
			static_cast<int32>(
				DayIndex + 1);

		const double DayStartHours =
			static_cast<double>(
				DayIndex)
				* DiurnalCycle::GHoursPerDay;

		for (const FDiurnalTimeEvent& Event :
			 TimeEvents)
		{
			if (!Event.OccursOnDay(
					OccurrenceDay))
			{
				continue;
			}

			const double OccurrenceHours =
				DayStartHours
				+ Event.TimeOfDay.ToHours();

			if (OccurrenceHours
					<= PreviousHours
				|| OccurrenceHours
					> CurrentHours)
			{
				continue;
			}

			PendingOccurrences.Add(
				{
					Event,
					FDiurnalDateTime(
						OccurrenceDay,
						Event.TimeOfDay)
				});
		}
	}

	bool bFinalGateStateApplied =
		false;

	const FDiurnalDateTime FinalDateTime =
		GetDateTime();

	/*
	 * The occurrence batch is complete before callbacks begin. Schedule
	 * mutations performed by listeners therefore affect future advancement
	 * without invalidating this dispatch.
	 */
	for (const FPendingDiurnalOccurrence& Pending :
		 PendingOccurrences)
	{
		if (!bFinalGateStateApplied
			&& Pending.OccurrenceTime
				== FinalDateTime)
		{
			const TArray<FGameplayTag>
				ScheduledGates =
					GetScheduledTimeGatesAt(
						FinalDateTime);

			if (!ScheduledGates.IsEmpty())
			{
				SetActiveTimeGates(
					ScheduledGates,
					FinalDateTime,
					true);
			}

			bFinalGateStateApplied =
				true;
		}

		TimeEventTriggeredEvent.Broadcast(
			Pending.Event,
			Pending.OccurrenceTime);
	}
}

#pragma endregion
