#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "DiurnalCycleWorldSettings.h"

class AWorldSettings;
class FActiveTimerHandle;
class UDiurnalCycleSubsystem;
struct FDiurnalTimeChange;

/**
 * Compact Level Editor toolbar integration for Day Night Cycle.
 *
 * During PIE the widget displays the active game-instance clock and updates
 * through the subsystem's native time-change notification. A temporary
 * low-frequency timer is used only while waiting for the PIE subsystem to
 * become available.
 *
 * Outside PIE the current map's automatic-advancement policy can be authored
 * directly from the toolbar. The policy is stored as Diurnal Cycle asset user
 * data on the map's AWorldSettings.
 */
class SDiurnalCycleToolbarWidget final
	: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalCycleToolbarWidget)
	{
	}
	SLATE_END_ARGS()

	void Construct(
		const FArguments& InArgs);

	virtual ~SDiurnalCycleToolbarWidget() override;

private:
#pragma region PIELifecycle

	void HandleBeginPIE(
		bool bIsSimulating);

	void HandleEndPIE(
		bool bIsSimulating);

	bool TryBindToSubsystem();
	void UnbindFromSubsystem();

	void StartDiscoveryTimer();
	void StopDiscoveryTimer();

	EActiveTimerReturnType DiscoverSubsystem(
		double CurrentTime,
		float DeltaTime);

#pragma endregion

#pragma region ClockDisplay

	void HandleTimeChanged(
		const FDiurnalTimeChange& Change);

	void UpdateDisplayText();

	FText GetClockToolTipText() const;

	UDiurnalCycleSubsystem* FindSubsystem() const;

#pragma endregion

#pragma region WorldPolicyAuthoring

	TSharedRef<SWidget> BuildWorldPolicyMenu();

	FText GetWorldPolicyToolTipText() const;

	bool CanEditWorldPolicy() const;

	EDiurnalCycleWorldTimePolicy
	GetEditorWorldPolicy() const;

	bool IsEditorWorldPolicy(
		EDiurnalCycleWorldTimePolicy Policy) const;

	void SetEditorWorldPolicy(
		EDiurnalCycleWorldTimePolicy Policy);

	AWorldSettings* FindEditorWorldSettings() const;

#pragma endregion

#pragma region ProjectSettings

	FReply OpenSettings();

#pragma endregion

private:
#pragma region State

	FText DisplayText;

	TWeakObjectPtr<UDiurnalCycleSubsystem> BoundSubsystem;
	FDelegateHandle TimeChangedHandle;

	TSharedPtr<FActiveTimerHandle> DiscoveryTimerHandle;

	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;

#pragma endregion
};