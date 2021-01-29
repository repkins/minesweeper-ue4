// Fill out your copyright notice in the Description page of Project Settings.


#include "MineGridBase.h"
#include "MineGridCellBase.h"

// Sets default values
AMineGridBase::AMineGridBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Setup scene root component
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());
	SceneRoot->Mobility = EComponentMobility::Static;
	
#if WITH_EDITORONLY_DATA
	SceneRoot->bVisualizeComponent = true;
#endif

	SetRootComponent(SceneRoot);
	AddInstanceComponent(SceneRoot);

	// Setting actor defaults
	GridCellClass = AMineGridCellBase::StaticClass();
	CellSize = 200.f;
}

void AMineGridBase::HandleCharacterCellTriggering(AMineGridCellBase* EnteredCell, ACharacter* EnteringCharacter)
{
	// Determine coords of cell actor in grid
	const FIntPoint* FoundEnteredCoordsPtr = GridCoordsCells.FindKey(EnteredCell);
	if (FoundEnteredCoordsPtr)
	{
		const FIntPoint EnteredCoords = *FoundEnteredCoordsPtr;

		// Broadcast it
		OnCharacterTriggeredCoords.Broadcast(EnteredCoords);
	}
}

void AMineGridBase::AddOrRemoveGridCells(const FMineGridMapChanges& GridMapChanges)
{
	GridCoordsCells.Reserve(GridMapChanges.NewGridDimensions.X * GridMapChanges.NewGridDimensions.Y);

	// Remove and destroy cell actors
	for (const FIntPoint& CoordsToDestroy : GridMapChanges.RemovedGridMapCells)
	{
		if (AMineGridCellBase* CellActor = GridCoordsCells.FindAndRemoveChecked(CoordsToDestroy))
		{
			CellActor->Destroy();
		}
	}

	// Spawn and add cell actors
	auto AddedCoordsIt = GridMapChanges.AddedGridMapCellCoords.CreateConstIterator();
	auto AddedValuesIt = GridMapChanges.AddedGridMapCellValues.CreateConstIterator();

	for (AddedCoordsIt, AddedValuesIt; AddedCoordsIt && AddedValuesIt; ++AddedCoordsIt, ++AddedValuesIt)
	{
		if (AMineGridCellBase* NewCellActor = SpawnCellAt(*AddedCoordsIt))
		{
			NewCellActor->UpdateCellValue(*AddedValuesIt);
			GridCoordsCells.Emplace(*AddedCoordsIt, NewCellActor);
		}
	}

	// Update grid-dimensions
	GridDimensions = GridMapChanges.NewGridDimensions;
}

void AMineGridBase::UpdateCellValues(const FMineGridMapCellUpdates& UpdatedMineGridMapCells)
{
	auto UpdatedCellCoordsIt = UpdatedMineGridMapCells.UpdatedGridMapCellCoords.CreateConstIterator();
	auto UpdatedCellValuesIt = UpdatedMineGridMapCells.UpdatedGridMapCellValues.CreateConstIterator();

	for (UpdatedCellCoordsIt, UpdatedCellValuesIt; UpdatedCellCoordsIt && UpdatedCellValuesIt; ++UpdatedCellCoordsIt, ++UpdatedCellValuesIt)
	{
		const FIntPoint Coords = *UpdatedCellCoordsIt;
		const EMineGridMapCell NewCellValue = *UpdatedCellValuesIt;

		if (AMineGridCellBase* CellActor = GridCoordsCells[Coords])
		{
			CellActor->UpdateCellValue(NewCellValue);
		}
	}
}

void AMineGridBase::SetGridCellExploded(const FIntPoint& ExplodedCoords)
{
	AMineGridCellBase** FoundGridCellPtr = GridCoordsCells.Find(ExplodedCoords);
	if (FoundGridCellPtr)
	{
		AMineGridCellBase* GridCellToExploded = *FoundGridCellPtr;

		// Tell our cell actor to update representation of exploded cell
		GridCellToExploded->SetCellExploded();
	}
}

// Called when the game starts or when spawned
void AMineGridBase::BeginPlay()
{
	Super::BeginPlay();
	
}

AMineGridCellBase* AMineGridBase::SpawnCellAt(const FIntPoint& CellCoords)
{
	const int XOffset = CellCoords.X;
	const int YOffset = CellCoords.Y;

	// Make position vector, offset from Grid location
	const FVector BlockLocation = FVector(XOffset, YOffset, 0.f) * CellSize + GetActorLocation();

	// Spawn a cell
	AMineGridCellBase* NewCell = GetWorld()->SpawnActor<AMineGridCellBase>(GridCellClass, BlockLocation, FRotator(0, 0, 0));

	if (NewCell)
	{
		// Setup owner of new cell
		NewCell->OwnerGrid = this;

		return NewCell;
	}

	return nullptr;
}
