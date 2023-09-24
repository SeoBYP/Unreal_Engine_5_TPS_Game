// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "UnrealSuviverGameCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Curves/CurveVector.h"
// Sets default values
AItem::AItem() :
	ItemName(FString("Default")),
	ItemCount(0),
	ItemRarity(EItemRarity::EIR_Common),
	ItemState(EItemState::EIS_Pickup),
	ZCurveTime(0.7f),
	ItemInterpStartLocation(FVector(0.f)),
	CameraTargetLocation(FVector(0.f)),
	bInterping(false),
	ItemInterpX(0.f),
	ItemInterpY(0.f),
	InterpInitialYawOffset(0.f),
	ItemType(EItemType::EIT_MAX),
	InterpLocIndex(0),
	MaterialIndex(0),
	bCanChangeCustomDepth(true),
	//Dynamic Material 
	GlowAmount(150.f),
	FresnelExponent(3.f),
	FresnelReflrectionFraction(4.f),
	PulseCurveTime(5.f),
	SlotIndex(0),
	bCharacterInventoryFull(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Visibility,
		ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaShpere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaShpere"));
	AreaShpere->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	//Hide PickupWidget
	if (PickupWidget) {
		PickupWidget->SetVisibility(false);
	}

	//Sets ActiveStars
	SetAcitiveStars();

	//Setup overlap for AreaSphere
	AreaShpere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	AreaShpere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);

	SetItemProperties(ItemState);

	//Set Custom Depth 
	InitializeCustomDepth();

	StartPulseTimer();
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor) 
	{
		AUnrealSuviverGameCharacter* SuviverGameCharacter = Cast<AUnrealSuviverGameCharacter>(OtherActor);
		if (SuviverGameCharacter) {
			SuviverGameCharacter->IncrementOverlappedItemCount(1);
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		AUnrealSuviverGameCharacter* SuviverGameCharacter = Cast<AUnrealSuviverGameCharacter>(OtherActor);
		if (SuviverGameCharacter) {
			SuviverGameCharacter->IncrementOverlappedItemCount(-1);
			SuviverGameCharacter->UnHighlightInventorySlot();
		}
	}
}

void AItem::SetAcitiveStars() {
	//The 0 element isn't used
	for (int32 i = 0; i <= 5; i++) {
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
		case EItemRarity::EIR_Damaged:
			ActiveStars[1] = true;
			break;
		case EItemRarity::EIR_Common:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			break;
		case EItemRarity::EIR_UnCommon:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			break;
		case EItemRarity::EIR_Rare:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			break;
		case EItemRarity::EIR_Legendary:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			ActiveStars[5] = true;
			break;
		default:
			break;
	}


}

void AItem::SetItemProperties(EItemState State)
{
	switch (State) {
		case EItemState::EIS_Pickup:
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			AreaShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			AreaShpere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(
				ECollisionChannel::ECC_Visibility,
				ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
		case EItemState::EIS_Equipped:
			PickupWidget->SetVisibility(false);
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			AreaShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaShpere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EItemState::EIS_Falling:
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetEnableGravity(true);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionResponseToChannel(
				ECollisionChannel::ECC_WorldStatic,
				ECollisionResponse::ECR_Block);

			AreaShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaShpere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EItemState::EIS_EquipInterping:
			PickupWidget->SetVisibility(false);
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			AreaShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaShpere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EItemState::EIS_Pickedup:
			PickupWidget->SetVisibility(false);
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(false);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			AreaShpere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaShpere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
	}
}

void AItem::FinishInterping()
{
	bInterping = false;
	if (Character) {
		//Subtract 1 from the Item Count of the interp loaction struct
		Character->IncreamentInterpLocItemCount(InterpLocIndex, -1);
		Character->GetPickupItem(this);
		Character->UnHighlightInventorySlot();
	}
	SetActorScale3D(FVector(1.f));
	DisableGlowMaterial();
	bCanChangeCustomDepth = true;
	DisableCustomDepth();
}

void AItem::ItemInterp(float DeltaTime)
{
	if (!bInterping) return;

	if(Character == nullptr)
		UE_LOG(LogTemp, Warning, TEXT("Character is null"));
	if (ItemZCurve == nullptr)
		UE_LOG(LogTemp, Warning, TEXT("ItemZCurve is null"));

	if (Character && ItemZCurve) 
	{
		// Elapsed time since we started ItemInterpTimer
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		UE_LOG(LogTemp, Warning, TEXT("ElapsedTime: %f"), ElapsedTime);
		// Get curve value corresponding to ElapsedTime
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);
		UE_LOG(LogTemp, Warning, TEXT("CurveValue: %f"), CurveValue);
		// Get the item's initial location when the curve started
		FVector ItemLocation = ItemInterpStartLocation;
		// Get location in front of the camera
		const FVector CameraInterpLocation{ GetInterpLoaction() };

		// Vector from Item to Camera Interp Location, X and Y are zeroed out
		const FVector ItemToCamera{ FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z) };
		// Scale factor to multiply with CurveValue
		const float DeltaZ = ItemToCamera.Size();

		const FVector CurrentLocation{ GetActorLocation() };
		// Interpolated X value
		const float InterpXValue = FMath::FInterpTo(
			CurrentLocation.X,
			CameraInterpLocation.X,
			DeltaTime,
			30.0f);
		// Interpolated Y value
		const float InterpYValue = FMath::FInterpTo(
			CurrentLocation.Y,
			CameraInterpLocation.Y,
			DeltaTime,
			30.f);

		// Set X and Y of ItemLocation to Interped values
		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;

		// Adding curve value to the Z component of the Initial Location (scaled by DeltaZ)
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

		// Camera rotation this frame
		const FRotator CameraRotation{ Character->GetThirdCamera()->GetComponentRotation() };
		// Camera rotation plus inital Yaw Offset
		FRotator ItemRotation{ 0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f };
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

		if (ItemScaleCurve)
		{
			const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
		}
	}

}

FVector AItem::GetInterpLoaction()
{
	if (Character == nullptr) return FVector(0.f);

	switch (ItemType)
	{
	case EItemType::EIT_Ammo:
		return Character->GetInterpLoaction(InterpLocIndex).SceneComponent->GetComponentLocation();
	case EItemType::EIT_Weapon:
		return Character->GetInterpLoaction(0).SceneComponent->GetComponentLocation();
	case EItemType::EIT_MAX:
		return FVector(0.f);
	default:
		return FVector(0.f);
	}
}

void AItem::PlayPickupSound(bool bForcePlaySound)
{
	if (Character == nullptr) return;
	if (bForcePlaySound) {
		if (PickupSound) {
			UGameplayStatics::PlaySound2D(this, PickupSound);
			return;
		}
	}
	if (Character->ShoudPlayPickupSound()) {
		Character->StartPickupSoundTimer();
		if (PickupSound) {
			UGameplayStatics::PlaySound2D(this, PickupSound);
		}
	}
}
void AItem::EnableCustomDepth()
{
	if (bCanChangeCustomDepth) {
		ItemMesh->SetRenderCustomDepth(true);
	}

}
void AItem::DisableCustomDepth()
{
	if (bCanChangeCustomDepth) {
		ItemMesh->SetRenderCustomDepth(false);
	}
}
void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}
void AItem::OnConstruction(const FTransform& Transform)
{
	//Load the data in the Item Rarity Data Table
	//Path to the Item Rarity Data Table
	FString RarityTablePath(TEXT("/Script/Engine.DataTable'/Game/Resources/DataTable/ItemRariyDataTable.ItemRariyDataTable'"));
	UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
	if (RarityTableObject) 
	{
		FItemRarityTable* RarityRow = nullptr;
		switch (ItemRarity)
		{
			case EItemRarity::EIR_Damaged:
				RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
				break;
			case EItemRarity::EIR_Common:
				RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
				break;
			case EItemRarity::EIR_UnCommon:
				RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("UnCommon"), TEXT(""));
				break;
			case EItemRarity::EIR_Rare:
				RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
				break;
			case EItemRarity::EIR_Legendary:
				RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
				break;
		}
		if (RarityRow) 
		{
			GlowColor = RarityRow->GlowColor;
			LightColor = RarityRow->LightColor;
			DarkColor = RarityRow->DarkColor;
			NumberOfStars = RarityRow->NumberOfStars;
			IconBackGround = RarityRow->IconBackGround;
			if (GetItemMesh()) {
				GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
			}
		}
	}

	if (MaterialInstance) {
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		EnableGlowMaterial();
	}
}
void AItem::EnableGlowMaterial()
{
	if (DynamicMaterialInstance) 
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}
void AItem::UpdatePulse()
{
	float ElapsedTime{};
	FVector CurveValue{};
	switch (ItemState) {
		case EItemState::EIS_Pickup:
			if (PulseCurve) {
				ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
				CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
			}
			break;
		case EItemState::EIS_EquipInterping:
			if (InterpPulseCurve) {
				ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
				CurveValue = InterpPulseCurve->GetVectorValue(ElapsedTime);
			}
			break;
	}
	if (DynamicMaterialInstance) {
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflrectionFraction"), CurveValue.Z * FresnelReflrectionFraction);
	}

	//const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(PulseTimer) };
	//if (PulseCurve) {
	//	const FVector CurveValue{ PulseCurve->GetVectorValue(ElapsedTime) };



	//}
}
void AItem::DisableGlowMaterial()
{
	if (DynamicMaterialInstance) 
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
	}
}
void AItem::PlayEquipSound(bool bForcePlaySound)
{
	if (Character == nullptr) return;

	if (bForcePlaySound) {
		if (EquipSound) {
			UGameplayStatics::PlaySound2D(this, EquipSound);
			return;
		}
	}

	if (Character->ShoudPlayEquipSound()) {
		Character->StartEquipSoundTimer();
		if (EquipSound) {
			UGameplayStatics::PlaySound2D(this, EquipSound);
		}
	}
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);

	//Get Curve values from PulseCurve and set Dynamic Material parameter
	UpdatePulse();
}

void AItem::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItem::StartPulseTimer()
{
	if (ItemState == EItemState::EIS_Pickup) {
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer,
			PulseCurveTime);
	}
}

// Called every frame
void AItem::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemProperties(State);
}

void AItem::StartItemCurve(AUnrealSuviverGameCharacter* Char, bool bForcePlaySound)
{
	Character = Char;

	// Get array index in InterpLoaction with the lower item count
	InterpLocIndex = Character->GetInterpLoactionIndex();
	Character->IncreamentInterpLocItemCount(InterpLocIndex, 1);

	PlayPickupSound(bForcePlaySound);

	ItemInterpStartLocation = GetActorLocation();

	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);

	GetWorldTimerManager().ClearTimer(PulseTimer);

	GetWorldTimerManager().SetTimer(
		ItemInterpTimer,
		this,
		&AItem::FinishInterping,
		ZCurveTime);

	//Get InitialYaw
	const float CameraRoataionYaw{ (float)Character->GetThirdCamera()->GetComponentRotation().Yaw};
	const float ItemRotationYaw{ (float)GetActorRotation().Yaw };

	InterpInitialYawOffset = ItemRotationYaw - CameraRoataionYaw;
	bCanChangeCustomDepth = false;
}