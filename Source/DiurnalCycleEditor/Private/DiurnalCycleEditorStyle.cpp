#include "DiurnalCycleEditorStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FDiurnalCycleEditorStyle::StyleInstance;

void FDiurnalCycleEditorStyle::Initialize()
{
	if (StyleInstance.IsValid()) return;
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DiurnalCycle"));
	check(Plugin.IsValid());
	StyleInstance = MakeShared<FSlateStyleSet>(GetStyleSetName());
	StyleInstance->SetContentRoot(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));
	const FString SchedulePath = StyleInstance->RootToContentDir(TEXT("DiurnalSchedule"), TEXT(".svg"));
	const FString ScheduleToolbarPath = StyleInstance->RootToContentDir(TEXT("DiurnalScheduleToolbar"), TEXT(".svg"));
	const FString AdvancePath = StyleInstance->RootToContentDir(TEXT("DiurnalAdvance"), TEXT(".svg"));
	const FString FreezePath = StyleInstance->RootToContentDir(TEXT("DiurnalFreeze"), TEXT(".svg"));
	const FString DefaultPath = StyleInstance->RootToContentDir(TEXT("DiurnalDefault"), TEXT(".svg"));
	const FString RepeatingPath = StyleInstance->RootToContentDir(TEXT("DiurnalEventRepeating"), TEXT(".svg"));
	const FString OncePath = StyleInstance->RootToContentDir(TEXT("DiurnalEventOnce"), TEXT(".svg"));
	const FString RangePath = StyleInstance->RootToContentDir(TEXT("DiurnalRange"), TEXT(".svg"));
	const FString NotifyPath = StyleInstance->RootToContentDir(TEXT("DiurnalNotify"), TEXT(".svg"));
	const FString BlockingPath = StyleInstance->RootToContentDir(TEXT("DiurnalBlocking"), TEXT(".svg"));
	const FString ListPath = StyleInstance->RootToContentDir(TEXT("DiurnalList"), TEXT(".svg"));
	const FString TimelinePath = StyleInstance->RootToContentDir(TEXT("DiurnalTimeline"), TEXT(".svg"));
	StyleInstance->Set(TEXT("DiurnalCycle.ScheduleIcon"), new FSlateVectorImageBrush(SchedulePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.ScheduleThumbnail"), new FSlateVectorImageBrush(SchedulePath, FVector2D(64, 64)));
	StyleInstance->Set(TEXT("DiurnalCycle.Toolbar.Schedule"), new FSlateVectorImageBrush(ScheduleToolbarPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Policy.Advance"), new FSlateVectorImageBrush(AdvancePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Policy.Freeze"), new FSlateVectorImageBrush(FreezePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Policy.Default"), new FSlateVectorImageBrush(DefaultPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Entry.Repeating"), new FSlateVectorImageBrush(RepeatingPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Entry.Once"), new FSlateVectorImageBrush(OncePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Entry.Range"), new FSlateVectorImageBrush(RangePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Entry.Notify"), new FSlateVectorImageBrush(NotifyPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.Entry.Blocking"), new FSlateVectorImageBrush(BlockingPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.View.List"), new FSlateVectorImageBrush(ListPath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("DiurnalCycle.View.Timeline"), new FSlateVectorImageBrush(TimelinePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("ClassIcon.DiurnalSchedule"), new FSlateVectorImageBrush(SchedulePath, FVector2D(16, 16)));
	StyleInstance->Set(TEXT("ClassThumbnail.DiurnalSchedule"), new FSlateVectorImageBrush(SchedulePath, FVector2D(64, 64)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FDiurnalCycleEditorStyle::Shutdown()
{
	if (!StyleInstance.IsValid()) return;
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	StyleInstance.Reset();
}

FName FDiurnalCycleEditorStyle::GetStyleSetName()
{
	static const FName Name(TEXT("DiurnalCycleEditorStyle"));
	return Name;
}

const ISlateStyle& FDiurnalCycleEditorStyle::Get()
{
	check(StyleInstance.IsValid());
	return *StyleInstance;
}
