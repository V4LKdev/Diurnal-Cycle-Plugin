#include "SDiurnalCycleToolbarWidget.h"

#include "DiurnalCycleSettings.h"
#include "Subsystem/DiurnalCycleWorldSubsystem.h"
#include "Subsystem/DiurnalCycleSubsystem.h"

#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/WorldSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DiurnalCycleToolbar
{
	/**
	 * Used only while waiting for the PIE game instance and subsystem to exist.
	 *
	 * Once the subsystem is found, live clock updates are fully event-driven.
	 */
	constexpr float DiscoveryIntervalSeconds =
		0.25f;

	FSlateFontInfo GetClockFont()
	{
		FSlateFontInfo Font =
			FAppStyle::Get().GetFontStyle(
				TEXT("NormalFontBold"));

		Font.Size =
			12;

		return Font;
	}


	const TCHAR* GetPolicyDisplayText(
		const EDiurnalCycleWorldTimePolicy Policy)
	{
		switch (Policy)
		{
		case EDiurnalCycleWorldTimePolicy::UseProjectDefault:
			return TEXT("Use Project Default");

		case EDiurnalCycleWorldTimePolicy::Advance:
			return TEXT("Advance");

		case EDiurnalCycleWorldTimePolicy::Freeze:
			return TEXT("Freeze");

		default:
			return TEXT("Unknown");
		}
	}
}

#pragma region ConstructionAndPIELifecycle

void SDiurnalCycleToolbarWidget::Construct(
	const FArguments& /* InArgs */)
{
	DisplayText =
		FText::GetEmpty();

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility_Lambda(
				[this]()
				{
					return BoundSubsystem.IsValid()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(
				FMargin(
					6.0f,
					1.0f,
					4.0f,
					0.0f))
			[
				SNew(STextBlock)
				.Text_Lambda(
					[this]()
					{
						return DisplayText;
					})
				.ToolTipText_Lambda(
					[this]()
					{
						return GetClockToolTipText();
					})
				.Font(
					DiurnalCycleToolbar::
						GetClockFont())
				.Justification(
					ETextJustify::Center)
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(
			FMargin(
				0.0f,
				0.0f,
				2.0f,
				0.0f))
		[
			SNew(SComboButton)
				.ButtonStyle(
					&FAppStyle::Get()
						.GetWidgetStyle<FButtonStyle>(
							TEXT("SimpleButton")))
				.HasDownArrow(false)
				.ContentPadding(
					FMargin(3.0f))
				.ToolTipText_Lambda(
					[this]()
					{
						return GetWorldPolicyToolTipText();
					})
				.IsEnabled_Lambda(
					[this]()
					{
						return CanEditWorldPolicy();
					})
				.OnGetMenuContent(
					this,
					&SDiurnalCycleToolbarWidget::
						BuildWorldPolicyMenu)
				.ButtonContent()
				[
					SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
					[
						SNew(SImage)
							.Image(
								FSlateIconFinder::
									FindIconBrushForClass(
										UWorld::StaticClass(),
										FName(
											TEXT("ClassIcon.World"))))
							.ColorAndOpacity(
								FSlateColor::UseForeground())
					]
				]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(
			FMargin(
				0.0f,
				0.0f,
				2.0f,
				0.0f))
		[
			SNew(SButton)
				.ButtonStyle(
					&FAppStyle::Get()
						.GetWidgetStyle<FButtonStyle>(
							TEXT("SimpleButton")))
				.ContentPadding(
					FMargin(3.0f))
				.ToolTipText(
					NSLOCTEXT(
						"DiurnalCycleToolbar",
						"OpenSettingsTooltip",
						"Open Day Night Cycle project settings"))
				.OnClicked(
					this,
					&SDiurnalCycleToolbarWidget::
						OpenSettings)
			[
				SNew(SBox)
					.WidthOverride(
						16.0f)
					.HeightOverride(
						16.0f)
				[
					SNew(SImage)
						.Image(
							FAppStyle::Get().GetBrush(
								TEXT("Icons.Settings")))
						.ColorAndOpacity(
							FSlateColor::UseForeground())
				]
			]
		]
	];

	BeginPIEHandle =
		FEditorDelegates::BeginPIE.AddSP(
			this,
			&SDiurnalCycleToolbarWidget::
				HandleBeginPIE);

	EndPIEHandle =
		FEditorDelegates::EndPIE.AddSP(
			this,
			&SDiurnalCycleToolbarWidget::
				HandleEndPIE);

	/*
	 * Supports toolbar reconstruction while PIE is already active, including
	 * after menu refreshes and editor module reloads.
	 */
	if (GEditor
		&& GEditor->PlayWorld)
	{
		if (!TryBindToSubsystem())
		{
			StartDiscoveryTimer();
		}
	}
}

SDiurnalCycleToolbarWidget::
~SDiurnalCycleToolbarWidget()
{
	StopDiscoveryTimer();
	UnbindFromSubsystem();

	FEditorDelegates::BeginPIE.Remove(
		BeginPIEHandle);

	FEditorDelegates::EndPIE.Remove(
		EndPIEHandle);
}

void SDiurnalCycleToolbarWidget::HandleBeginPIE(
	const bool /* bIsSimulating */)
{
	UnbindFromSubsystem();

	DisplayText =
		FText::GetEmpty();

	if (!TryBindToSubsystem())
	{
		StartDiscoveryTimer();
	}
}

void SDiurnalCycleToolbarWidget::HandleEndPIE(
	const bool /* bIsSimulating */)
{
	StopDiscoveryTimer();
	UnbindFromSubsystem();

	DisplayText =
		FText::GetEmpty();
}

bool SDiurnalCycleToolbarWidget::TryBindToSubsystem()
{
	if (BoundSubsystem.IsValid())
	{
		return true;
	}

	UDiurnalCycleSubsystem* Subsystem =
		FindSubsystem();

	if (!Subsystem)
	{
		return false;
	}

	BoundSubsystem =
		Subsystem;

	TimeChangedHandle =
		Subsystem->OnTimeChanged().AddSP(
			SharedThis(this),
			&SDiurnalCycleToolbarWidget::
				HandleTimeChanged);

	UpdateDisplayText();
	return true;
}

void SDiurnalCycleToolbarWidget::
UnbindFromSubsystem()
{
	if (UDiurnalCycleSubsystem* Subsystem =
		BoundSubsystem.Get())
	{
		if (TimeChangedHandle.IsValid())
		{
			Subsystem->OnTimeChanged().Remove(
				TimeChangedHandle);
		}
	}

	TimeChangedHandle.Reset();
	BoundSubsystem.Reset();
}

void SDiurnalCycleToolbarWidget::
StartDiscoveryTimer()
{
	if (DiscoveryTimerHandle.IsValid())
	{
		return;
	}

	DiscoveryTimerHandle =
		RegisterActiveTimer(
			DiurnalCycleToolbar::
				DiscoveryIntervalSeconds,
			FWidgetActiveTimerDelegate::CreateSP(
				this,
				&SDiurnalCycleToolbarWidget::
					DiscoverSubsystem));
}

void SDiurnalCycleToolbarWidget::
StopDiscoveryTimer()
{
	if (!DiscoveryTimerHandle.IsValid())
	{
		return;
	}

	UnRegisterActiveTimer(
		DiscoveryTimerHandle.ToSharedRef());

	DiscoveryTimerHandle.Reset();
}

EActiveTimerReturnType
SDiurnalCycleToolbarWidget::DiscoverSubsystem(
	const double /* CurrentTime */,
	const float /* DeltaTime */)
{
	if (!TryBindToSubsystem())
	{
		return EActiveTimerReturnType::Continue;
	}

	DiscoveryTimerHandle.Reset();

	return EActiveTimerReturnType::Stop;
}

#pragma endregion

#pragma region ClockDisplay

void SDiurnalCycleToolbarWidget::
HandleTimeChanged(
	const FDiurnalTimeChange& Change)
{
	DisplayText =
		FText::FromString(
			Change.CurrentDateTime.ToString());
}

void SDiurnalCycleToolbarWidget::
UpdateDisplayText()
{
	const UDiurnalCycleSubsystem* Subsystem =
		BoundSubsystem.Get();

	DisplayText =
		Subsystem
			? FText::FromString(
				Subsystem->GetDateTime().ToString())
			: FText::GetEmpty();
}

FText SDiurnalCycleToolbarWidget::
GetClockToolTipText() const
{
	const UDiurnalCycleSubsystem* Subsystem =
		BoundSubsystem.Get();

	if (!Subsystem)
	{
		return NSLOCTEXT(
			"DiurnalCycleToolbar",
			"ClockUnavailableTooltip",
			"Start Play In Editor to display the active Day Night Cycle clock.");
	}

	const UWorld* PlayWorld =
		GEditor
			? GEditor->PlayWorld
			: nullptr;

	const UDiurnalCycleWorldSubsystem* WorldSubsystem =
		IsValid(PlayWorld)
			? PlayWorld->GetSubsystem<
				UDiurnalCycleWorldSubsystem>()
			: nullptr;

	FString StateText;

	if (Subsystem->IsPaused())
	{
		StateText =
			TEXT("Paused");
	}
	else if (Subsystem->IsBlockedByTimeGate())
	{
		StateText =
			TEXT("Blocked by Time Gate");
	}
	else if (Subsystem->GetTimeScale() <= 0.0)
	{
		StateText =
			TEXT("Stopped by Zero Time Scale");
	}
	else if (WorldSubsystem
		&& !WorldSubsystem->ShouldAdvanceTime())
	{
		StateText =
			TEXT("Frozen by World Policy");
	}
	else
	{
		StateText =
			TEXT("Running");
	}

	return FText::FromString(
		FString::Printf(
			TEXT("%s\nAutomatic Advancement: %s"),
			*Subsystem->GetDateTime().ToString(),
			*StateText));
}

UDiurnalCycleSubsystem*
SDiurnalCycleToolbarWidget::FindSubsystem() const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* PlayWorld =
		GEditor->PlayWorld;

	if (!IsValid(PlayWorld))
	{
		return nullptr;
	}

	UGameInstance* GameInstance =
		PlayWorld->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<
		UDiurnalCycleSubsystem>();
}

#pragma endregion

#pragma region WorldPolicyAuthoring

TSharedRef<SWidget>
SDiurnalCycleToolbarWidget::
BuildWorldPolicyMenu()
{
	FMenuBuilder MenuBuilder(
		true,
		nullptr);

	const auto AddPolicyEntry =
		[this, &MenuBuilder](
			const EDiurnalCycleWorldTimePolicy Policy,
			const FText& Label,
			const FText& ToolTip)
		{
			const FUIAction Action(
				FExecuteAction::CreateSP(
					this,
					&SDiurnalCycleToolbarWidget::
						SetEditorWorldPolicy,
					Policy),
				FCanExecuteAction::CreateSP(
					this,
					&SDiurnalCycleToolbarWidget::
						CanEditWorldPolicy),
				FIsActionChecked::CreateSP(
					this,
					&SDiurnalCycleToolbarWidget::
						IsEditorWorldPolicy,
					Policy),
				EUIActionRepeatMode::RepeatDisabled);

			MenuBuilder.AddMenuEntry(
				Label,
				ToolTip,
				FSlateIcon(),
				Action,
				NAME_None,
				EUserInterfaceActionType::RadioButton);
		};

	AddPolicyEntry(
		EDiurnalCycleWorldTimePolicy::UseProjectDefault,
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyDefault",
			"Use Project Default"),
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyDefaultTooltip",
			"Use the project-wide Day Night Cycle world advancement default."));

	AddPolicyEntry(
		EDiurnalCycleWorldTimePolicy::Advance,
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyAdvance",
			"Advance"),
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyAdvanceTooltip",
			"Allow automatic Day Night Cycle advancement in this map."));

	AddPolicyEntry(
		EDiurnalCycleWorldTimePolicy::Freeze,
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyFreeze",
			"Freeze"),
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyFreezeTooltip",
			"Prevent automatic Day Night Cycle advancement in this map."));

	return MenuBuilder.MakeWidget();
}


FText SDiurnalCycleToolbarWidget::
GetWorldPolicyToolTipText() const
{
	AWorldSettings* WorldSettings =
		FindEditorWorldSettings();

	if (!WorldSettings)
	{
		return NSLOCTEXT(
			"DiurnalCycleToolbar",
			"WorldPolicyUnavailableTooltip",
			"No editable Level Editor world is currently available.");
	}

	const EDiurnalCycleWorldTimePolicy Policy =
		GetEditorWorldPolicy();

	FString EffectiveSuffix;

	if (Policy
		== EDiurnalCycleWorldTimePolicy::UseProjectDefault)
	{
		const UDiurnalCycleSettings* Settings =
			GetDefault<
				UDiurnalCycleSettings>();

		EffectiveSuffix =
			FString::Printf(
				TEXT(" (currently %s)"),
				Settings
					&& Settings->bAdvanceTimeByDefault
						? TEXT("Advance")
						: TEXT("Freeze"));
	}

	const FString PIEHint =
		GEditor
			&& GEditor->PlayWorld
				? TEXT(
					"\nStop PIE to edit the map policy. "
					"Use the Gameplay Debugger for transient runtime overrides.")
				: TEXT("");

	return FText::FromString(
		FString::Printf(
			TEXT(
				"Current map policy: %s%s."
				"\nStored with this map's World Settings.%s"),
			DiurnalCycleToolbar::
				GetPolicyDisplayText(
					Policy),
			*EffectiveSuffix,
			*PIEHint));
}

bool SDiurnalCycleToolbarWidget::
CanEditWorldPolicy() const
{
	return GEditor
		&& !GEditor->PlayWorld
		&& IsValid(
			FindEditorWorldSettings());
}

EDiurnalCycleWorldTimePolicy
SDiurnalCycleToolbarWidget::
GetEditorWorldPolicy() const
{
	AWorldSettings* WorldSettings =
		FindEditorWorldSettings();

	if (!WorldSettings)
	{
		return EDiurnalCycleWorldTimePolicy::
			UseProjectDefault;
	}

	const UDiurnalCycleWorldSettings* WorldPolicy =
		Cast<UDiurnalCycleWorldSettings>(
			WorldSettings->GetAssetUserDataOfClass(
				UDiurnalCycleWorldSettings::
					StaticClass()));

	return WorldPolicy
		? WorldPolicy->TimeAdvancementPolicy
		: EDiurnalCycleWorldTimePolicy::
			UseProjectDefault;
}

bool SDiurnalCycleToolbarWidget::
IsEditorWorldPolicy(
	const EDiurnalCycleWorldTimePolicy Policy) const
{
	return GetEditorWorldPolicy()
		== Policy;
}

void SDiurnalCycleToolbarWidget::
SetEditorWorldPolicy(
	const EDiurnalCycleWorldTimePolicy Policy)
{
	AWorldSettings* WorldSettings =
		FindEditorWorldSettings();

	if (!IsValid(WorldSettings)
		|| !CanEditWorldPolicy())
	{
		return;
	}

	UDiurnalCycleWorldSettings* ExistingPolicy =
		Cast<UDiurnalCycleWorldSettings>(
			WorldSettings->GetAssetUserDataOfClass(
				UDiurnalCycleWorldSettings::
					StaticClass()));

	const EDiurnalCycleWorldTimePolicy CurrentPolicy =
		ExistingPolicy
			? ExistingPolicy->TimeAdvancementPolicy
			: EDiurnalCycleWorldTimePolicy::
				UseProjectDefault;

	if (CurrentPolicy == Policy)
	{
		return;
	}

	const FScopedTransaction Transaction(
		NSLOCTEXT(
			"DiurnalCycleToolbar",
			"SetWorldPolicyTransaction",
			"Set Day Night Cycle World Policy"));

	WorldSettings->Modify();

	if (Policy
		== EDiurnalCycleWorldTimePolicy::UseProjectDefault)
	{
		if (ExistingPolicy)
		{
			ExistingPolicy->Modify();

			WorldSettings->RemoveUserDataOfClass(
				UDiurnalCycleWorldSettings::
					StaticClass());
		}
	}
	else
	{
		UDiurnalCycleWorldSettings* WorldPolicy =
			ExistingPolicy;

		if (!WorldPolicy)
		{
			WorldPolicy =
				NewObject<
					UDiurnalCycleWorldSettings>(
						WorldSettings,
						NAME_None,
						RF_Transactional);

			WorldSettings->AddAssetUserData(
				WorldPolicy);
		}

		WorldPolicy->Modify();

		WorldPolicy->TimeAdvancementPolicy =
			Policy;
	}

	WorldSettings->MarkPackageDirty();
}

AWorldSettings*
SDiurnalCycleToolbarWidget::
FindEditorWorldSettings() const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* EditorWorld =
		GEditor->GetEditorWorldContext().World();

	return IsValid(EditorWorld)
		? EditorWorld->GetWorldSettings()
		: nullptr;
}

#pragma endregion

#pragma region ProjectSettings

FReply SDiurnalCycleToolbarWidget::OpenSettings()
{
	const UDiurnalCycleSettings* Settings =
		GetDefault<
			UDiurnalCycleSettings>();

	check(Settings);

	ISettingsModule& SettingsModule =
		FModuleManager::LoadModuleChecked<
			ISettingsModule>(
				TEXT("Settings"));

	SettingsModule.ShowViewer(
		Settings->GetContainerName(),
		Settings->GetCategoryName(),
		Settings->GetSectionName());

	return FReply::Handled();
}

#pragma endregion
