#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DiurnalCycleTypes.h"

#include "DiurnalSchedule.generated.h"

enum class EDiurnalScheduleIssueSeverity : uint8
{
	Warning,
	Error
};

enum class EDiurnalScheduleIssueEntryType : uint8
{
	Asset,
	Event,
	Range
};

/** Runtime-owned structured validation result shared by asset validation and editor UI. */
struct DIURNALCYCLERUNTIME_API FDiurnalScheduleValidationIssue
{
	EDiurnalScheduleIssueSeverity Severity = EDiurnalScheduleIssueSeverity::Error;
	FText Message;
	EDiurnalScheduleIssueEntryType EntryType = EDiurnalScheduleIssueEntryType::Asset;
	FGuid EntryId;
};

/** Reusable authored event and time-range schedule. Contains no clock state. */
UCLASS(BlueprintType, DisplayName = "Day/Night Cycle Schedule")
class DIURNALCYCLERUNTIME_API UDiurnalSchedule final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule", meta = (TitleProperty = "EventName"))
	TArray<FDiurnalTimeEvent> TimeEvents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schedule", meta = (TitleProperty = "RangeName"))
	TArray<FDiurnalTimeRange> TimeRanges;

	/** Repairs missing/duplicated identities and missing presentation names. */
	bool RepairEntries();

	/** Produces the authoritative structured validation issue set. */
	void GetValidationIssues(TArray<FDiurnalScheduleValidationIssue>& OutIssues) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
