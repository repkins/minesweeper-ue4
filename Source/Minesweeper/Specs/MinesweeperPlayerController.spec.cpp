#include "Misc/AutomationTest.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"


BEGIN_DEFINE_SPEC(AMinesweeperPlayerControllerTest, "Minesweeper.MinesweeperPlayerController", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
	UWorld* World = nullptr;
	AMinesweeperPlayerControllerBase* Controller = nullptr;
	APawn* Pawn = nullptr;
END_DEFINE_SPEC(AMinesweeperPlayerControllerTest)

void AMinesweeperPlayerControllerTest::Define() 
{
	Describe("AddRemoveGridMapAreaCells", [this]() {
		It("should update grid map area: left overlapping", [this]() {
			// Setup
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			FURL URL;
			World->InitializeActorsForPlay(URL);
			World->BeginPlay();

			Pawn = NewObject<APawn>(World->PersistentLevel);
			Controller = NewObject<AMinesweeperPlayerControllerBase>(World->PersistentLevel);
			auto MineGrid = NewObject<AMineGridBase>(World->PersistentLevel);
			auto Component = NewObject<USceneComponent>(Pawn);

			Pawn->SetRootComponent(Component);
			Controller->Possess(Pawn);

			MineGrid->SetActorLocation(FVector::ZeroVector);

			auto MineGridActorProperty = FindFProperty<FObjectProperty>(Controller->GetClass(), TEXT("MineGridActor"));
			auto Controller_MineGridActorAddress = MineGridActorProperty->ContainerPtrToValuePtr<AMineGridBase*>(Controller);
			*Controller_MineGridActorAddress = MineGrid;

			auto MapAreaMaxHalfSizeXProperty = FindFProperty<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeX"));
			auto Controller_MapAreaMaxHalfSizeXAddress = MapAreaMaxHalfSizeXProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeXAddress = 7;

			auto MapAreaMaxHalfSizeYProperty = FindFProperty<FByteProperty>(Controller->GetClass(), TEXT("MapAreaMaxHalfSizeY"));
			auto Controller_MapAreaMaxHalfSizeYAddress = MapAreaMaxHalfSizeYProperty->ContainerPtrToValuePtr<uint8>(Controller);
			*Controller_MapAreaMaxHalfSizeYAddress = 4;

			auto CellSizeProperty = FindFProperty<FFloatProperty>(MineGrid->GetClass(), TEXT("CellSize"));
			auto MineGrid_CellSizeAddress = CellSizeProperty->ContainerPtrToValuePtr<float>(MineGrid);
			*MineGrid_CellSizeAddress = 100.f;

			FMineGridMap GivenGridMap;
			GivenGridMap.GridDimensions = FIntPoint(25, 20);
			GivenGridMap.StartCoords = FIntPoint::ZeroValue;
			GivenGridMap.EndCoords = GivenGridMap.GridDimensions - 1;
			GivenGridMap.Cells.Reserve(GivenGridMap.GridDimensions.X * GivenGridMap.GridDimensions.Y);
			for (int32 Y = GivenGridMap.StartCoords.Y; Y <= GivenGridMap.EndCoords.Y; Y++) {
				for (int32 X = GivenGridMap.StartCoords.X; X <= GivenGridMap.EndCoords.X; X++) {
					GivenGridMap.Cells.Emplace(FIntPoint(X, Y), EMineGridMapCell::MGMC_Undiscovered);
				}
			}

			auto MineGridMapAreaProperty = FindFProperty<FStructProperty>(Controller->GetClass(), TEXT("MineGridMapArea"));
			auto Controller_MineGridMapAreaAddress = MineGridMapAreaProperty->ContainerPtrToValuePtr<FMineGridMap>(Controller);
			auto& MineGridMapArea = *Controller_MineGridMapAreaAddress;

			// Prepare
			Pawn->SetActorLocation(FVector(1250.f, 950.f, 0.f));

			auto PrevPlayerRelGridCoordsProperty = FindFProperty<FStructProperty>(Controller->GetClass(), TEXT("PrevPlayerRelativeGridCoords"));
			auto &PrevPlayerRelGridCoords = *PrevPlayerRelGridCoordsProperty->ContainerPtrToValuePtr<FIntPoint>(Controller);
			PrevPlayerRelGridCoords = FIntPoint(12, 9);

			MineGridMapArea.StartCoords = PrevPlayerRelGridCoords;
			MineGridMapArea.EndCoords = PrevPlayerRelGridCoords - 1;

			auto ExpectedAreaBounds = FIntRect(FIntPoint(5, 5), FIntPoint(20, 14));
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

			// Teardown
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		});
	});
}
