// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealSuviverGameCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Item.h"
#include "Weapon.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Ammo.h"
#include "WeaponType.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UnrealSuviverGame.h"
#include "BulletHitInterface.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
//////////////////////////////////////////////////////////////////////////
// AUnrealSuviverGameCharacter

AUnrealSuviverGameCharacter::AUnrealSuviverGameCharacter() :
	//Base Turn Rate / Look Up Rate
	BaseLockUpRate(45.0f),
	BaseTurnRate(45.0f),
	HipTurnRate(90.0f),
	HipLockUpRate(90.0f),
	AimingTurnRate(20.f),
	AimingLockUpRate(20.f),
	MouseHipTurnRate(1.0f),
	MouseHipLockUpRate(1.0f),
	MouseAimingTurnRate(0.4f),
	MouseAimingLockUpRate(0.4f),
	bAiming(false),
	CameraDefaultFOV(0.0f),
	CameraZoomedFOV(25.0f),
	CameraCurrentFOV(0.0f),
	ZoomInterpSpeed(20.f),
	//Crosshair spread Factors
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	//Bullet Fire Timer variables
	ShootTimeDuration(0.05f),
	bFiringbullet(false),
	//Automatic Fire variable
	bShouldFire(true),
	bFireButtonPressed(false),
	bShuouldTraceForItems(false),
	CameraInterpDistance(250.f),
	CameraInterpElevation(65.f),
	Starting9mmAmmo(85),
	StartingARAmmo(120),
	CombatState(ECombatState::ECS_Unoccupied),
	bCrouching(false),
	BaseMovementSpeed(650.f),
	CrouchMovementSpeed(300.f),
	CrouchingCapsuleHalfHeight(44.f),
	StandingCapsuleHalfHeight(88.f),
	BaseGroundFriction(2.f),
	CrouchGroundFriction(100.f),
	bAimingButtonPressed(false),
	bShouldPlayPickupSound(true),
	bShouldPlayEquipSound(true),
	PickupSoundResetTime(0.2f),
	EquipSoundResetTime(0.2f),
	HighlightedSlot(-1),
	Health(100.f),
	MaxHealth(100.f),
	StunChance(0.25f),
	bDead(false)
{
	PrimaryActorTick.bCanEverTick = true;

	//Create 카메라 Boom
	CameraBoom = CreateAbstractDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.0f;
	CameraBoom->bUsePawnControlRotation = true;// 콘트롤러를 기준으로 회전 가능ㄴ
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	ThirdCamera = CreateAbstractDefaultSubobject<UCameraComponent>(TEXT("ThirdCamera"));
	ThirdCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ThirdCamera->bUsePawnControlRotation = false;// 카메라는 회전하지 않는다

	//키보드 입력으로 회전 X
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	//Configure Character Movement
	GetCharacterMovement()->bOrientRotationToMovement = false;//Character Moves in the Direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);//Rotation Rate
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	//Craete Interpolation Component
	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetThirdCamera());

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component1"));
	InterpComp1->SetupAttachment(GetThirdCamera());
	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component2"));
	InterpComp2->SetupAttachment(GetThirdCamera());
	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component3"));
	InterpComp3->SetupAttachment(GetThirdCamera());
	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component4"));
	InterpComp4->SetupAttachment(GetThirdCamera());
	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component5"));
	InterpComp5->SetupAttachment(GetThirdCamera());
	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component6"));
	InterpComp6->SetupAttachment(GetThirdCamera());
}

void AUnrealSuviverGameCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (ThirdCamera) {
		CameraDefaultFOV = GetThirdCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	EquipWeapon(SpawnDefaultWeapon());
	if (EquippedWeapon) {
		Inventory.Add(EquippedWeapon);
		EquippedWeapon->SetSlotIndex(0);
		EquippedWeapon->DisableCustomDepth();
		EquippedWeapon->DisableGlowMaterial();
		EquippedWeapon->SetCharacter(this);
	}

	InitializeAmmoMap();

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	InitializeInterpLoaction();
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputSystem, 0);
		}
	}
}

void AUnrealSuviverGameCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AUnrealSuviverGameCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		float TrunScaleFactor{};
		float LookUpScaleFactor{};
		if (bAiming) 
		{
			TrunScaleFactor = MouseAimingTurnRate;
			LookUpScaleFactor = MouseAimingLockUpRate;
		}
		else
		{
			TrunScaleFactor = MouseHipTurnRate;
			LookUpScaleFactor = MouseHipLockUpRate;
		}
		// add yaw and pitch input to controller
		AddControllerYawInput(TrunScaleFactor * LookAxisVector.X);
		AddControllerPitchInput(LookUpScaleFactor * LookAxisVector.Y);
	}
}

void AUnrealSuviverGameCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr) return;

	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (WeaponHasAmmo()) {
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();

		EquippedWeapon->DecreamentAmmo();
		StartFireTimer();
		StartCrosshairBulletFire();

		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol) {
			EquippedWeapon->StartSlideTimer();
		}

	}

	/*
	FHitResult FireHit;
	// 현재 소켓의 위치
	const FVector Start{ SocketTransform.GetLocation() };
	// 현재 소켓의 회전값
	const FQuat Rotation{ SocketTransform.GetRotation() };
	// 현재 소켓의 X 축의 방향 백터
	const FVector RotationAxis{ Rotation.GetAxisX() };
	// 타겟의 위치 = 시작위치 + 바라보는 방향의 끝 위치(RotationAxis * 50'000.f.f)
	const FVector End{ Start + RotationAxis * 50'000.f };

	FVector BeamEndPoint{ End };


	GetWorld()->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);
	if (FireHit.bBlockingHit) {
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f);
		DrawDebugPoint(GetWorld(), FireHit.Location, 5.0f, FColor::Red, false, 2.0f);

		BeamEndPoint = FireHit.Location;

		if (ImpactParticles) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.Location);
		}
	}
	if (BeamParticles) {
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
		if (Beam) {
			Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
		}

	}
	*/
}

bool AUnrealSuviverGameCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation, 
	FHitResult& OutHitResult)
{
	FVector OutBeamLocation;
	// Check for crosshair trace hit
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshair(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		// Tentative beam location - still need to trace from gun
		OutBeamLocation = CrosshairHitResult.Location;
	}
	else // no crosshair trace hit
	{
		// OutBeamLocation is the End location for the line trace
	}

	// Perform a second trace, this time from the gun barrel
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - WeaponTraceStart };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
	if (!OutHitResult.bBlockingHit) // object between barrel and BeamEndPoint?
	{
		OutHitResult.Location = OutBeamLocation;
		return false;
	}

	return true;

	/*
	* //Get Current Size Viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	//Get screen Space location of crosshair;
	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	//Get World Position and Direction of crosshair
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld) // was deprojection successful?
	{
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		// 타겟의 위치 = 시작위치 + 바라보는 방향의 끝 위치(RotationAxis * 50'000.f.f)
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f };

		OutBeamLocation = End;

		GetWorld()->LineTraceSingleByChannel(
			ScreenTraceHit,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);

		if (ScreenTraceHit.bBlockingHit) {
			OutBeamLocation = ScreenTraceHit.Location;
		}

		//Perform a second trace, this time from the gun barrel
		FHitResult WeaponTraceHit;
		const FVector WeaponTraceStart{ MuzzleSocketLocation };
		const FVector WeaponTraceEnd{ OutBeamLocation };
		GetWorld()->LineTraceSingleByChannel(
			WeaponTraceHit,
			WeaponTraceStart,
			WeaponTraceEnd,
			ECollisionChannel::ECC_Visibility);

		if (WeaponTraceHit.bBlockingHit) {
			OutBeamLocation = WeaponTraceHit.Location;
		}

		return true;
	}

	return false;
	* 
	*/
}

void AUnrealSuviverGameCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping
		&& CombatState != ECombatState::ECS_Stunned) {
		Aim();
	}
}

void AUnrealSuviverGameCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AUnrealSuviverGameCharacter::CameraInterpZoom(float DeltaTime)
{
	//Set Current Camera Field of view
	if (bAiming) {
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraZoomedFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraDefaultFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	GetThirdCamera()->SetFieldOfView(CameraCurrentFOV);

}

void AUnrealSuviverGameCharacter::SetLookRates()
{
	if (bAiming) {
		BaseTurnRate = AimingTurnRate;
		BaseLockUpRate = AimingLockUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLockUpRate = HipLockUpRate;
	}
}

void AUnrealSuviverGameCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f,600.f };
	FVector2D ValocityMultiplierRange{ 0.f,1.f };
	FVector Valocity{ GetVelocity() };
	Valocity.Z = 0.f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange, ValocityMultiplierRange, Valocity.Size());

	//Crosshair In Air Factor
	if (GetCharacterMovement()->IsFalling()) {
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		//Shrink the crosshairs rapidly while on the ground
		CrosshairInAirFactor =
			FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if (bAiming) 
	{
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.6f,
			DeltaTime,
			30.f
		);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.f,
			DeltaTime,
			30.f
		);
	}

	if (bFiringbullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.3f,
			DeltaTime,
			60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.f,
			DeltaTime,
			60.f);
	}

	CrosshairSpreadMultiplier = 
		0.5f + CrosshairVelocityFactor + CrosshairInAirFactor 
		- CrosshairAimFactor + CrosshairShootingFactor;

}

void AUnrealSuviverGameCharacter::StartCrosshairBulletFire()
{
	bFiringbullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShootTimer,
		this,
		&AUnrealSuviverGameCharacter::FinishcrosshairBulletFire,
		ShootTimeDuration);

}

void AUnrealSuviverGameCharacter::FinishcrosshairBulletFire()
{
	bFiringbullet = false;
}

void AUnrealSuviverGameCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();

}

void AUnrealSuviverGameCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AUnrealSuviverGameCharacter::StartFireTimer()
{
	if (EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_FireTimerInProgress;

	GetWorldTimerManager().SetTimer(
		AutoFireTimer,
		this,
		&AUnrealSuviverGameCharacter::AutoFireReset,
		EquippedWeapon->GetAutoFireRate());
}

void AUnrealSuviverGameCharacter::AutoFireReset()
{
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;
	if (EquippedWeapon == nullptr) return;
	if (WeaponHasAmmo()) {
		if (bFireButtonPressed && EquippedWeapon->GetAutomatic()) {
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
		//Reload Weapon;
	}
}

bool AUnrealSuviverGameCharacter::TraceUnderCrosshair(FHitResult& OutHitResult, FVector& OuthitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	//Get screen Space location of crosshair;
	FVector2D CrosshairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld) {
		//Trace from Crosshair world location outward

		FVector Start{CrosshairWorldPosition};
		FVector End{ Start + CrosshairWorldDirection * 50'000.f };
		OuthitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit) 
		{
			OuthitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void AUnrealSuviverGameCharacter::TraceForItems()
{
	if (bShuouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshair(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit) {
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if (TraceHitWeapon) {
				if (HighlightedSlot == -1) {
					HighlightInventorySlot();
				}
			}
			else
			{
				if (HighlightedSlot != -1) 
				{
					UnHighlightInventorySlot();
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}
			if (TraceHitItem && TraceHitItem->GetPickupWidget()) {
				//Show Item's PickUpWidget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();
				if (Inventory.Num() >= INVENTORY_CAPACITY) {
					//Inventory is Full
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetCharacterInventoryFull(false);
				}

			}
			//We hit an Item last Frame
			if (TraceHitItemLastFrame) 
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}

			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if(TraceHitItemLastFrame)
	{
		// No longer Overlapping any items
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

AWeapon* AUnrealSuviverGameCharacter::SpawnDefaultWeapon()
{
	//Check the TSubclassOf variable
	if (DefaultWeaponClass) {
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}
void AUnrealSuviverGameCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	//Check the TSubclassOf variable
	if (WeaponToEquip) {

		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket) {
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		if (EquippedWeapon == nullptr) {
			// -1 == no EquipWeapon yet, No need to reverse the icon animation
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(),WeaponToEquip->GetSlotIndex());
		}
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AUnrealSuviverGameCharacter::DropWeapon() 
{
	if (EquippedWeapon) {
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AUnrealSuviverGameCharacter::SelectButtonPressed()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (TraceHitItem) 
	{
		TraceHitItem->StartItemCurve(this,true);
		TraceHitItem = nullptr;
	}
}

void AUnrealSuviverGameCharacter::SelectButtonReleased()
{
}

void AUnrealSuviverGameCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if (WeaponToSwap == nullptr) return;

	if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex()) 
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}

	DropWeapon();
	EquipWeapon(WeaponToSwap,true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AUnrealSuviverGameCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AUnrealSuviverGameCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) return false;
	return EquippedWeapon->GetAmmo() > 0;
}

void AUnrealSuviverGameCharacter::PlayFireSound()
{
	//Play FireSound
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AUnrealSuviverGameCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket =
		EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(
			EquippedWeapon->GetItemMesh());

		if (EquippedWeapon->GetMuzzleFlash())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}

		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(), BeamHitResult);
		if (bBeamEnd)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), BeamHitResult.GetActor() != nullptr ? "Hit" : "null");
			// Does hit Actor implement BulletHitInterface?
			if (BeamHitResult.GetActor())
			{
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.GetActor());
				if (BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult,this,GetController());
				}

				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.GetActor());
				if (HitEnemy) 
				{
					int32 Damage{};
					if (BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBoneName()) 
					{
						//Head Shot
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location,true);
					}
					else 
					{
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
					}
				}
			}
			else
			{
				// Spawn default particles
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						BeamHitResult.Location);
				}
			}

			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				BeamParticles,
				SocketTransform);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
			}
		}
	}
}

void AUnrealSuviverGameCharacter::PlayGunFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage) {
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AUnrealSuviverGameCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AUnrealSuviverGameCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon == nullptr) 	return;
	//Check ammo Type
	if (CarryingAmmo() && !EquippedWeapon->ClipIsFull()) {
		// TODo enum for Weapon Type
		//TODO : swith on EquipmentWeapon->WeaponType;
		if (bAiming) { StopAiming(); }

		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage) {
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(
				EquippedWeapon->GetReloadMontageSection());
		}
	}

}

void AUnrealSuviverGameCharacter::FinishReloading()
{
	if (CombatState == ECombatState::ECS_Stunned) return;
	//Update AmmoMap
	CombatState = ECombatState::ECS_Unoccupied;

	if (bAimingButtonPressed) { Aim(); }

	if (EquippedWeapon == nullptr) return;
	//Update Ammo Map

	const auto AmmoType { EquippedWeapon->GetAmmoType() };
	if (AmmoMap.Contains(AmmoType)) {
		//캐릭터가 가지고 있는 Ammo 갯수
		int32 CarriedAmmo = AmmoMap[AmmoType];
		const int32 MagEmptySpace = 
			EquippedWeapon->GetMagazineCapacity()
			- EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo) 
		{
			// Reload the magazine
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			//fill the magazine
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AUnrealSuviverGameCharacter::FinishEquipping()
{
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed) {
		Aim();
	}
}

bool AUnrealSuviverGameCharacter::CarryingAmmo()
{
	if(EquippedWeapon == nullptr) 	return false;
	auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType)) {
		return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AUnrealSuviverGameCharacter::CrouchingButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling()) {
		bCrouching = !bCrouching;
	}
	if (bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AUnrealSuviverGameCharacter::Jump()
{
	if (bCrouching) {
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		ACharacter::Jump();
	}
}

void AUnrealSuviverGameCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight{};
	if(bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}

	const float InterpHalfHeight{ FMath::FInterpTo(
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
		TargetCapsuleHalfHeight,
		DeltaTime,
		20.f) };

	//Nefative Value if Crouching , Positive Value if Standing
	const float DeltaCapsuleHalfHeight{ InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };
	FVector MeshOffset{ 0.f,0.f,-DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);

}

void AUnrealSuviverGameCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AUnrealSuviverGameCharacter::StopAiming()
{
	bAiming = false;
	if (!bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AUnrealSuviverGameCharacter::PickupAmmo(AAmmo* Ammo)
{
	//check to sse if AmmoMap constains Ammo's Type

	if (AmmoMap.Contains(Ammo->GetAmmoType())) {
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}
	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType()) {
		//Check to see
		if (EquippedWeapon->GetAmmo() == 0) {
			ReloadWeapon();
		}
	}

	Ammo->Destroy();
}

void AUnrealSuviverGameCharacter::InitializeInterpLoaction()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp,0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1,0 };
	InterpLocations.Add(InterpLoc1);
	FInterpLocation InterpLoc2{ InterpComp2,0 };
	InterpLocations.Add(InterpLoc2);
	FInterpLocation InterpLoc3{ InterpComp3,0 };
	InterpLocations.Add(InterpLoc3);
	FInterpLocation InterpLoc4{ InterpComp4,0 };
	InterpLocations.Add(InterpLoc4);
	FInterpLocation InterpLoc5{ InterpComp5,0 };
	InterpLocations.Add(InterpLoc5);
	FInterpLocation InterpLoc6{ InterpComp6,0 };
	InterpLocations.Add(InterpLoc6);
}

void AUnrealSuviverGameCharacter::Key_F_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 0) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 0);
}

void AUnrealSuviverGameCharacter::Key_1_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 1) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 1);
}

void AUnrealSuviverGameCharacter::Key_2_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 2) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 2);
}

void AUnrealSuviverGameCharacter::Key_3_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 3) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 3);
}

void AUnrealSuviverGameCharacter::Key_4_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 4) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 4);
}

void AUnrealSuviverGameCharacter::Key_5_Pressed()
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->GetSlotIndex() == 5) return;

	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 5);
}

void AUnrealSuviverGameCharacter::ExchangeInventoryItem(int32 CurrentItemIndex, int32 NewItemIndex)
{
	const bool bCanExchangeItems = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num())
		&& ((CombatState == ECombatState::ECS_Unoccupied) || (CombatState == ECombatState::ECS_Equipping));

	if (bCanExchangeItems)
	{
		if (bAiming) StopAiming();
		auto OldEquipWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		if (NewWeapon)
		{
			EquipWeapon(NewWeapon);
			OldEquipWeapon->SetItemState(EItemState::EIS_Pickedup);
			NewWeapon->SetItemState(EItemState::EIS_Equipped);
			CombatState = ECombatState::ECS_Equipping;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance && EquipMontage)
			{
				AnimInstance->Montage_Play(EquipMontage, 1.0f);
				AnimInstance->Montage_JumpToSection(FName("Equip"));
			}
			NewWeapon->PlayEquipSound(true);
		}
	}
}

int32 AUnrealSuviverGameCharacter::GetEmptyInventorySlot()
{
	for (int32 i = 0; i < Inventory.Num(); i++) {
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if (Inventory.Num() < INVENTORY_CAPACITY) {
		return Inventory.Num();
	}
	return -1;
}

void AUnrealSuviverGameCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot{ GetEmptyInventorySlot() };
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

EPhysicalSurface AUnrealSuviverGameCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start{ GetActorLocation() };
	const FVector End{ Start + FVector(0.f,0.f,-400.f) };
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult, 
		Start, 
		End,
		ECollisionChannel::ECC_Visibility,
		QueryParams);

	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AUnrealSuviverGameCharacter::Stun()
{
	if (Health <= 0.f) return;

	CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage) {
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

void AUnrealSuviverGameCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed) {
		Aim();
	}
}

void AUnrealSuviverGameCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

int32 AUnrealSuviverGameCharacter::GetInterpLoactionIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for (int32 i = 0; i < InterpLocations.Num(); i++)
	{
		if (InterpLocations[i].ItemCount < LowestCount) {
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}

	return LowestIndex;
}

void AUnrealSuviverGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);
	SetLookRates();
	CalculateCrosshairSpread(DeltaTime);
	TraceForItems();
	InterpCapsuleHalfHeight(DeltaTime);
}

void AUnrealSuviverGameCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AUnrealSuviverGameCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}

float AUnrealSuviverGameCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AUnrealSuviverGameCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0) 
	{
		OverlappedItemCount = 0;
		bShuouldTraceForItems = false;
	}
	else 
	{
		OverlappedItemCount += Amount;
		bShuouldTraceForItems = true;
	}
}

//FVector AUnrealSuviverGameCharacter::GetCameraInterpLocation()
//{
//	const FVector CameraWorldLocation{ ThirdCamera->GetComponentLocation() };
//	const FVector CameraForward{ ThirdCamera->GetForwardVector() };
//
//	//Desired = CameraWorldLocation + Forward * A + Up * b;
//	return CameraWorldLocation + CameraForward * CameraInterpDistance +
//		FVector(0.f, 0.f, CameraInterpElevation);
//}

void AUnrealSuviverGameCharacter::GetPickupItem(AItem* Item)
{
	Item->PlayEquipSound();

	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon) 
	{
		if (Inventory.Num() < INVENTORY_CAPACITY) {
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_Pickedup);
		}
		else
		{
			SwapWeapon(Weapon);
		}
	}
	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo) 
	{
		PickupAmmo(Ammo);
	}
}

int32 AUnrealSuviverGameCharacter::GetAmmoCount() const
{
	if (EquippedWeapon != nullptr)
	{
		return EquippedWeapon->GetAmmo();
	}
	return 10;
}

EAmmoType AUnrealSuviverGameCharacter::GetEquippedWeaponAmmoType()
{
	if (EquippedWeapon != nullptr)
		return EquippedWeapon->GetAmmoType();
	return EAmmoType::EAT_MAX;
}

void AUnrealSuviverGameCharacter::GrapClip()
{
	if (EquippedWeapon == nullptr) return;
	if (HandSceneComponent == nullptr) return;

	int32 ClipBoneIndex{ 
		EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };

	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules{ EAttachmentRule::KeepRelative,true };
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules,FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);
	EquippedWeapon->SetMovingClip(true);
}

void AUnrealSuviverGameCharacter::ReleaseClip()
{
	if (EquippedWeapon == nullptr) return;

	EquippedWeapon->SetMovingClip(false);
}

FInterpLocation AUnrealSuviverGameCharacter::GetInterpLoaction(int32 Index)
{
	if (Index <= InterpLocations.Num()) {
		return InterpLocations[Index];
	}
	return FInterpLocation();
}


void AUnrealSuviverGameCharacter::IncreamentInterpLocItemCount(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount > 1) return;
	if (InterpLocations.Num() >= Index) {
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AUnrealSuviverGameCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(
		PickupSoundTimer, this,
		&AUnrealSuviverGameCharacter::ResetPickupSoundTimer, PickupSoundResetTime);

}

void AUnrealSuviverGameCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(
		EquipSoundTimer, this,
		&AUnrealSuviverGameCharacter::ResetEquipSoundTimer, EquipSoundResetTime);
}

float AUnrealSuviverGameCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f) {
		Health = 0.f;
		Die();
		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController) {
			EnemyController->GetBlackboardComponent()->SetValueAsBool(
				FName(TEXT("CheracterDeath")),
				true);
		}
	}
	else {
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AUnrealSuviverGameCharacter::Die() {
	bDead = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage) {
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void AUnrealSuviverGameCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC) {
		DisableInput(PC);
	}
}

// Input
void AUnrealSuviverGameCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUnrealSuviverGameCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUnrealSuviverGameCharacter::Look);

		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AUnrealSuviverGameCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AUnrealSuviverGameCharacter::FireButtonReleased);
	
		EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::AimingButtonPressed);
		EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Completed, this, &AUnrealSuviverGameCharacter::AimingButtonReleased);

		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::SelectButtonPressed);
		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &AUnrealSuviverGameCharacter::SelectButtonReleased);

		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(CrouchingAction, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::CrouchingButtonPressed);
		EnhancedInputComponent->BindAction(Key_F_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_F_Pressed);
		EnhancedInputComponent->BindAction(Key_1_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_1_Pressed);
		EnhancedInputComponent->BindAction(Key_2_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_2_Pressed);
		EnhancedInputComponent->BindAction(Key_3_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_3_Pressed);
		EnhancedInputComponent->BindAction(Key_4_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_4_Pressed);
		EnhancedInputComponent->BindAction(Key_5_Action, ETriggerEvent::Started, this, &AUnrealSuviverGameCharacter::Key_5_Pressed);
	}
}
