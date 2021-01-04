#pragma once

#include "CoreMinimal.h"
#include "MineGridMapCell.h"

#include "MineGridMap.generated.h"

USTRUCT(BlueprintType)
struct FMineGridMap
{
	GENERATED_BODY()

	/** Represents mapping between coordinates and enumerable cell values */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FIntPoint, EMineGridMapCell> Cells;

	/** Represents size and "shape" of grid map */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint GridDimensions = FIntPoint::ZeroValue;

	/** Represents first cell in first row */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint StartCoords = FIntPoint(0, 0);

  /** Represents last cell in last row */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint EndCoords = FIntPoint(-1, -1);
};
