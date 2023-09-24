// Fill out your copyright notice in the Description page of Project Settings.


#include "SuviverCharacterAnimInstance.h"
#include "UnrealSuviverGameCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"
#include "WeaponType.h"
USuviverCharacterAnimInstance::USuviverCharacterAnimInstance() :
	Speed(0.f),
	bIsInAir(false),
	bIsAccelerating(false),
	MovementOffsetYaw(0.f),
	LastMovementOffsetYaw(0.f),
	bAiming(false),
	CharacterRotation(FRotator(0.f)),
	CharacterRotationLastFrame(FRotator(0.f)),
	TIPCharacterYaw(0.f),
	TIPCharacterYawLastFrame(0.f),
	RootYawOffset(0.f),
	YawDelta(0.f),
	bReloading(false),
	OffsetState(EOffsetState::EOS_Hip),
	RecoilWeight(1.f),
	bTurningInPlace(false),
	EquippedWeaponType(EWeaponType::EWT_MAX),
	bShouldUseFABRIK(false)
{

}

void USuviverCharacterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (SuviverCharacter == nullptr) {
		SuviverCharacter = Cast<AUnrealSuviverGameCharacter>(TryGetPawnOwner());
	}
	if (SuviverCharacter)
	{
		bCrouching = SuviverCharacter->GetCrouching();
		bReloading = SuviverCharacter->GetCombatState() == ECombatState::ECS_Reloading;
		bEquipping = SuviverCharacter->GetCombatState() == ECombatState::ECS_Equipping;

		bShouldUseFABRIK = SuviverCharacter->GetCombatState() == ECombatState::ECS_Unoccupied ||
			SuviverCharacter->GetCombatState() == ECombatState::ECS_FireTimerInProgress;
		//Get the Speed of the character from velocity
		FVector Velocity{ SuviverCharacter->GetVelocity() };
		Velocity.Z = 0;
		Speed = Velocity.Size();

		bIsInAir = SuviverCharacter->GetCharacterMovement()->IsFalling();

		if (SuviverCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f) {
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		FRotator AimRotation = SuviverCharacter->GetBaseAimRotation();
		FRotator MovementRotation =
			UKismetMathLibrary::MakeRotFromX(SuviverCharacter->GetVelocity());

		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(
			MovementRotation,
			AimRotation).Yaw;

		if (SuviverCharacter->GetVelocity().Size() > 0.f) {
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = SuviverCharacter->GetAiming();
		if (bReloading) {
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if(bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (SuviverCharacter->GetAiming()) {
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
		//Check if SuviverCharacter has a valied Weapon;
		if (SuviverCharacter->GetEquippedWeapon()) {
			EquippedWeaponType = SuviverCharacter->GetEquippedWeapon()->GetWeaponType();
		}

	}
	TurnInPlace();
	Lean(DeltaTime);
}



void USuviverCharacterAnimInstance::NativeInitializeAnimation()
{
	SuviverCharacter = Cast<AUnrealSuviverGameCharacter>(TryGetPawnOwner());
}

void USuviverCharacterAnimInstance::CharacterStartReload()
{
	if (SuviverCharacter) {
		SuviverCharacter->FinishReloading();
	}
}

void USuviverCharacterAnimInstance::CharacterGrabClip()
{
	if (SuviverCharacter) {
		SuviverCharacter->GrapClip();
	}
}

void USuviverCharacterAnimInstance::CharacterReleaseClip()
{
	if (SuviverCharacter) {
		SuviverCharacter->ReleaseClip();
	}
}
void USuviverCharacterAnimInstance::CharacterFinishEquip()
{
	if (SuviverCharacter) {
		SuviverCharacter->FinishEquipping();
	}
}

void USuviverCharacterAnimInstance::CharacterEndStun()
{
	if (SuviverCharacter) {
		SuviverCharacter->EndStun();
	}
}

void USuviverCharacterAnimInstance::FinishCharacterDeath()
{
	if (SuviverCharacter) {
		SuviverCharacter->FinishDeath();
	}
}
void USuviverCharacterAnimInstance::TurnInPlace()
{
	if (SuviverCharacter == nullptr) return;

	Pitch = SuviverCharacter->GetBaseAimRotation().Pitch;

	if (Speed > 0 || bIsInAir) {
		RootYawOffset = 0.f;
		TIPCharacterYaw = SuviverCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurveLastFrame = 0.f;
		RotationCurve = 0.f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = SuviverCharacter->GetActorRotation().Yaw;
		const float TIPTIPYawDelta{ TIPCharacterYaw - TIPCharacterYawLastFrame };

		/// <summary>
		/// Root Yaw Offset, Updated And Clamped to [-180,180]
		/// </summary>
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPTIPYawDelta);

		
		const float Turning{ GetCurveValue(TEXT("Turning")) };
		if (Turning > 0) {
			bTurningInPlace = true;

			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation{ RotationCurve - RotationCurveLastFrame };

			// RootYawOffset > 0 => Turning Left , RootYawOffset < 0 => Turning Right
			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float ABSRootYawOffset{ FMath::Abs(RootYawOffset) };
			if (ABSRootYawOffset > 90.f) {
				const float YawExcess{ ABSRootYawOffset - 90.f };
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
			
			//if (GEngine) GEngine->AddOnScreenDebugMessage(2, -1, FColor::Red, FString::Printf(TEXT("RootYawOffset : %f"), RootYawOffset));
		}
		else 
		{
			bTurningInPlace = false;
		}
	}

	// Set The Recoil Weight
	if (bTurningInPlace)
	{
		if (bReloading || bEquipping) {
			RecoilWeight = 1.f;
		}
		else
		{
			RecoilWeight = 0.f;
		}

	}
	else
	{
		if (bCrouching) {
			if (bReloading || bEquipping) {
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if (bAiming || bReloading || bEquipping) {
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.5f;
			}
		}
	}

}


void USuviverCharacterAnimInstance::Lean(float DeltaTime) {
	if (SuviverCharacter == nullptr) return;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SuviverCharacter->GetActorRotation();

	const FRotator Delta{ UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation,CharacterRotationLastFrame) };

	const float Target{ (float)Delta.Yaw / DeltaTime };

	const float Interp{ FMath::FInterpTo(YawDelta,Target,DeltaTime,6.f) };

	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

}