﻿#pragma once
#include "AnimNode_KawaiiPhysics.h"
#include "Curves/CurveVector.h"

#include "KawaiiPhysicsExternalForce.generated.h"


USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayPriority=1))
	bool bIsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayPriority=1))
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, meta=(DisplayPriority=1))
	TArray<FBoneReference> ApplyBoneFilter;

	UPROPERTY(EditAnywhere, meta=(DisplayPriority=1))
	TArray<FBoneReference> IgnoreBoneFilter;

public:
	virtual ~FKawaiiPhysics_ExternalForce() = default;

	virtual void PreApply(FAnimNode_KawaiiPhysics& Node, const USkeletalMeshComponent* SkelComp)
	{
	};

	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   const FComponentSpacePoseContext& PoseContext)
	{
	};


	virtual bool IsDebugEnabled()
	{
		if (const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimNode.KawaiiPhysics.Debug")))
		{
			return CVar->GetBool() && bDrawDebug;
		}
		return false;
	}

#if WITH_EDITOR
	virtual void AnimDrawDebugForEditMode(const FKawaiiPhysicsModifyBone& ModifyBone,
	                                      const FAnimNode_KawaiiPhysics& Node, FPrimitiveDrawInterface* PDI)
	{
	}
#endif

protected:
	bool CanApply(const FKawaiiPhysicsModifyBone& Bone) const
	{
		if (!ApplyBoneFilter.IsEmpty() && !ApplyBoneFilter.Contains(Bone.BoneRef))
		{
			return false;
		}

		if (!IgnoreBoneFilter.IsEmpty() && IgnoreBoneFilter.Contains(Bone.BoneRef))
		{
			return false;
		}

		return true;
	}
};

USTRUCT(BlueprintType, DisplayName = "Simple")
struct KAWAIIPHYSICS_API FKawaiiPhysics_ExternalForce_Simple : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ForceDir = FVector::Zero();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForceScale = 1.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowLength = 5.0f;
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowSize = 1.0f;
	UPROPERTY(EditDefaultsOnly)
	FVector DebugArrowOffset = FVector::Zero();
#endif

private:
	UPROPERTY()
	FVector Force = FVector::Zero();;

public:
	virtual void PreApply(FAnimNode_KawaiiPhysics& Node, const USkeletalMeshComponent* SkelComp) override;
	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   const FComponentSpacePoseContext& PoseContext) override;

#if WITH_EDITOR
	virtual void AnimDrawDebugForEditMode(const FKawaiiPhysicsModifyBone& ModifyBone,
	                                      const FAnimNode_KawaiiPhysics& Node, FPrimitiveDrawInterface* PDI) override;
#endif
};

USTRUCT(BlueprintType, DisplayName = "Gravity")
struct KAWAIIPHYSICS_API FKawaiiPhysics_ExternalForce_Gravity : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GravityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseCharacterGravityDirection = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseCharacterGravityScale = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (editcondition = "bUseOverrideGravityDirection"))
	FVector OverrideGravityDirection = FVector::Zero();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bUseOverrideGravityDirection = false;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowLength = 5.0f;
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowSize = 1.0f;
	UPROPERTY(EditDefaultsOnly)
	FVector DebugArrowOffset = FVector::Zero();
#endif

private:
	UPROPERTY()
	FVector Gravity = FVector::Zero();;

public:
	virtual void PreApply(FAnimNode_KawaiiPhysics& Node, const USkeletalMeshComponent* SkelComp) override;
	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   const FComponentSpacePoseContext& PoseContext) override;

#if WITH_EDITOR
	virtual void AnimDrawDebugForEditMode(const FKawaiiPhysicsModifyBone& ModifyBone,
	                                      const FAnimNode_KawaiiPhysics& Node, FPrimitiveDrawInterface* PDI) override;
#endif
};

USTRUCT(BlueprintType, DisplayName = "Curve")
struct KAWAIIPHYSICS_API FKawaiiPhysics_ExternalForce_Curve : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinHiddenByDefault, XAxisName="Time", YAxisName="Force"))
	FRuntimeVectorCurve ForceCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForceScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinHiddenByDefault))
	float TimeScale = 1.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowLength = 5.0f;
	UPROPERTY(EditDefaultsOnly)
	float DebugArrowSize = 1.0f;
	UPROPERTY(EditDefaultsOnly)
	FVector DebugArrowOffset = FVector::Zero();
#endif

	UPROPERTY()
	float Time;

private:
	UPROPERTY()
	FVector Force = FVector::Zero();

public:
	virtual void PreApply(FAnimNode_KawaiiPhysics& Node, const USkeletalMeshComponent* SkelComp) override;
	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   const FComponentSpacePoseContext& PoseContext) override;

#if WITH_EDITOR
	virtual void AnimDrawDebugForEditMode(const FKawaiiPhysicsModifyBone& ModifyBone,
	                                      const FAnimNode_KawaiiPhysics& Node, FPrimitiveDrawInterface* PDI) override;
#endif
};