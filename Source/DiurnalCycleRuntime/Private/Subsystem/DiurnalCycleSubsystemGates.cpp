#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region TimeGateQueries

bool UDiurnalCycleSubsystem::IsTimeGateActive(
	const FGameplayTag GateTag) const
{
	return GateTag.IsValid()
		&& ActiveTimeGates.Contains(
			GateTag);
}

#pragma endregion

#pragma region TimeGateControl

bool UDiurnalCycleSubsystem::ReleaseTimeGate(
	const FGameplayTag GateTag)
{
	if (!GateTag.IsValid())
	{
		return false;
	}

	const int32 GateIndex =
		ActiveTimeGates.IndexOfByKey(
			GateTag);

	if (GateIndex == INDEX_NONE)
	{
		return false;
	}

	ActiveTimeGates.RemoveAt(
		GateIndex);

	const FDiurnalDateTime ReleaseTime =
		GetDateTime();

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Released time gate '%s' at %s | Remaining: %d."),
		*GateTag.ToString(),
		*ReleaseTime.ToString(),
		ActiveTimeGates.Num());

	TimeGateReleasedEvent.Broadcast(
		GateTag,
		ReleaseTime);

	return true;
}

int32 UDiurnalCycleSubsystem::ReleaseAllTimeGates()
{
	if (ActiveTimeGates.IsEmpty())
	{
		return 0;
	}

	const TArray<FGameplayTag> ReleasedGates =
		ActiveTimeGates;

	ActiveTimeGates.Reset();

	const FDiurnalDateTime ReleaseTime =
		GetDateTime();

	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT(
			"Released all %d time gates at %s."),
		ReleasedGates.Num(),
		*ReleaseTime.ToString());

	for (const FGameplayTag GateTag :
		 ReleasedGates)
	{
		TimeGateReleasedEvent.Broadcast(
			GateTag,
			ReleaseTime);
	}

	return ReleasedGates.Num();
}

#pragma endregion

#pragma region TimeGateProcessing

TArray<FGameplayTag>
UDiurnalCycleSubsystem::GetScheduledTimeGatesAt(
	const FDiurnalDateTime& DateTime) const
{
	checkf(
		DateTime.IsValid(),
		TEXT(
			"GetScheduledTimeGatesAt requires "
			"a valid date-time."));

	const FDiurnalTimeOfDay TimeOfDay =
		DateTime.GetTimeOfDay();

	TArray<FGameplayTag> Result;

	for (const FDiurnalTimeEvent& Event :
		 TimeEvents)
	{
		if (!Event.IsBlocking()
			|| !Event.OccursOnDay(
				DateTime.Day)
			|| Event.TimeOfDay
				!= TimeOfDay)
		{
			continue;
		}

		Result.Add(
			Event.EventTag);
	}

	return Result;
}

void UDiurnalCycleSubsystem::SetActiveTimeGates(
	const TArray<FGameplayTag>& NewActiveTimeGates,
	const FDiurnalDateTime& TransitionTime,
	const bool bBroadcastTransitions)
{
	checkf(
		TransitionTime.IsValid(),
		TEXT(
			"SetActiveTimeGates requires "
			"a valid transition time."));

	TArray<FGameplayTag> ValidatedNewGates;

	ValidatedNewGates.Reserve(
		NewActiveTimeGates.Num());

	for (const FGameplayTag GateTag :
		 NewActiveTimeGates)
	{
		if (!GateTag.IsValid()
			|| ValidatedNewGates.Contains(
				GateTag))
		{
			continue;
		}

		const FDiurnalTimeEvent* GateEvent =
			TimeEvents.FindByPredicate(
				[GateTag](
					const FDiurnalTimeEvent& Event)
				{
					return Event.EventTag
							== GateTag
						&& Event.IsBlocking();
				});

		if (!ensureMsgf(
				GateEvent,
				TEXT(
					"Attempted to activate invalid time gate '%s'."),
				*GateTag.ToString()))
		{
			continue;
		}

		ValidatedNewGates.Add(
			GateTag);
	}

	TArray<FGameplayTag> ReleasedGates;

	for (const FGameplayTag PreviousGate :
		 ActiveTimeGates)
	{
		if (!ValidatedNewGates.Contains(
				PreviousGate))
		{
			ReleasedGates.Add(
				PreviousGate);
		}
	}

	TArray<FDiurnalTimeEvent> ActivatedGates;

	for (const FGameplayTag NewGate :
		 ValidatedNewGates)
	{
		if (ActiveTimeGates.Contains(
				NewGate))
		{
			continue;
		}

		const FDiurnalTimeEvent* GateEvent =
			TimeEvents.FindByPredicate(
				[NewGate](
					const FDiurnalTimeEvent& Event)
				{
					return Event.EventTag
						== NewGate;
				});

		check(GateEvent);

		ActivatedGates.Add(
			*GateEvent);
	}

	/*
	 * Commit the complete new gate set before callbacks so every listener sees
	 * the final state, even when several gates transition together.
	 */
	ActiveTimeGates =
		MoveTemp(
			ValidatedNewGates);

	if (!bBroadcastTransitions)
	{
		return;
	}

	for (const FGameplayTag ReleasedGate :
		 ReleasedGates)
	{
		TimeGateReleasedEvent.Broadcast(
			ReleasedGate,
			TransitionTime);
	}

	for (const FDiurnalTimeEvent& ActivatedGate :
		 ActivatedGates)
	{
		UE_LOG(
			LogDiurnalCycle,
			Verbose,
			TEXT(
				"Activated time gate '%s' at %s | Active: %d."),
			*ActivatedGate.EventTag.ToString(),
			*TransitionTime.ToString(),
			ActiveTimeGates.Num());

		TimeGateActivatedEvent.Broadcast(
			ActivatedGate,
			TransitionTime);
	}
}

bool UDiurnalCycleSubsystem::TryFindFirstBlockingOccurrence(
	const double PreviousHours,
	const double RequestedHours,
	double& OutOccurrenceHours) const
{
	checkf(
		RequestedHours >= PreviousHours,
		TEXT(
			"TryFindFirstBlockingOccurrence requires forward time."));

	double BestOccurrenceHours =
		TNumericLimits<double>::Max();

	bool bFoundBlockingOccurrence =
		false;

	for (const FDiurnalTimeEvent& Event :
		 TimeEvents)
	{
		if (!Event.IsBlocking())
		{
			continue;
		}

		double OccurrenceHours = 0.0;

		if (!TryGetNextOccurrenceHours(
				Event,
				PreviousHours,
				OccurrenceHours))
		{
			continue;
		}

		if (OccurrenceHours
				> RequestedHours
			|| OccurrenceHours
				>= BestOccurrenceHours)
		{
			continue;
		}

		BestOccurrenceHours =
			OccurrenceHours;

		bFoundBlockingOccurrence =
			true;
	}

	if (!bFoundBlockingOccurrence)
	{
		return false;
	}

	OutOccurrenceHours =
		BestOccurrenceHours;

	return true;
}

#pragma endregion