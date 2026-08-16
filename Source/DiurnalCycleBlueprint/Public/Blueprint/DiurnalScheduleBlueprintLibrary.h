#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DiurnalCycleTypes.h"
#include "DiurnalScheduleBlueprintLibrary.generated.h"

class UDiurnalSchedule;

UCLASS()
class DIURNALCYCLEBLUEPRINT_API UDiurnalScheduleBlueprintLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Active Day Night Cycle Schedules"))
	static bool SetActiveDiurnalSchedules(const UObject* WorldContextObject, const TArray<TSoftObjectPtr<UDiurnalSchedule>>& Schedules);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Activate Day Night Cycle Schedule"))
	static bool ActivateDiurnalSchedule(const UObject* WorldContextObject, UDiurnalSchedule* Schedule);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Deactivate Day Night Cycle Schedule"))
	static bool DeactivateDiurnalSchedule(const UObject* WorldContextObject, UDiurnalSchedule* Schedule);

	UFUNCTION(BlueprintPure, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Active Day Night Cycle Schedules"))
	static TArray<TSoftObjectPtr<UDiurnalSchedule>> GetActiveDiurnalSchedules(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Resolved Day Night Cycle Events", ToolTip = "Returns all active event contributions with their exact source schedule and EntryId."))
	static TArray<FDiurnalResolvedTimeEvent> GetResolvedDiurnalEvents(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Day Night Cycle|Schedules", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Resolved Day Night Cycle Time Ranges", ToolTip = "Returns all active range contributions with their exact source schedule and EntryId."))
	static TArray<FDiurnalResolvedTimeRange> GetResolvedDiurnalRanges(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Events", meta = (WorldContext = "WorldContextObject", DisplayName = "Re-enable Day Night Cycle Event by Reference"))
	static bool ReenableDiurnalEventByReference(const UObject* WorldContextObject, const FDiurnalScheduleEntryReference& Reference);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Events", meta = (WorldContext = "WorldContextObject", DisplayName = "Re-enable Day Night Cycle Events Matching Tag", ToolTip = "Atomically re-enables every disabled authored event containing this semantic tag and returns the committed count."))
	static int32 ReenableDiurnalEventsMatchingTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "DiurnalCycle.TimeEvent")) FGameplayTag EventTag);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Time Ranges", meta = (WorldContext = "WorldContextObject", DisplayName = "Re-enable Day Night Cycle Time Range by Reference"))
	static bool ReenableDiurnalRangeByReference(const UObject* WorldContextObject, const FDiurnalScheduleEntryReference& Reference);

	UFUNCTION(BlueprintCallable, Category = "Day Night Cycle|Time Ranges", meta = (WorldContext = "WorldContextObject", DisplayName = "Re-enable Day Night Cycle Time Ranges Matching Tag", ToolTip = "Atomically re-enables every disabled authored range containing this semantic tag and returns the committed count."))
	static int32 ReenableDiurnalRangesMatchingTag(const UObject* WorldContextObject, UPARAM(meta = (Categories = "DiurnalCycle.TimeRange")) FGameplayTag RangeTag);

	UFUNCTION(BlueprintPure, Category = "Day Night Cycle|Schedules", meta = (DisplayName = "Get Resolved Day Night Cycle Event Reference"))
	static FDiurnalScheduleEntryReference GetResolvedDiurnalEventReference(const FDiurnalResolvedTimeEvent& Event);

	UFUNCTION(BlueprintPure, Category = "Day Night Cycle|Schedules", meta = (DisplayName = "Get Resolved Day Night Cycle Time Range Reference"))
	static FDiurnalScheduleEntryReference GetResolvedDiurnalRangeReference(const FDiurnalResolvedTimeRange& Range);

private:
	static class UDiurnalCycleSubsystem* Resolve(const UObject* WorldContextObject);
};
