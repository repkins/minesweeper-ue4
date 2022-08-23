// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minesweeper/Includes/MineGridMapCell.h"
#include "MineGridCellBase.generated.h"

/**
 * Represents single cell of owning grid in the world. Contains triggering box and various rendering components.
 */
UCLASS()
class MINESWEEPER_API AMineGridCellBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMineGridCellBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGridCell")
	class AMineGridBase* OwnerGrid;

	UFUNCTION()
	void UpdateCellValue(const EMineGridMapCell& NewCellValue);

protected:
	// Overlap volume to emit cell entering events
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGridCell")
	class UBoxComponent* TriggerBox;

	// In-game representation of cell
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGridCell")
	class UStaticMeshComponent* CellMesh;

	// Textual representation of cell value
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MineGridCell")
	class UTextRenderComponent* ValueText;

	// Material used to represent untriggered cell (undiscovered or revealed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MineGridCell")
	class UMaterialInterface* UntriggeredMaterial;

	// Material used to represent clear cell (no mine)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MineGridCell")
	class UMaterialInterface* ClearMaterial;

	// Material used to represent cell with "stepped-on" mine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MineGridCell")
	class UMaterialInterface* ExplodedMaterial;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnCharacterCellEnter(class ACharacter* EnteringCharacter);

};
