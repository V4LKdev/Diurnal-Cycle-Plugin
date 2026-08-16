#include "Blueprint/DiurnalScheduleBlueprintLibrary.h"

#include "Subsystem/DiurnalCycleSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UDiurnalCycleSubsystem* UDiurnalScheduleBlueprintLibrary::Resolve(const UObject* WorldContextObject)
{
	if (!GEngine || !IsValid(WorldContextObject)) return nullptr;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return IsValid(World) && IsValid(World->GetGameInstance()) ? World->GetGameInstance()->GetSubsystem<UDiurnalCycleSubsystem>() : nullptr;
}

bool UDiurnalScheduleBlueprintLibrary::SetActiveDiurnalSchedules(const UObject* WorldContextObject, const TArray<TSoftObjectPtr<UDiurnalSchedule>>& Schedules)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem && Subsystem->TrySetActiveSchedules(Schedules);
}

bool UDiurnalScheduleBlueprintLibrary::ActivateDiurnalSchedule(const UObject* WorldContextObject, UDiurnalSchedule* Schedule)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem && Subsystem->TryActivateSchedule(Schedule);
}

bool UDiurnalScheduleBlueprintLibrary::DeactivateDiurnalSchedule(const UObject* WorldContextObject, UDiurnalSchedule* Schedule)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem && Subsystem->DeactivateSchedule(Schedule);
}

TArray<TSoftObjectPtr<UDiurnalSchedule>> UDiurnalScheduleBlueprintLibrary::GetActiveDiurnalSchedules(const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem ? TArray<TSoftObjectPtr<UDiurnalSchedule>>(Subsystem->GetActiveSchedules()) : TArray<TSoftObjectPtr<UDiurnalSchedule>>{};
}

TArray<FDiurnalResolvedTimeEvent> UDiurnalScheduleBlueprintLibrary::GetResolvedDiurnalEvents(const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem ? TArray<FDiurnalResolvedTimeEvent>(Subsystem->GetResolvedTimeEvents()) : TArray<FDiurnalResolvedTimeEvent>{};
}

TArray<FDiurnalResolvedTimeRange> UDiurnalScheduleBlueprintLibrary::GetResolvedDiurnalRanges(const UObject* WorldContextObject)
{
	const UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem ? TArray<FDiurnalResolvedTimeRange>(Subsystem->GetResolvedTimeRanges()) : TArray<FDiurnalResolvedTimeRange>{};
}

bool UDiurnalScheduleBlueprintLibrary::ReenableDiurnalEventByReference(const UObject* WorldContextObject, const FDiurnalScheduleEntryReference& Reference)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem && Subsystem->ReenableTimeEvent(Reference);
}

int32 UDiurnalScheduleBlueprintLibrary::ReenableDiurnalEventsMatchingTag(const UObject* WorldContextObject, const FGameplayTag EventTag)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem ? Subsystem->ReenableTimeEventsByTag(EventTag) : 0;
}

bool UDiurnalScheduleBlueprintLibrary::ReenableDiurnalRangeByReference(const UObject* WorldContextObject, const FDiurnalScheduleEntryReference& Reference)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem && Subsystem->ReenableTimeRange(Reference);
}

int32 UDiurnalScheduleBlueprintLibrary::ReenableDiurnalRangesMatchingTag(const UObject* WorldContextObject, const FGameplayTag RangeTag)
{
	UDiurnalCycleSubsystem* Subsystem = Resolve(WorldContextObject);
	return Subsystem ? Subsystem->ReenableTimeRangesByTag(RangeTag) : 0;
}

FDiurnalScheduleEntryReference UDiurnalScheduleBlueprintLibrary::GetResolvedDiurnalEventReference(const FDiurnalResolvedTimeEvent& Event)
{
	return Event.GetEntryReference();
}

FDiurnalScheduleEntryReference UDiurnalScheduleBlueprintLibrary::GetResolvedDiurnalRangeReference(const FDiurnalResolvedTimeRange& Range)
{
	return Range.GetEntryReference();
}
