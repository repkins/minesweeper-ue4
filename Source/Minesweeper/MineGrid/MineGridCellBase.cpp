// Fill out your copyright notice in the Description page of Project Settings.


#include "MineGridCellBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Character.h"
#include "MineGridBase.h"

// Sets default values
AMineGridCellBase::AMineGridCellBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Setup trigger box component
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);

	// Set default trigger box collision conditions
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	TriggerBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	TriggerBox->SetMobility(EComponentMobility::Static);

	// Set default trigger box extent
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 20.f));

	// Setup cell mesh component
	CellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CellMesh"));
	CellMesh->SetupAttachment(GetRootComponent());

	CellMesh->SetMobility(EComponentMobility::Stationary);

	// Setup cell value text component
	ValueText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ValueText"));
	ValueText->SetupAttachment(GetRootComponent());
}

void AMineGridCellBase::UpdateCellValue(const EMineGridMapCell& NewCellValue)
{
	if (NewCellValue <= EMineGridMapCell::MGMC_Eight)
	{
		if (ValueText)
		{
			if (NewCellValue > EMineGridMapCell::MGMC_Zero)
			{
				// Set cell value if not zero
				FString NumOfMinesStr = FString::Printf(TEXT("%i"), (uint8)NewCellValue);
				ValueText->SetText(FText::FromString(NumOfMinesStr));
			}
			else
			{
				// Set empty string if zero
				ValueText->SetText(FText::FromString(""));
			}
		}

		if (ClearMaterial)
		{
			// Set appropriate material
			CellMesh->SetMaterial(0, ClearMaterial);
		}

		// No need to generate overlap events anymore so disable them to save resources
		TriggerBox->SetGenerateOverlapEvents(false);
	} 
	else if (NewCellValue == EMineGridMapCell::MGMC_Revealed)
	{
		if (ValueText)
		{
			// Set cell value
			FString AsteriskStr = TEXT("*");
			ValueText->SetText(FText::FromString(AsteriskStr));
		}

		if (UntriggeredMaterial)
		{
			// Set appropriate material
			CellMesh->SetMaterial(0, UntriggeredMaterial);
		}
	}
	else if (NewCellValue == EMineGridMapCell::MGMC_Exploded)
	{
		if (ValueText)
		{
			// Set cell value
			FString AsteriskStr = TEXT("*");
			ValueText->SetText(FText::FromString(AsteriskStr));
		}

		if (ExplodedMaterial)
		{
			// Set appropriate material
			CellMesh->SetMaterial(0, ExplodedMaterial);
		}
	}
	else if (NewCellValue == EMineGridMapCell::MGMC_Undiscovered)
	{
		if (ValueText)
		{
			// Set cell value
			FString EmptyStr = TEXT("");
			ValueText->SetText(FText::FromString(EmptyStr));
		}

		if (UntriggeredMaterial)
		{
			// Set appropriate material
			CellMesh->SetMaterial(0, UntriggeredMaterial);
		}

		// Enable generation of overlap events
		TriggerBox->SetGenerateOverlapEvents(true);
	}
}

// Called when the game starts or when spawned
void AMineGridCellBase::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMineGridCellBase::OnOverlapBegin);
}

void AMineGridCellBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor);
		if (OtherCharacter)
		{
			OnCharacterCellEnter(OtherCharacter);
		}
	}
}

void AMineGridCellBase::OnCharacterCellEnter_Implementation(ACharacter* EnteringCharacter)
{
	if (OwnerGrid)
	{
		OwnerGrid->HandleCharacterCellTriggering(this, EnteringCharacter);
	}
}
