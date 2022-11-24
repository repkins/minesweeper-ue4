// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperPlayerControllerBase.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Minesweeper/GameMode/MinesweeperGameModeBase.h"
#include "Minesweeper/GameMode/MinesweeperGameStateBase.h"
#include "Minesweeper/HUD/MinesweeperHUDBase.h"

AMinesweeperPlayerControllerBase::AMinesweeperPlayerControllerBase(): Super()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	bIsLobbyLeader = false;
	MineGridClass = AMineGridBase::StaticClass();

	MapAreaMaxHalfSizeX = 8;
	MapAreaMaxHalfSizeY = 5;

	PrevPlayerRelativeGridCoords = FIntPoint(-1, -1);
	GridMapAreaVersion = 0;
}

void AMinesweeperPlayerControllerBase::NotifyGameStarted_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->HideGameOver();
		MinesweeperHUD->HideGameWin();
	}
}

void AMinesweeperPlayerControllerBase::NotifyGameOver_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->ShowGameOver();
	}
}

void AMinesweeperPlayerControllerBase::NotifyGameWin_Implementation()
{
	if (auto MinesweeperHUD = Cast<AMinesweeperHUDBase>(MyHUD))
	{
		MinesweeperHUD->ShowGameWin();
	}
}

void AMinesweeperPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// Finding grid actor
	MineGridActor = FindMineGridActor();
	if (MineGridActor)
	{
		// Bind to on entered coords event of grid actor
		MineGridActor->OnCharacterTriggeredCoords.AddDynamic(this, &AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MinesweeperPlayerController needs MineGrid actor placed in the level to work."));
		}
	}

	// Initializing LastCoords field
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			PrevPlayerRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);
		}
	}
}

void AMinesweeperPlayerControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AMinesweeperGameModeBase* MinesweeperGameMode = GetWorld()->GetAuthGameMode<AMinesweeperGameModeBase>())
	{
		const FMineGridMap& FullMineGridMap = MinesweeperGameMode->GetMineGridMap();

		// Compare grid map versions to determine if update is necessary, then proceed with update
		const uint32& FullGridMapVersion = MinesweeperGameMode->GetMineGridMapVersion();

		if (FullGridMapVersion != GridMapAreaVersion)
		{
			// Update GridMapArea values
			UpdateGridMapAreaCellValues(FullMineGridMap);

			// Update map version
			GridMapAreaVersion = FullGridMapVersion;
		}
		else
		{
			// Add&remove marginal cells of GridMapArea as necessary
			AddRemoveGridMapAreaCells(FullMineGridMap);
		}
	}
}

void AMinesweeperPlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllGridCells();
}

void AMinesweeperPlayerControllerBase::AddRemoveGridMapAreaCells(const FMineGridMap& MineGridMap, bool bForcedAddRemove)
{
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			const FIntPoint PawnRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);

			// Check if need update by determining if pawn (player) is at the same coords as before or 
			// it is forced to update, then proceed with adding & removing marginal cells if new pawn coords is different
			if (PawnRelativeGridCoords != PrevPlayerRelativeGridCoords || bForcedAddRemove)
			{
				// Define reference to grid map dimensions
				const FIntPoint& MapGridDimensions = MineGridMap.GridDimensions;

				// Determine starting & endings coords of map area
				const FIntPoint MapAreaMaxHalfSize = FIntPoint(MapAreaMaxHalfSizeX, MapAreaMaxHalfSizeY);
				const FIntRect MaxBounds(
					PawnRelativeGridCoords - MapAreaMaxHalfSize, 
					PawnRelativeGridCoords + MapAreaMaxHalfSize
				);

				// Allocate max size of authoritive mines area
				const FIntPoint MapAreaMaxSize = MaxBounds.Size() + FIntPoint(1, 1);
				MineGridMapArea.Cells.Reserve(MapAreaMaxSize.X * MapAreaMaxSize.Y);

				// Determine valid starting & endings coords of map "visible" area
				const FIntRect NewBounds(
					FIntPoint(
						MaxBounds.Min.X >= 0 ? MaxBounds.Min.X : FMath::Min(0, MaxBounds.Max.X + 1),
						MaxBounds.Min.Y >= 0 ? MaxBounds.Min.Y : FMath::Min(0, MaxBounds.Max.Y + 1)
					),
					FIntPoint(
						MaxBounds.Max.X <= MapGridDimensions.X - 1 ? MaxBounds.Max.X : FMath::Max(MapGridDimensions.X - 1, MaxBounds.Min.X - 1),
						MaxBounds.Max.Y <= MapGridDimensions.Y - 1 ? MaxBounds.Max.Y : FMath::Max(MapGridDimensions.Y - 1, MaxBounds.Min.Y - 1)
					)
				);

				// Retrieve old starting & endings coords of map area
				const FIntRect OldBounds(MineGridMapArea.StartCoords, MineGridMapArea.EndCoords);

				// Because start and end coords are inclusive, we need to increment by 1
				const FIntPoint MapAreaSize = NewBounds.Size() + FIntPoint(1, 1);

				FIntRect SideBounds;

				// Get center and extents of both new and old bounds for calculation needs
				FVector2D NewBoundsExtents = FVector2D(NewBounds.Size() + 1) / 2;
				FVector2D NewBoundsCenter = FVector2D(NewBounds.Min) + NewBoundsExtents;

				FVector2D OldBoundsExtents = FVector2D(OldBounds.Size() + 1) / 2;
				FVector2D OldBoundsCenter = FVector2D(OldBounds.Min) + OldBoundsExtents;

				FVector2D NewOldCentersDiff = NewBoundsCenter - OldBoundsCenter;
				FVector2D RotatedNewOldCentersDiff = NewOldCentersDiff;

				FVector2D RotatedNewBoundsExtents = NewBoundsExtents;
				FIntRect RotatedNewBounds = NewBounds;

				FVector2D RotatedOldBoundsCenter = OldBoundsCenter;
				FVector2D RotatedOldBoundsExtents = OldBoundsExtents;
				FVector2D RotatedOldBoundsExtentsWithDiff = RotatedOldBoundsExtents + RotatedNewOldCentersDiff;
				FIntRect RotatedOldBounds = OldBounds;
				FMatrix2x2 RotatedOldBoundsContDiffMatrix;

				// Here is our rotation matrix to do 90 degrees turn
				FMatrix2x2 RotationMatrix(
					0, 1,
					-1, 0
				);

				TArray<FIntRect> TempAdditiveSides;
				TArray<FIntRect> TempSubtractiveSides;
				TempAdditiveSides.Reserve(4);
				TempSubtractiveSides.Reserve(4);

				do {
					// Calculate difference represented as rect
					const FIntRect Diff = RotatedOldBounds - RotatedNewBounds;

					// Left side
					if (Diff.Min.X > 0)
					{
						// Additive
						SideBounds.Min = FIntPoint(RotatedNewBounds.Min);
						SideBounds.Max = FIntPoint(RotatedNewBounds.Min.X + Diff.Min.X - 1, RotatedNewBounds.Max.Y);
						TempAdditiveSides.Emplace(SideBounds);
					}
					else if (Diff.Min.X < 0)
					{
						// Subtractive
						SideBounds.Min = FIntPoint(RotatedOldBounds.Min);
						SideBounds.Max = FIntPoint(RotatedOldBounds.Min.X - Diff.Min.X - 1, RotatedOldBounds.Max.Y);
						TempSubtractiveSides.Emplace(SideBounds);
					}

					// Rotate new bounds
					RotatedNewBoundsExtents = RotationMatrix.TransformPoint(RotatedNewBoundsExtents * FVector2D(1, -1));

					RotatedNewBounds.Min = (NewBoundsCenter - RotatedNewBoundsExtents).IntPoint();
					RotatedNewBounds.Max = (NewBoundsCenter + RotatedNewBoundsExtents).IntPoint() - 1;

					// Mirror one axis of extents before rotation
					RotatedOldBoundsExtentsWithDiff = RotatedOldBoundsExtents + RotatedNewOldCentersDiff * FVector2D(1, -1);

					// Rotate old bounds "with" new bounds aka in relation to new bounds center
					RotatedOldBoundsExtentsWithDiff = RotationMatrix.TransformPoint(RotatedOldBoundsExtentsWithDiff * FVector2D(1, -1));
					RotatedNewOldCentersDiff = RotationMatrix.TransformPoint(RotatedNewOldCentersDiff);

					RotatedOldBoundsExtents = RotatedOldBoundsExtentsWithDiff - RotatedNewOldCentersDiff;
					RotatedOldBoundsCenter = NewBoundsCenter - RotatedNewOldCentersDiff;

					RotatedOldBounds.Min = (RotatedOldBoundsCenter - RotatedOldBoundsExtents).IntPoint();
					RotatedOldBounds.Max = (RotatedOldBoundsCenter + RotatedOldBoundsExtents).IntPoint() - 1;
				}
				while (RotatedOldBounds != OldBounds);

				// Define side arrays
				TArray<FIntRect> AdditiveSides;
				TArray<FIntRect> SubtractiveSides;
				AdditiveSides.Reserve(4);
				SubtractiveSides.Reserve(4);

				// Calculate difference represented as rect
				const FIntRect Diff = OldBounds - NewBounds;
					
				// Left side
				if (Diff.Min.X > 0)
				{
					// Additive
					SideBounds.Min = FIntPoint(NewBounds.Min);
					SideBounds.Max = FIntPoint(NewBounds.Min.X + Diff.Min.X - 1, NewBounds.Max.Y);
					AdditiveSides.Emplace(SideBounds);
				}
				else if (Diff.Min.X < 0)
				{
					// Subtractive
					SideBounds.Min = FIntPoint(OldBounds.Min);
					SideBounds.Max = FIntPoint(OldBounds.Min.X - Diff.Min.X - 1, OldBounds.Max.Y);
					SubtractiveSides.Emplace(SideBounds);
				}

				// Top side
				if (Diff.Min.Y > 0)
				{
					// Additive
					SideBounds.Min = FIntPoint(NewBounds.Min);
					SideBounds.Max = FIntPoint(NewBounds.Max.X, NewBounds.Min.Y + Diff.Min.Y - 1);
					AdditiveSides.Emplace(SideBounds);
				}
				else if (Diff.Min.Y < 0)
				{
					// Subtractive
					SideBounds.Min = FIntPoint(OldBounds.Min);
					SideBounds.Max = FIntPoint(OldBounds.Max.X, OldBounds.Min.Y - Diff.Min.Y - 1);
					SubtractiveSides.Emplace(SideBounds);
				}

				// Right side
				if (Diff.Max.X < 0)
				{
					// Additive
					SideBounds.Min = FIntPoint(NewBounds.Max.X + Diff.Max.X + 1, NewBounds.Min.Y);
					SideBounds.Max = FIntPoint(NewBounds.Max);
					AdditiveSides.Emplace(SideBounds);
				}
				else if (Diff.Max.X > 0)
				{
					// Subtractive
					SideBounds.Min = FIntPoint(OldBounds.Max.X - Diff.Max.X + 1, OldBounds.Min.Y);
					SideBounds.Max = FIntPoint(OldBounds.Max);
					SubtractiveSides.Emplace(SideBounds);
				}

				// Bottom side
				if (Diff.Max.Y < 0)
				{
					// Additive
					SideBounds.Min = FIntPoint(NewBounds.Min.X, NewBounds.Max.Y + Diff.Max.Y + 1);
					SideBounds.Max = FIntPoint(NewBounds.Max);
					AdditiveSides.Emplace(SideBounds);
				}
				else if (Diff.Max.Y > 0)
				{
					// Subtractive
					SideBounds.Min = FIntPoint(OldBounds.Min.X, OldBounds.Max.Y - Diff.Max.Y + 1);
					SideBounds.Max = FIntPoint(OldBounds.Max);
					SubtractiveSides.Emplace(SideBounds);
				}

				// Define removed & added cell containers
				FMineGridMapChanges GridMapChanges;
				GridMapChanges.NewGridDimensions = MapAreaSize;

				// Start with subtractive bounds first
				for (const auto &SubtractiveBounds : SubtractiveSides)
				{
					if (AdditiveSides.Contains(SubtractiveBounds))
					{
						continue;
					}

					for (int32 Y = SubtractiveBounds.Min.Y; Y <= SubtractiveBounds.Max.Y; ++Y)
					{
						for (int32 X = SubtractiveBounds.Min.X; X <= SubtractiveBounds.Max.X; ++X)
						{
							FIntPoint Coords(X, Y);

							GridMapChanges.RemovedGridMapCells.AddUnique(Coords);
						}
					}
				}

				// Then with additive bounds
				for (const auto &AdditiveBounds : AdditiveSides)
				{
					if (SubtractiveSides.Contains(AdditiveBounds))
					{
						continue;
					}

					for (int32 Y = AdditiveBounds.Min.Y; Y <= AdditiveBounds.Max.Y; ++Y)
					{
						for (int32 X = AdditiveBounds.Min.X; X <= AdditiveBounds.Max.X; ++X)
						{
							FIntPoint Coords(X, Y);
							EMineGridMapCell CellValue = MineGridMap.Cells[Coords];

							if (!GridMapChanges.AddedGridMapCellCoords.Contains(Coords))
							{
								GridMapChanges.AddedGridMapCellCoords.Add(Coords);
								GridMapChanges.AddedGridMapCellValues.Add(CellValue);
							}
						}
					}
				}

				MineGridMapArea.GridDimensions = MapAreaSize;
				MineGridMapArea.StartCoords = NewBounds.Min;
				MineGridMapArea.EndCoords = NewBounds.Max;

				// Finally apply changes
				ApplyAddedRemovedGridCells(GridMapChanges);

				// Remember player coords used to process
				PrevPlayerRelativeGridCoords = PawnRelativeGridCoords;
			}
		}
	}
}

void AMinesweeperPlayerControllerBase::ClearAllGridCells()
{
	FMineGridMapChanges GridMapChanges;
	GridMapChanges.NewGridDimensions = FIntPoint::ZeroValue;

	for (auto CellToRemove : MineGridMapArea.Cells)
	{
		const FIntPoint CoordsToRemoveAt = CellToRemove.Key;

		GridMapChanges.RemovedGridMapCells.AddUnique(CoordsToRemoveAt);
	}

	ApplyAddedRemovedGridCells(GridMapChanges);
}

void AMinesweeperPlayerControllerBase::UpdateGridMapAreaCellValues(const FMineGridMap& MineGridMap)
{
	FMineGridMapCellUpdates CellsUpdate;

	for (TPair<FIntPoint, EMineGridMapCell>& CoordsCellEntry : MineGridMapArea.Cells)
	{
		const FIntPoint Coords = CoordsCellEntry.Key;
		const EMineGridMapCell NewCellValue = MineGridMap.Cells[Coords];

		if (CoordsCellEntry.Value != NewCellValue)
		{
			CellsUpdate.UpdatedGridMapCellCoords.Emplace(Coords);
			CellsUpdate.UpdatedGridMapCellValues.Emplace(NewCellValue);
		}
	}

	if (CellsUpdate.UpdatedGridMapCellCoords.Num() > 0)
	{
		ApplyUpdatedGridCellValues(CellsUpdate);
	}
}

void AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords(const FIntPoint& EnteredCoords)
{
	OnPlayerTriggeredCoords.Broadcast(EnteredCoords);
}

void AMinesweeperPlayerControllerBase::SelectNewGame_Implementation(const uint8 MapSize)
{
	OnPlayerNewGame.Broadcast(MapSize);
}

void AMinesweeperPlayerControllerBase::ApplyAddedRemovedGridCells_Implementation(const FMineGridMapChanges& GridMapChanges)
{
	// 
	// 1. Update area of map according to new changes
	// 

	for (const FIntPoint& RemovedCellCoords : GridMapChanges.RemovedGridMapCells)
	{
		MineGridMapArea.Cells.Remove(RemovedCellCoords);
	}

	auto AddedCoordsIt = GridMapChanges.AddedGridMapCellCoords.CreateConstIterator();
	auto AddedValuesIt = GridMapChanges.AddedGridMapCellValues.CreateConstIterator();
	for (AddedCoordsIt, AddedValuesIt; AddedCoordsIt && AddedValuesIt; ++AddedCoordsIt, ++AddedValuesIt)
	{
		MineGridMapArea.Cells.Emplace(*AddedCoordsIt, *AddedValuesIt);
	}

	// 
	// 2. Update visible representation of area
	//
	if (MineGridActor)
	{
		MineGridActor->AddOrRemoveGridCells(GridMapChanges);
	}
}

void AMinesweeperPlayerControllerBase::ApplyUpdatedGridCellValues_Implementation(const FMineGridMapCellUpdates& UpdatedCells)
{
	auto UpdatedCoordsIt = UpdatedCells.UpdatedGridMapCellCoords.CreateConstIterator();
	auto UpdatedValuesIt = UpdatedCells.UpdatedGridMapCellValues.CreateConstIterator();
	for (UpdatedCoordsIt, UpdatedValuesIt; UpdatedCoordsIt && UpdatedValuesIt; ++UpdatedCoordsIt, ++UpdatedValuesIt)
	{
		MineGridMapArea.Cells[*UpdatedCoordsIt] = *UpdatedValuesIt;
	}

	if (MineGridActor)
	{
		MineGridActor->UpdateCellValues(UpdatedCells);
	}
}

AMineGridBase* AMinesweeperPlayerControllerBase::FindMineGridActor()
{
	UWorld* World = GetWorld();
	for (TActorIterator<AMineGridBase> ActorIt(World, MineGridClass); ActorIt; ++ActorIt)
	{
		AMineGridBase* MineGrid = *ActorIt;
		if (MineGrid)
		{
			return MineGrid;
		}
	}
	return nullptr;
}

FIntPoint AMinesweeperPlayerControllerBase::GetPawnRelativeLocationOfGrid(APawn* PlayerPawn, AMineGridBase* MineGrid)
{
	// Get relative grid coords of player pawn
	const FVector PawnAbsoluteLocation = PlayerPawn->GetActorLocation();
	const FVector MineGridLocation = MineGrid->GetActorLocation();
	const float CellSize = MineGrid->GetCellSize();

	const FVector PawnRelativeLocation = PawnAbsoluteLocation - MineGridLocation;

	int32 PawnRelativeLocationX = FMath::TruncToInt(PawnRelativeLocation.X);
	int32 PawnRelativeLocationY = FMath::TruncToInt(PawnRelativeLocation.Y);

	FIntPoint PawnRelativeGridCoords = FIntPoint(PawnRelativeLocationX, PawnRelativeLocationY) / CellSize;

	return PawnRelativeGridCoords;
}

void AMinesweeperPlayerControllerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMinesweeperPlayerControllerBase, bIsLobbyLeader);
}
