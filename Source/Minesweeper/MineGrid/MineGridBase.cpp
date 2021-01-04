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

void AMineGridBase::ResetGridCells(const FMineGridMap& NewMineGridMap)
{
	auto CurrentGridCellsIterator = GridCoordsCells.CreateConstIterator();
	auto NewGridMapCellsIterator = NewMineGridMap.Cells.CreateConstIterator();

	FIntPoint NewStartCoords(0, 0);
	FIntPoint NewEndCoords(-1, -1);
	if (NewGridMapCellsIterator)
	{
		NewStartCoords = NewGridMapCellsIterator->Key;
		NewEndCoords = NewStartCoords + (NewMineGridMap.GridDimensions - FIntPoint(1, 1));
	}

	FIntPoint CurrentStartCoords(0, 0);
	FIntPoint CurrentEndCoords(-1, -1);
	if (CurrentGridCellsIterator)
	{
		CurrentStartCoords = CurrentGridCellsIterator->Key;
		CurrentEndCoords = CurrentStartCoords + (GridDimensions - FIntPoint(1, 1));
	}

	TMap<FIntPoint, AMineGridCellBase*> NewGridCoordsCells;
	while (CurrentGridCellsIterator || NewGridMapCellsIterator)
	{
		AMineGridCellBase* NewOrExistingActor = nullptr;
		FIntPoint NewOrExistingCoords;
		EMineGridMapCell NewCellValue;

		// Spawn new cell if both:
		//	1) new map cells iterator is valid
		//	2) and current new map cells iterator entry is not within coords range of current grid
		if (NewGridMapCellsIterator && 
			(!FMath::IsWithinInclusive(NewGridMapCellsIterator->Key.X, CurrentStartCoords.X, CurrentEndCoords.X) || 
			!FMath::IsWithinInclusive(NewGridMapCellsIterator->Key.Y, CurrentStartCoords.Y, CurrentEndCoords.Y)))
		{
			NewOrExistingActor = SpawnCellAt(NewGridMapCellsIterator->Key);
			NewOrExistingCoords = NewGridMapCellsIterator->Key;
			NewCellValue = NewGridMapCellsIterator->Value;
			++NewGridMapCellsIterator;
		}
		// Destroy redundant cell if both
		//	1) new grid map iterator is valid
		//	2) and current grid cells iterator entry is not within coords range of current grid
		else if (CurrentGridCellsIterator && 
			(!FMath::IsWithinInclusive(CurrentGridCellsIterator->Key.X, NewStartCoords.X, NewEndCoords.X) ||
			!FMath::IsWithinInclusive(CurrentGridCellsIterator->Key.Y, NewStartCoords.Y, NewEndCoords.Y)))
		{
			CurrentGridCellsIterator->Value->Destroy();
			++CurrentGridCellsIterator;
		}
		// Use existing cell if:
		//	1) both current cells and new cells map iterators are valid 
		//	2) and coords of both iterators current entries is equal
		else if (CurrentGridCellsIterator && NewGridMapCellsIterator && CurrentGridCellsIterator->Key == NewGridMapCellsIterator->Key)
		{
			NewOrExistingActor = CurrentGridCellsIterator->Value;
			NewOrExistingCoords = CurrentGridCellsIterator->Key;
			NewCellValue = NewGridMapCellsIterator->Value;
			++NewGridMapCellsIterator;
			++CurrentGridCellsIterator;
		}
		else
		{
			checkNoEntry();
		}

		if (NewOrExistingActor)
		{
			NewOrExistingActor->UpdateCellValue(NewCellValue);
			NewGridCoordsCells.Add(NewOrExistingCoords, NewOrExistingActor);
		}
	}

	// Finally transfer the ownership of cells&coords mappings
	GridCoordsCells = MoveTemp(NewGridCoordsCells);

	// Assign new grid dimensions
	GridDimensions = NewMineGridMap.GridDimensions;
}

void AMineGridBase::AddOrRemoveGridCells(const TMap<FIntPoint, EMineGridMapCell>& AddedGridMapCells, const TSet<FIntPoint>& RemovedGridMapCells, const FIntPoint& NewGridDimensions)
{
	GridCoordsCells.Reserve(NewGridDimensions.X * NewGridDimensions.Y);

	// Remove and destroy cell actors
	for (const FIntPoint& CoordsToDestroy : RemovedGridMapCells)
	{
		if (AMineGridCellBase* CellActor = GridCoordsCells.FindAndRemoveChecked(CoordsToDestroy))
		{
			CellActor->Destroy();
		}
	}

	// Spawn and add cell actors
	for (const TPair<FIntPoint, EMineGridMapCell>& CoordsCellEntryToAdd : AddedGridMapCells)
	{
		if (AMineGridCellBase* NewCellActor = SpawnCellAt(CoordsCellEntryToAdd.Key))
		{
			NewCellActor->UpdateCellValue(CoordsCellEntryToAdd.Value);
			GridCoordsCells.Emplace(CoordsCellEntryToAdd.Key, NewCellActor);
		}
	}

	// Update grid-dimensions
	GridDimensions = NewGridDimensions;
}

void AMineGridBase::UpdateCellValues(const TMap<FIntPoint, EMineGridMapCell>& UpdatedMineGridMapCells)
{
	for (const TPair<FIntPoint, EMineGridMapCell>& CoordsCellEntry : UpdatedMineGridMapCells)
	{
		const FIntPoint Coords = CoordsCellEntry.Key;
		const EMineGridMapCell NewCellValue = CoordsCellEntry.Value;

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
