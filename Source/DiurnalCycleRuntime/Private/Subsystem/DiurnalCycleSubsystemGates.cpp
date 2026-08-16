#include "Subsystem/DiurnalCycleSubsystem.h"

#include "DiurnalCycleLog.h"

#pragma region TimeGateQueries

TArray<FGameplayTag> UDiurnalCycleSubsystem::GetActiveTimeGateTags() const
{
	TArray<FGameplayTag> Result;
	for (const FDiurnalEventOccurrenceHandle& Occurrence : ActiveTimeGateOccurrences)
	{
		FDiurnalResolvedTimeEvent Resolved;
		if (!TryGetTimeEvent(Occurrence.Entry, Resolved))
		{
			continue;
		}
		for (const FGameplayTag Tag : Resolved.Event.EventTags.GetGameplayTagArray())
		{
			Result.AddUnique(Tag);
		}
	}
	return Result;
}

bool UDiurnalCycleSubsystem::IsTimeGateActive(const FGameplayTag GateTag) const
{
	if (!GateTag.IsValid())
	{
		return false;
	}
	for (const FDiurnalEventOccurrenceHandle& Occurrence : ActiveTimeGateOccurrences)
	{
		FDiurnalResolvedTimeEvent Resolved;
		if (TryGetTimeEvent(Occurrence.Entry, Resolved)
			&& Resolved.Event.HasTagExact(GateTag))
		{
			return true;
		}
	}
	return false;
}

bool UDiurnalCycleSubsystem::IsTimeGateOccurrenceActive(
	const FDiurnalEventOccurrenceHandle& Occurrence) const
{
	return Occurrence.IsValid()
		&& ActiveTimeGateOccurrences.ContainsByPredicate(
			[&Occurrence](const FDiurnalEventOccurrenceHandle& Active)
			{
				return Active.OccurrenceId == Occurrence.OccurrenceId;
			});
}

#pragma endregion

#pragma region TimeGateControl

bool UDiurnalCycleSubsystem::ReleaseTimeGate(
	const FDiurnalEventOccurrenceHandle& Occurrence)
{
	const int32 GateIndex = ActiveTimeGateOccurrences.IndexOfByPredicate(
		[&Occurrence](const FDiurnalEventOccurrenceHandle& Active)
		{
			return Active.OccurrenceId == Occurrence.OccurrenceId;
		});
	if (GateIndex == INDEX_NONE)
	{
		return false;
	}

	const FDiurnalEventOccurrenceHandle Released = ActiveTimeGateOccurrences[GateIndex];
	ActiveTimeGateOccurrences.RemoveAt(GateIndex);
	const FDiurnalDateTime ReleaseTime = GetDateTime();
	UE_LOG(
		LogDiurnalCycle,
		Verbose,
		TEXT("Released time gate occurrence '%s' at %s | Remaining: %d."),
		*Released.OccurrenceId.ToString(EGuidFormats::DigitsWithHyphensInBraces),
		*ReleaseTime.ToString(),
		ActiveTimeGateOccurrences.Num());

	TimeGateOccurrenceReleasedEvent.Broadcast(Released, ReleaseTime);
	return true;
}

int32 UDiurnalCycleSubsystem::ReleaseTimeGatesByTag(const FGameplayTag GateTag)
{
	if (!GateTag.IsValid())
	{
		return 0;
	}
	TArray<FDiurnalEventOccurrenceHandle> Matches;
	for (const FDiurnalEventOccurrenceHandle& Occurrence : ActiveTimeGateOccurrences)
	{
		FDiurnalResolvedTimeEvent Resolved;
		if (TryGetTimeEvent(Occurrence.Entry, Resolved)
			&& Resolved.Event.HasTagExact(GateTag))
		{
			Matches.Add(Occurrence);
		}
	}
	int32 Count = 0;
	for (const FDiurnalEventOccurrenceHandle& Match : Matches)
	{
		Count += ReleaseTimeGate(Match) ? 1 : 0;
	}
	return Count;
}

int32 UDiurnalCycleSubsystem::ReleaseAllTimeGates()
{
	if (ActiveTimeGateOccurrences.IsEmpty())
	{
		return 0;
	}
	const TArray<FDiurnalEventOccurrenceHandle> Released = ActiveTimeGateOccurrences;
	ActiveTimeGateOccurrences.Reset();
	const FDiurnalDateTime ReleaseTime = GetDateTime();

	UE_LOG(LogDiurnalCycle, Verbose, TEXT("Released all %d time gates at %s."), Released.Num(), *ReleaseTime.ToString());
	for (const FDiurnalEventOccurrenceHandle& Occurrence : Released)
	{
		TimeGateOccurrenceReleasedEvent.Broadcast(Occurrence, ReleaseTime);
	}
	return Released.Num();
}

#pragma endregion

#pragma region TimeGateProcessing

TArray<FDiurnalEventOccurrenceHandle> UDiurnalCycleSubsystem::GetScheduledTimeGatesAt(
	const FDiurnalDateTime& DateTime) const
{
	checkf(DateTime.IsValid(), TEXT("GetScheduledTimeGatesAt requires a valid date-time."));
	TArray<FDiurnalEventOccurrenceHandle> Result;
	const FDiurnalTimeOfDay TimeOfDay = DateTime.GetTimeOfDay();
	for (const FDiurnalResolvedTimeEvent& Resolved : ResolvedTimeEvents)
	{
		const FDiurnalTimeEvent& Event = Resolved.Event;
		if (!Event.IsBlocking()
			|| !Event.OccursOnDay(DateTime.Day)
			|| Event.TimeOfDay != TimeOfDay)
		{
			continue;
		}
		FDiurnalEventOccurrenceHandle& Handle = Result.AddDefaulted_GetRef();
		Handle.Entry = Resolved.GetEntryReference();
		Handle.OccurrenceTime = DateTime;
		Handle.OccurrenceId = FGuid::NewGuid();
	}
	return Result;
}

void UDiurnalCycleSubsystem::SetActiveTimeGates(
	const TArray<FDiurnalEventOccurrenceHandle>& NewActiveTimeGates,
	const FDiurnalDateTime& TransitionTime,
	const bool bBroadcastTransitions)
{
	checkf(TransitionTime.IsValid(), TEXT("SetActiveTimeGates requires a valid transition time."));
	TArray<FDiurnalEventOccurrenceHandle> ValidatedNewGates;
	for (const FDiurnalEventOccurrenceHandle& Candidate : NewActiveTimeGates)
	{
		FDiurnalResolvedTimeEvent GateEvent;
		if (!Candidate.IsValid()
			|| Candidate.OccurrenceTime != TransitionTime
			|| !TryGetTimeEvent(Candidate.Entry, GateEvent)
			|| !GateEvent.Event.IsBlocking()
			|| !GateEvent.Event.OccursOnDay(TransitionTime.Day)
			|| GateEvent.Event.TimeOfDay != TransitionTime.GetTimeOfDay())
		{
			ensureMsgf(false, TEXT("Attempted to activate an invalid time-gate occurrence."));
			continue;
		}

		if (ValidatedNewGates.ContainsByPredicate(
			[&Candidate](const FDiurnalEventOccurrenceHandle& Existing)
			{
				return Existing.Entry == Candidate.Entry;
			}))
		{
			continue;
		}

		const FDiurnalEventOccurrenceHandle* ExistingOccurrence = ActiveTimeGateOccurrences.FindByPredicate(
			[&Candidate](const FDiurnalEventOccurrenceHandle& Existing)
			{
				return Existing.Entry == Candidate.Entry
					&& Existing.OccurrenceTime == Candidate.OccurrenceTime;
			});
		ValidatedNewGates.Add(ExistingOccurrence ? *ExistingOccurrence : Candidate);
	}

	TArray<FDiurnalEventOccurrenceHandle> ReleasedGates;
	for (const FDiurnalEventOccurrenceHandle& Previous : ActiveTimeGateOccurrences)
	{
		if (!ValidatedNewGates.ContainsByPredicate(
			[&Previous](const FDiurnalEventOccurrenceHandle& Current)
			{
				return Current.OccurrenceId == Previous.OccurrenceId;
			}))
		{
			ReleasedGates.Add(Previous);
		}
	}

	TArray<FDiurnalEventOccurrenceHandle> ActivatedGates;
	for (const FDiurnalEventOccurrenceHandle& Current : ValidatedNewGates)
	{
		if (!ActiveTimeGateOccurrences.ContainsByPredicate(
			[&Current](const FDiurnalEventOccurrenceHandle& Previous)
			{
				return Previous.OccurrenceId == Current.OccurrenceId;
			}))
		{
			ActivatedGates.Add(Current);
		}
	}

	ActiveTimeGateOccurrences = MoveTemp(ValidatedNewGates);
	if (!bBroadcastTransitions)
	{
		return;
	}

	for (const FDiurnalEventOccurrenceHandle& Released : ReleasedGates)
	{
		TimeGateOccurrenceReleasedEvent.Broadcast(Released, TransitionTime);
	}
	for (const FDiurnalEventOccurrenceHandle& Activated : ActivatedGates)
	{
		FDiurnalResolvedTimeEvent Event;
		check(TryGetTimeEvent(Activated.Entry, Event));
		UE_LOG(
			LogDiurnalCycle,
			Verbose,
			TEXT("Activated time gate '%s' (%s) at %s | Active: %d."),
			*Event.Event.GetDisplayName().ToString(),
			*Activated.OccurrenceId.ToString(EGuidFormats::DigitsWithHyphensInBraces),
			*TransitionTime.ToString(),
			ActiveTimeGateOccurrences.Num());
		TimeGateOccurrenceActivatedEvent.Broadcast(Activated, Event.Event);
		TimeGateActivatedEvent.Broadcast(Event.Event, TransitionTime);
	}
}

bool UDiurnalCycleSubsystem::TryFindFirstBlockingOccurrence(
	const double PreviousHours,
	const double RequestedHours,
	double& OutOccurrenceHours) const
{
	OutOccurrenceHours = 0.0;
	checkf(RequestedHours >= PreviousHours, TEXT("TryFindFirstBlockingOccurrence requires forward time."));
	double BestOccurrenceHours = TNumericLimits<double>::Max();
	bool bFoundBlockingOccurrence = false;
	for (const FDiurnalTimeEvent& Event : TimeEvents)
	{
		if (!Event.IsBlocking())
		{
			continue;
		}
		double OccurrenceHours = 0.0;
		if (TryGetNextOccurrenceHours(Event, PreviousHours, OccurrenceHours)
			&& OccurrenceHours <= RequestedHours
			&& OccurrenceHours < BestOccurrenceHours)
		{
			BestOccurrenceHours = OccurrenceHours;
			bFoundBlockingOccurrence = true;
		}
	}
	if (!bFoundBlockingOccurrence)
	{
		return false;
	}
	OutOccurrenceHours = BestOccurrenceHours;
	return true;
}

#pragma endregion
