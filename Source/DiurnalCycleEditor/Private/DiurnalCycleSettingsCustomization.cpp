#include "DiurnalCycleSettingsCustomization.h"

#include "DiurnalCycleEditor.h"
#include "DiurnalCycleSettings.h"
#include "DiurnalSchedule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "DiurnalCycleSettingsCustomization"

TSharedRef<IDetailCustomization> FDiurnalCycleSettingsCustomization::MakeInstance() { return MakeShared<FDiurnalCycleSettingsCustomization>(); }

void FDiurnalCycleSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	PropertyUtilities = DetailBuilder.GetPropertyUtilities();
	const TSharedRef<IPropertyHandle> Defaults = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UDiurnalCycleSettings, DefaultSchedules));
	Defaults->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FDiurnalCycleSettingsCustomization::HandleDefaultSchedulesChanged));
	Defaults->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FDiurnalCycleSettingsCustomization::HandleDefaultSchedulesChanged));
	if (const TSharedPtr<IPropertyHandleArray> DefaultsArray = Defaults->AsArray())
	{
		DefaultsArray->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this, &FDiurnalCycleSettingsCustomization::HandleDefaultSchedulesChanged));
	}
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Schedule"), LOCTEXT("Schedule", "Schedule"), ECategoryPriority::Important);
	Category.AddProperty(Defaults);
	Category.AddCustomRow(LOCTEXT("DuplicateDefaults", "Duplicate Default Schedule")).WholeRowContent()
	[
		SNew(SBorder)
		.Visibility(this, &FDiurnalCycleSettingsCustomization::GetDuplicateWarningVisibility)
		.Padding(10)
		.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
		.BorderBackgroundColor(FStyleColors::Warning)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("DuplicateTitle", "Duplicate default schedule")).Font(FAppStyle::GetFontStyle("NormalFontBold"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 8)
			[
				SNew(STextBlock).Text(this, &FDiurnalCycleSettingsCustomization::GetDuplicateWarningText).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SNew(SButton).Text(LOCTEXT("RemoveDuplicates", "Remove Duplicate Entries")).OnClicked(this, &FDiurnalCycleSettingsCustomization::RemoveDuplicateSchedules)
			]
		]
	];
	Category.AddCustomRow(LOCTEXT("ScheduleActions", "Schedule Actions")).WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("OpenBrowser", "Open Schedule Browser")).OnClicked(this, &FDiurnalCycleSettingsCustomization::OpenBrowser)]
	];

	Category.AddCustomRow(LOCTEXT("ScheduleHelp", "Schedule ownership")).WholeRowContent()
	[SNew(STextBlock).AutoWrapText(true).Text(LOCTEXT("Help", "Schedule assets are authored defaults. Current PIE date/time, active runtime schedule layers, and runtime-added overrides are independent state."))];
}

FReply FDiurnalCycleSettingsCustomization::OpenBrowser() { FDiurnalCycleEditorModule::OpenScheduleBrowser(); return FReply::Handled(); }

void FDiurnalCycleSettingsCustomization::HandleDefaultSchedulesChanged()
{
	if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin()) Utilities->RequestRefresh();
}

EVisibility FDiurnalCycleSettingsCustomization::GetDuplicateWarningVisibility() const
{
	int32 FirstIndex = INDEX_NONE;
	int32 DuplicateIndex = INDEX_NONE;
	FSoftObjectPath Path;
	return DiurnalCycle::FindDuplicateScheduleReference(GetDefault<UDiurnalCycleSettings>()->DefaultSchedules, FirstIndex, DuplicateIndex, Path)
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FText FDiurnalCycleSettingsCustomization::GetDuplicateWarningText() const
{
	int32 FirstIndex = INDEX_NONE;
	int32 DuplicateIndex = INDEX_NONE;
	FSoftObjectPath Path;
	if (!DiurnalCycle::FindDuplicateScheduleReference(GetDefault<UDiurnalCycleSettings>()->DefaultSchedules, FirstIndex, DuplicateIndex, Path)) return FText::GetEmpty();
	return FText::Format(
		LOCTEXT("DuplicateMessage", "{0} is assigned at indices {1} and {2}. The runtime schedule set will not be applied; the previous valid active set is preserved."),
		FText::FromString(Path.GetAssetName()), FText::AsNumber(FirstIndex), FText::AsNumber(DuplicateIndex));
}

FReply FDiurnalCycleSettingsCustomization::RemoveDuplicateSchedules()
{
	UDiurnalCycleSettings* Settings = GetMutableDefault<UDiurnalCycleSettings>();
	const FScopedTransaction Transaction(LOCTEXT("RemoveDuplicateSchedulesTransaction", "Remove Duplicate Default Schedules"));
	Settings->Modify();
	TSet<FSoftObjectPath> Seen;
	Settings->DefaultSchedules.RemoveAll([&Seen](const TSoftObjectPtr<UDiurnalSchedule>& Schedule)
	{
		const FSoftObjectPath Path = Schedule.ToSoftObjectPath();
		if (Path.IsNull() || !Seen.Contains(Path))
		{
			if (!Path.IsNull()) Seen.Add(Path);
			return false;
		}
		return true;
	});
	Settings->SaveConfig();
	Settings->PostEditChange();
	if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin()) Utilities->RequestForceRefresh();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
