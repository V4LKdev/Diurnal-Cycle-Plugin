#include "AssetDefinition_DiurnalSchedule.h"

#include "DiurnalCycleEditorStyle.h"
#include "DiurnalSchedule.h"
#include "ScheduleEditor/DiurnalScheduleEditorToolkit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_DiurnalSchedule)

#define LOCTEXT_NAMESPACE "AssetDefinition_DiurnalSchedule"

FText UAssetDefinition_DiurnalSchedule::GetAssetDisplayName() const { return LOCTEXT("Name", "Day/Night Cycle Schedule"); }
FLinearColor UAssetDefinition_DiurnalSchedule::GetAssetColor() const { return FColor(85, 199, 232); }
TSoftClassPtr<UObject> UAssetDefinition_DiurnalSchedule::GetAssetClass() const { return UDiurnalSchedule::StaticClass(); }
TConstArrayView<FAssetCategoryPath> UAssetDefinition_DiurnalSchedule::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { FAssetCategoryPath(LOCTEXT("Category", "Day/Night Cycle")) };
	return Categories;
}
const FSlateBrush* UAssetDefinition_DiurnalSchedule::GetThumbnailBrush(const FAssetData&, FName) const { return FDiurnalCycleEditorStyle::Get().GetBrush(TEXT("DiurnalCycle.ScheduleThumbnail")); }
const FSlateBrush* UAssetDefinition_DiurnalSchedule::GetIconBrush(const FAssetData&, FName) const { return FDiurnalCycleEditorStyle::Get().GetBrush(TEXT("DiurnalCycle.ScheduleIcon")); }
EAssetCommandResult UAssetDefinition_DiurnalSchedule::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UDiurnalSchedule* Schedule : OpenArgs.LoadObjects<UDiurnalSchedule>())
	{
		if (IsValid(Schedule))
		{
			TSharedRef<FDiurnalScheduleEditorToolkit> Editor = MakeShared<FDiurnalScheduleEditorToolkit>();
			Editor->InitScheduleEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Schedule);
		}
	}
	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
