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
	FFloatProperty* CellSizeProperty;
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

			auto MapAreaMaxHalfSizeXProperty = FindFieldChecked<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeX"));
			auto Controller_MapAreaMaxHalfSizeXAddress = MapAreaMaxHalfSizeXProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeXAddress = 2;

			auto MapAreaMaxHalfSizeYProperty = FindFieldChecked<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeY"));
			auto Controller_MapAreaMaxHalfSizeYAddress = MapAreaMaxHalfSizeYProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeYAddress = 1;

			CellSizeProperty = FindFieldChecked<FFloatProperty>(MineGrid->GetClass(), TEXT("CellSize"));
			auto MineGrid_CellSizeAddress = CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
			*MineGrid_CellSizeAddress = 100.f;

			GivenGridMap.GridDimensions = FIntPoint(11, 5);
			GivenGridMap.StartCoords = FIntPoint::ZeroValue;
			GivenGridMap.EndCoords = GivenGridMap.GridDimensions - 1;
			GivenGridMap.Cells.Reserve(GivenGridMap.GridDimensions.X * GivenGridMap.GridDimensions.Y);
			for (int32 Y = GivenGridMap.StartCoords.Y; Y <= GivenGridMap.EndCoords.Y; Y++) {
				for (int32 X = GivenGridMap.StartCoords.X; X <= GivenGridMap.EndCoords.X; X++) {
					GivenGridMap.Cells.Emplace(FIntPoint(X, Y), EMineGridMapCell::MGMC_Undiscovered);
				}
			}

			MineGridMapAreaProperty = FindFieldChecked<FStructProperty>(Controller->GetClass(), TEXT("MineGridMapArea"));
			PrevPlayerRelGridCoordsProperty = FindFieldChecked<FStructProperty>(Controller->GetClass(), TEXT("PrevPlayerRelativeGridCoords"));
		});

		TTuple<FIntPoint, FIntRect, FString> ExpectedForGivenData[] = {
			//  □□□□□□□□□□□
			//  □□□■■■■■□□□
			//  □□□■■◊■■□□□
			//  □□□■■■■■□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(5, 2), FIntRect(FIntPoint(3, 1), FIntPoint(7, 3)), TEXT("at map center")),

			//  □□□□□□□□□□□
			//  ■■□□□□□□□□□
			// ◊■■□□□□□□□□□
			//  ■■□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(-1, 2), FIntRect(FIntPoint(0, 1), FIntPoint(1, 3)), TEXT("at left map edge")),

			//  □□□□□□□□□□□
			//  □□□□□□□□□■■
			//  □□□□□□□□□■■◊
			//  □□□□□□□□□■■
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, 2), FIntRect(FIntPoint(9, 1), FIntPoint(10, 3)), TEXT("at right map edge")),
			//       ◊
			//  □□□■■■■■□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(5, -1), FIntRect(FIntPoint(3, 0), FIntPoint(7, 0)), TEXT("at top map edge")),

			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□■■■■■□□□
			//       ◊
			MakeTuple(FIntPoint(5, 5), FIntRect(FIntPoint(3, 4), FIntPoint(7, 4)), TEXT("at bottom map edge")),
			// ◊
			//  ■■□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(-1, -1), FIntRect(FIntPoint(0, 0), FIntPoint(1, 0)), TEXT("at left-top map edge")),
			//             ◊
			//  □□□□□□□□□■■
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			MakeTuple(FIntPoint(11, -1), FIntRect(FIntPoint(9, 0), FIntPoint(10, 0)), TEXT("at right-top map edge")),
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□■■
			//             ◊
			MakeTuple(FIntPoint(11, 5), FIntRect(FIntPoint(9, 4), FIntPoint(10, 4)), TEXT("at right-bottom map edge")),
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  □□□□□□□□□□□
			//  ■■□□□□□□□□□
			// ◊
			MakeTuple(FIntPoint(-1, 5), FIntRect(FIntPoint(0, 4), FIntPoint(1, 4)), TEXT("at left-bottom map edge")),
		};
		
		for (auto &DataRow : ExpectedForGivenData)
		{
			FIntPoint GivenPrevCoords;
			FIntRect ExpectedAreaBounds;
			FString DataRowDesc;

			Tie(GivenPrevCoords, ExpectedAreaBounds, DataRowDesc) = DataRow;

			It(FString::Printf(TEXT("should update grid map area: forced, %s"), *DataRowDesc), [this, GivenPrevCoords, ExpectedAreaBounds]() {
				// Prepare
				auto& PrevPlayerRelGridCoords = *PrevPlayerRelGridCoordsProperty->ContainerPtrToValuePtr<FIntPoint>(Controller);
				PrevPlayerRelGridCoords = GivenPrevCoords;

				auto &CellSize = *CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
				Pawn->SetActorLocation(FVector(GivenPrevCoords) * CellSize);

				auto &MineGridMapArea = *MineGridMapAreaProperty->ContainerPtrToValuePtr<FMineGridMap>(Controller);
				MineGridMapArea.StartCoords.X = FMath::Clamp(PrevPlayerRelGridCoords.X, GivenGridMap.StartCoords.X, GivenGridMap.EndCoords.X);
				MineGridMapArea.StartCoords.Y = FMath::Clamp(PrevPlayerRelGridCoords.Y, GivenGridMap.StartCoords.Y, GivenGridMap.EndCoords.Y);
				MineGridMapArea.EndCoords = MineGridMapArea.StartCoords - 1;

				auto ExpectedAreaDimensions = ExpectedAreaBounds.Size() + 1;
				auto ExpectedAreaCells = TMap<FIntPoint, EMineGridMapCell>();
				ExpectedAreaCells.Reserve(ExpectedAreaDimensions.X * ExpectedAreaDimensions.Y);
				for (int32 Y = ExpectedAreaBounds.Min.Y; Y <= ExpectedAreaBounds.Max.Y; Y++) {
					for (int32 X = ExpectedAreaBounds.Min.X; X <= ExpectedAreaBounds.Max.X; X++) {
						ExpectedAreaCells.Emplace(FIntPoint(X, Y), EMineGridMapCell::MGMC_Undiscovered);
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

		AfterEach([this]() {
			// Teardown
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		});
	});
}
