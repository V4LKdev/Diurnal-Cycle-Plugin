#pragma once

#include "AssetDefinitionDefault.h"
#include "AssetDefinition_DiurnalSchedule.generated.h"

UCLASS()
class UAssetDefinition_DiurnalSchedule final : public UAssetDefinitionDefault
{
	GENERATED_BODY()
protected:
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& AssetData, FName ClassName) const override;
	virtual const FSlateBrush* GetIconBrush(const FAssetData& AssetData, FName ClassName) const override;
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
