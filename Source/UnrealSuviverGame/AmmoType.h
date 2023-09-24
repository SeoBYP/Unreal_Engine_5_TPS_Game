#pragma once



UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(Displayname = "9mm"),
	EAT_AR UMETA(Displayname = "Assault Rifle"),

	EAT_MAX UMETA(Displayname = "DefaultMAX")
};
