// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minesweeper/Includes/MineGridMapChanges.h"
#include "MineGridCellBase.h"

#include "MineGridBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterTriggeredCoordsDelegate, const FIntPoint&, EnteredIntoCoords);

/**
 * This native actor defines actual mine grid placed in world to be played on.
 * Manages mine grid cells actors. Used by MinesweeeperGameModeBase. Currently supporting only 
 * one instance of this actor at a time.
 */
UCLASS()
class MINESWEEPER_API AMineGridBase : public AActor
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FOnCharacterTriggeredCoordsDelegate OnCharacterTriggeredCoords;

	// Sets default values for this actor's properties
	AMineGridBase();

	UFUNCTION()
	void HandleCharacterCellTriggering(class AMineGridCellBase* EnteredCell, class ACharacter* EnteringCharacter);

	// Updates grid cells actors according by added and removed cells.
	UFUNCTION(BlueprintCallable)
	void AddOrRemoveGridCells(const FMineGridMapChanges& GridMapChanges);

	// Update grid cell values according by new map data
	UFUNCTION()
	void UpdateCellValues(const FMineGridMapCellUpdates& UpdatedMineGridMapCells);

	// Tell grid actor to set cell as exploded
	UFUNCTION()
	void SetGridCellExploded(const FIntPoint& ExplodedCoords);

	FORCEINLINE float GetCellSize() { return CellSize; }

protected:

	// Subclass of cell actor class to use for spawning
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadWrite, Category = "MineGrid")
	TSubclassOf<AMineGridCellBase> GridCellClass;

	// How big cell
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MineGrid")
	float CellSize;

	// Mapping between cells and it's coordinates
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGrid")
	TMap<FIntPoint, AMineGridCellBase*> GridCoordsCells;

	// Horizontal and vertical size of grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGrid")
	FIntPoint GridDimensions;

	// Number of holding references to cells to ensure visibility
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGrid")
	TMap<FIntPoint, uint8> GridCellRefCounts;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Performs spawning cell actor
	AMineGridCellBase* SpawnCellAt(const FIntPoint& CellCoords);
};
