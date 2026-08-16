#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DiurnalScheduleFactory.generated.h"

UCLASS(hidecategories = Object)
class DIURNALCYCLEEDITOR_API UDiurnalScheduleFactory final : public UFactory
{
	GENERATED_BODY()
public:
	UDiurnalScheduleFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FName GetNewAssetThumbnailOverride() const override;
	virtual FName GetNewAssetIconOverride() const override;
	virtual FString GetDefaultNewAssetName() const override;
};
