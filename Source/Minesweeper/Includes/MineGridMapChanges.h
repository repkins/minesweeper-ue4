#pragma once

#include "CoreMinimal.h"
#include "MineGridMapCell.h"

#include "MineGridMapChanges.generated.h"

USTRUCT(BlueprintType)
struct FMineGridMapChanges
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FIntPoint> AddedGridMapCellCoords;
	UPROPERTY()
	TArray<EMineGridMapCell> AddedGridMapCellValues;

	UPROPERTY()
	TArray<FIntPoint> RemovedGridMapCells;

	UPROPERTY()
	FIntPoint NewGridDimensions;
};

USTRUCT(BlueprintType)
struct FMineGridMapCellUpdates
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FIntPoint> UpdatedGridMapCellCoords;
	UPROPERTY()
	TArray<EMineGridMapCell> UpdatedGridMapCellValues;
};
