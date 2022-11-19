#include "Misc/AutomationTest.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"

BEGIN_DEFINE_SPEC(AMinesweeperPlayerControllerTest, "Minesweeper.MinesweeperPlayerController", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
	UWorld* World = nullptr;
	AMinesweeperPlayerControllerBase* Controller = nullptr;
	APawn* Pawn = nullptr;
	AMineGridBase* MineGrid = nullptr;

	FMineGridMap GivenGridMap;

	FStructProperty* MineGridMapAreaProperty;
	FStructProperty* PrevPlayerRelGridCoordsProperty;
	FProperty* GridCellRefCountsProperty;
	FProperty* GridCoordsCellsProperty;
	FFloatProperty* CellSizeProperty;
	FByteProperty* MapAreaMaxHalfSizeXProperty;
	FByteProperty* MapAreaMaxHalfSizeYProperty;
END_DEFINE_SPEC(AMinesweeperPlayerControllerTest)

void AMinesweeperPlayerControllerTest::Define() 
{
	Describe("AddRemoveGridMapAreaCells", [this]() {
		BeforeEach([this]() {
			// Setup
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FURL URL;
			World->InitializeActorsForPlay(URL);
			World->BeginPlay();

			Pawn = NewObject<APawn>(World->PersistentLevel);
			Controller = NewObject<AMinesweeperPlayerControllerBase>(World->PersistentLevel);
			MineGrid = NewObject<AMineGridBase>(World->PersistentLevel);
			auto Component = NewObject<USceneComponent>(Pawn);

			Pawn->SetRootComponent(Component);
			Controller->Possess(Pawn);

			MineGrid->SetActorLocation(FVector::ZeroVector);

			auto MineGridActorProperty = FindFieldChecked<FObjectProperty>(Controller->GetClass(), TEXT("MineGridActor"));
			auto Controller_MineGridActorAddress = MineGridActorProperty->ContainerPtrToValuePtr<AMineGridBase*>(Controller);
			*Controller_MineGridActorAddress = MineGrid;

			MapAreaMaxHalfSizeXProperty = FindFieldChecked<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeX"));
			auto Controller_MapAreaMaxHalfSizeXAddress = MapAreaMaxHalfSizeXProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeXAddress = 2;

			MapAreaMaxHalfSizeYProperty = FindFieldChecked<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeY"));
			auto Controller_MapAreaMaxHalfSizeYAddress = MapAreaMaxHalfSizeYProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeYAddress = 1;

			CellSizeProperty = FindFieldChecked<FFloatProperty>(MineGrid->GetClass(), TEXT("CellSize"));
			auto MineGrid_CellSizeAddress = CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
			*MineGrid_CellSizeAddress = 100.f;

			MineGridMapAreaProperty = FindFieldChecked<FStructProperty>(Controller->GetClass(), TEXT("MineGridMapArea"));
			PrevPlayerRelGridCoordsProperty = FindFieldChecked<FStructProperty>(Controller->GetClass(), TEXT("PrevPlayerRelativeGridCoords"));
			GridCellRefCountsProperty = FindFieldChecked<FProperty>(MineGrid->GetClass(), TEXT("GridCellRefCounts"));
			GridCoordsCellsProperty = FindFieldChecked<FProperty>(MineGrid->GetClass(), TEXT("GridCoordsCells"));
		});

		///
		/// Forced
		///

		TTuple<FIntPoint, FIntPoint, FIntRect, FString> ExpectedForGivenData[] = {
			// 
			//      ■■■
			//      ■◊■
			//      ■■■
			MakeTuple(FIntPoint(3, 3), FIntPoint(1, 1), FIntRect(FIntPoint(0, 0), FIntPoint(2, 2)), TEXT("at tiny map center")),

			//  □□□□□□□□□□□
			//  □□□■■■■■□□□
			//  □□□■■◊■■□□□
			//  □□□■■■■■□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(5, 2), FIntRect(FIntPoint(3, 1), FIntPoint(7, 3)), TEXT("at map center")),

			//  □□□□□□□□□□□
			//  ■■□□□□□□□□□
			// ◊■■□□□□□□□□□
			//  ■■□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(-1, 2), FIntRect(FIntPoint(0, 1), FIntPoint(1, 3)), TEXT("at left map edge")),

			//  □□□□□□□□□□□
			//  □□□□□□□□□■■
			//  □□□□□□□□□■■◊
			//  □□□□□□□□□■■
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(11, 2), FIntRect(FIntPoint(9, 1), FIntPoint(10, 3)), TEXT("at right map edge")),
			//       ◊
			//  □□□■■■■■□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(5, -1), FIntRect(FIntPoint(3, 0), FIntPoint(7, 0)), TEXT("at top map edge")),

			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□■■■■■□□□
			//       ◊
			MakeTuple(FIntPoint(11, 5), FIntPoint(5, 5), FIntRect(FIntPoint(3, 4), FIntPoint(7, 4)), TEXT("at bottom map edge")),
			// ◊
			//  ■■□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(-1, -1), FIntRect(FIntPoint(0, 0), FIntPoint(1, 0)), TEXT("at left-top map edge")),
			//             ◊
			//  □□□□□□□□□■■
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 5), FIntPoint(11, -1), FIntRect(FIntPoint(9, 0), FIntPoint(10, 0)), TEXT("at right-top map edge")),

			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□■■
			//             ◊
			MakeTuple(FIntPoint(11, 5), FIntPoint(11, 5), FIntRect(FIntPoint(9, 4), FIntPoint(10, 4)), TEXT("at right-bottom map edge")),
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  ■■□□□□□□□□□
			// ◊
			MakeTuple(FIntPoint(11, 5), FIntPoint(-1, 5), FIntRect(FIntPoint(0, 4), FIntPoint(1, 4)), TEXT("at left-bottom map edge")),
		};
		
		for (auto &DataRow : ExpectedForGivenData)
		{
			FIntPoint GivenMapSize;
			FIntPoint GivenPrevCoords;
			FIntRect ExpectedAreaBounds;
			FString DataRowDesc;

			Tie(GivenMapSize, GivenPrevCoords, ExpectedAreaBounds, DataRowDesc) = DataRow;

			It(FString::Printf(TEXT("should update grid map area: forced, %s"), *DataRowDesc), [this, GivenMapSize, GivenPrevCoords, ExpectedAreaBounds]() {
				// Prepare
				auto& PrevPlayerRelGridCoords = *PrevPlayerRelGridCoordsProperty->ContainerPtrToValuePtr<FIntPoint>(Controller);
				PrevPlayerRelGridCoords = GivenPrevCoords;

				auto &CellSize = *CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
				Pawn->SetActorLocation(FVector(GivenPrevCoords) * CellSize);

				GivenGridMap.GridDimensions = GivenMapSize;
				GivenGridMap.StartCoords = FIntPoint::ZeroValue;
				GivenGridMap.EndCoords = GivenGridMap.GridDimensions - 1;
				GivenGridMap.Cells.Reserve(GivenGridMap.GridDimensions.X* GivenGridMap.GridDimensions.Y);
				for (int32 Y = GivenGridMap.StartCoords.Y; Y <= GivenGridMap.EndCoords.Y; Y++) {
					for (int32 X = GivenGridMap.StartCoords.X; X <= GivenGridMap.EndCoords.X; X++) {
						GivenGridMap.Cells.Emplace(FIntPoint(X, Y), (EMineGridMapCell)FMath::RandRange(0, 8));
					}
				}

				auto &MineGridMapArea = *MineGridMapAreaProperty->ContainerPtrToValuePtr<FMineGridMap>(Controller);
				MineGridMapArea.StartCoords.X = FMath::Clamp(PrevPlayerRelGridCoords.X, GivenGridMap.StartCoords.X, GivenGridMap.EndCoords.X);
				MineGridMapArea.StartCoords.Y = FMath::Clamp(PrevPlayerRelGridCoords.Y, GivenGridMap.StartCoords.Y, GivenGridMap.EndCoords.Y);
				MineGridMapArea.EndCoords = MineGridMapArea.StartCoords - 1;

				auto ExpectedAreaDimensions = ExpectedAreaBounds.Size() + 1;
				auto ExpectedAreaCells = TMap<FIntPoint, EMineGridMapCell>();
				ExpectedAreaCells.Reserve(ExpectedAreaDimensions.X * ExpectedAreaDimensions.Y);
				for (int32 Y = ExpectedAreaBounds.Min.Y; Y <= ExpectedAreaBounds.Max.Y; Y++) {
					for (int32 X = ExpectedAreaBounds.Min.X; X <= ExpectedAreaBounds.Max.X; X++) {
						ExpectedAreaCells.Emplace(FIntPoint(X, Y), GivenGridMap.Cells[FIntPoint(X, Y)]);
					}
				}

				// Act
				Controller->AddRemoveGridMapAreaCells(GivenGridMap, true);

				// Assert
				TestEqual(TEXT("MineGridMapArea.StartCoords"), MineGridMapArea.StartCoords, ExpectedAreaBounds.Min);
				TestEqual(TEXT("MineGridMapArea.EndCoords"), MineGridMapArea.EndCoords, ExpectedAreaBounds.Max);
				TestEqual(TEXT("MineGridMapArea.GridDimensions"), MineGridMapArea.GridDimensions, ExpectedAreaDimensions);

				TestTrue(TEXT("MineGridMapArea.Cells.OrderIndependentCompareEqual(expected)"), MineGridMapArea.Cells.OrderIndependentCompareEqual(ExpectedAreaCells));
			});
		}

		///
		/// Moving from center
		///
		Describe("Moving", [this]() 
		{
			TTuple<FIntPoint, FIntPoint, FIntRect, FString> ExpectedForGivenData[] = {
				//  □□□□□□□□□□□
				//  □□■■■■■□□□□
				//  □□■■◊■■□□□□
				//  □□■■■■■□□□□
				//  □□□□□□□□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(4, 2), FIntRect(FIntPoint(2, 1), FIntPoint(6, 3)), TEXT("to left")),

				//  □□□□□□□□□□□
				//  □□□□■■■■■□□
				//  □□□□■■◊■■□□
				//  □□□□■■■■■□□
				//  □□□□□□□□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(6, 2), FIntRect(FIntPoint(4, 1), FIntPoint(8, 3)), TEXT("to right")),

				//  □□□■■■■■□□□
				//  □□□■■◊■■□□□
				//  □□□■■■■■□□□
				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(5, 1), FIntRect(FIntPoint(3, 0), FIntPoint(7, 2)), TEXT("to top")),

				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				//  □□□■■■■■□□□
				//  □□□■■◊■■□□□
				//  □□□■■■■■□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(5, 3), FIntRect(FIntPoint(3, 2), FIntPoint(7, 4)), TEXT("to bottom")),

				//  □□■■■■■□□□□
				//  □□■■◊■■□□□□
				//  □□■■■■■□□□□
				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(4, 1), FIntRect(FIntPoint(2, 0), FIntPoint(6, 2)), TEXT("to top-left")),

				//  □□□□■■■■■□□
				//  □□□□■■◊■■□□
				//  □□□□■■■■■□□
				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(6, 1), FIntRect(FIntPoint(4, 0), FIntPoint(8, 2)), TEXT("to top-right")),

				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				//  □□□□■■■■■□□
				//  □□□□■■◊■■□□
				//  □□□□■■■■■□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(6, 3), FIntRect(FIntPoint(4, 2), FIntPoint(8, 4)), TEXT("to bottom-right")),

				//  □□□□□□□□□□□
				//  □□□□□□□□□□□
				//  □□■■■■■□□□□
				//  □□■■◊■■□□□□
				//  □□■■■■■□□□□
				MakeTuple(FIntPoint(11, 5), FIntPoint(4, 3), FIntRect(FIntPoint(2, 2), FIntPoint(6, 4)), TEXT("to bottom-left")),

				//     ■■■■□
				//     ■◊■■□
				//     ■■■■□
				MakeTuple(FIntPoint(5, 3), FIntPoint(1, 1), FIntRect(FIntPoint(0, 0), FIntPoint(3, 2)), TEXT("to left map edge")),

				//     □■■■■
				//     □■■◊■
				//     □■■■■
				MakeTuple(FIntPoint(5, 3), FIntPoint(3, 1), FIntRect(FIntPoint(1, 0), FIntPoint(4, 2)), TEXT("to right map edge")),

				//     ■■◊■■
				//     ■■■■■
				//     □□□□□
				MakeTuple(FIntPoint(5, 3), FIntPoint(2, 0), FIntRect(FIntPoint(0, 0), FIntPoint(4, 1)), TEXT("to top map edge")),

				//     □□□□□
				//     ■■■■■
				//     ■■◊■■
				MakeTuple(FIntPoint(5, 3), FIntPoint(2, 2), FIntRect(FIntPoint(0, 1), FIntPoint(4, 2)), TEXT("to bottom map edge")),
			};

			for (auto& DataRow : ExpectedForGivenData)
			{
				FIntPoint GivenMapSize;
				FIntPoint GivenNewCoords;
				FIntRect ExpectedAreaBounds;
				FString DataRowDesc;

				Tie(GivenMapSize, GivenNewCoords, ExpectedAreaBounds, DataRowDesc) = DataRow;

				It(FString::Printf(TEXT("should update grid map area: moving, %s"), *DataRowDesc), [this, GivenMapSize, GivenNewCoords, ExpectedAreaBounds]() {
					// Prepare
					auto& PrevPlayerRelGridCoords = *PrevPlayerRelGridCoordsProperty->ContainerPtrToValuePtr<FIntPoint>(Controller);
					PrevPlayerRelGridCoords = GivenMapSize / 2;

					auto& CellSize = *CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
					Pawn->SetActorLocation(FVector(GivenNewCoords) * CellSize);

					GivenGridMap.GridDimensions = GivenMapSize;
					GivenGridMap.StartCoords = FIntPoint::ZeroValue;
					GivenGridMap.EndCoords = GivenGridMap.GridDimensions - 1;
					GivenGridMap.Cells.Reserve(GivenGridMap.GridDimensions.X * GivenGridMap.GridDimensions.Y);
					for (int32 Y = GivenGridMap.StartCoords.Y; Y <= GivenGridMap.EndCoords.Y; Y++) {
						for (int32 X = GivenGridMap.StartCoords.X; X <= GivenGridMap.EndCoords.X; X++) {
							GivenGridMap.Cells.Emplace(FIntPoint(X, Y), (EMineGridMapCell)FMath::RandRange(0, 8));
						}
					}

					auto& MineGridMapArea = *MineGridMapAreaProperty->ContainerPtrToValuePtr<FMineGridMap>(Controller);
					auto& MapAreaMaxHalfSizeX = *MapAreaMaxHalfSizeXProperty->ContainerPtrToValuePtr<uint8>(Controller);
					auto& MapAreaMaxHalfSizeY = *MapAreaMaxHalfSizeYProperty->ContainerPtrToValuePtr<uint8>(Controller);
					auto& GridCellRefCounts = *GridCellRefCountsProperty->ContainerPtrToValuePtr<TMap<FIntPoint, uint8>>(MineGrid);
					auto& GridCoordsCells = *GridCoordsCellsProperty->ContainerPtrToValuePtr<TMap<FIntPoint, void*>>(MineGrid);
					MineGridMapArea.StartCoords.X = FMath::Clamp(PrevPlayerRelGridCoords.X - MapAreaMaxHalfSizeX, GivenGridMap.StartCoords.X, GivenGridMap.EndCoords.X);
					MineGridMapArea.StartCoords.Y = FMath::Clamp(PrevPlayerRelGridCoords.Y - MapAreaMaxHalfSizeY, GivenGridMap.StartCoords.Y, GivenGridMap.EndCoords.Y);
					MineGridMapArea.EndCoords.X = FMath::Clamp(PrevPlayerRelGridCoords.X + MapAreaMaxHalfSizeX, GivenGridMap.StartCoords.X, GivenGridMap.EndCoords.X);
					MineGridMapArea.EndCoords.Y = FMath::Clamp(PrevPlayerRelGridCoords.Y + MapAreaMaxHalfSizeY, GivenGridMap.StartCoords.Y, GivenGridMap.EndCoords.Y);
					MineGridMapArea.GridDimensions = MineGridMapArea.EndCoords - MineGridMapArea.StartCoords + 1;
					MineGridMapArea.Cells.Reserve(MineGridMapArea.GridDimensions.X* MineGridMapArea.GridDimensions.Y);
					GridCellRefCounts.Reserve(MineGridMapArea.GridDimensions.X * MineGridMapArea.GridDimensions.Y);
					for (int32 Y = MineGridMapArea.StartCoords.Y; Y <= MineGridMapArea.EndCoords.Y; Y++) {
						for (int32 X = MineGridMapArea.StartCoords.X; X <= MineGridMapArea.EndCoords.X; X++) {
							MineGridMapArea.Cells.Emplace(FIntPoint(X, Y), GivenGridMap.Cells[FIntPoint(X, Y)]);
							GridCellRefCounts.Emplace(FIntPoint(X, Y), 1);
							GridCoordsCells.Emplace(FIntPoint(X, Y), nullptr);
						}
					}

					auto ExpectedAreaDimensions = ExpectedAreaBounds.Size() + 1;
					auto ExpectedAreaCells = TMap<FIntPoint, EMineGridMapCell>();
					ExpectedAreaCells.Reserve(ExpectedAreaDimensions.X * ExpectedAreaDimensions.Y);
					for (int32 Y = ExpectedAreaBounds.Min.Y; Y <= ExpectedAreaBounds.Max.Y; Y++) {
						for (int32 X = ExpectedAreaBounds.Min.X; X <= ExpectedAreaBounds.Max.X; X++) {
							ExpectedAreaCells.Emplace(FIntPoint(X, Y), GivenGridMap.Cells[FIntPoint(X, Y)]);
						}
					}

					// Act
					Controller->AddRemoveGridMapAreaCells(GivenGridMap, false);

					// Assert
					TestEqual(TEXT("MineGridMapArea.StartCoords"), MineGridMapArea.StartCoords, ExpectedAreaBounds.Min);
					TestEqual(TEXT("MineGridMapArea.EndCoords"), MineGridMapArea.EndCoords, ExpectedAreaBounds.Max);
					TestEqual(TEXT("MineGridMapArea.GridDimensions"), MineGridMapArea.GridDimensions, ExpectedAreaDimensions);

					TestTrue(TEXT("MineGridMapArea.Cells.OrderIndependentCompareEqual(expected)"), MineGridMapArea.Cells.OrderIndependentCompareEqual(ExpectedAreaCells));
				});
			}
		});

		

		AfterEach([this]() {
			// Teardown
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		});
	});
}
