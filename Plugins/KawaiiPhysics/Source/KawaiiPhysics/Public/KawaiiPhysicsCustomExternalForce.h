// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once
#include "AnimNode_KawaiiPhysics.h"
#include "KawaiiPhysicsCustomExternalForce.generated.h"


UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class KAWAIIPHYSICS_API UKawaiiPhysics_CustomExternalForce : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayPriority=1), Category="KawaiiPhysics|CustomExternalForce")
	bool bIsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayPriority=1), Category="KawaiiPhysics|CustomExternalForce")
	bool bDrawDebug = false;

public:
	// 重要 / IMPORTANT (Thread-safety):
	// PreApply / Apply は FAnimNode_KawaiiPhysics::EvaluateSkeletalControl_AnyThread から呼び出されるため、
	// アニメーション・ワーカースレッド上で実行されうる。実装（特に Blueprint）では GameThread 専用 API
	// （アクター/コンポーネントの破壊・スポーン、ワールドのレイキャスト、UObject ライフサイクル操作など）に
	// 触れないこと。必要な GameThread 値は事前に取得してメンバへキャッシュし、ここでは読み取りのみ行う。
	// PreApply / Apply are invoked from EvaluateSkeletalControl_AnyThread and may therefore run on the
	// animation worker thread. Implementations (especially Blueprint) must NOT touch game-thread-only APIs
	// (spawning/destroying actors or components, world raycasts, UObject lifecycle, etc.). Cache any required
	// game-thread values beforehand and only read them here.
	UFUNCTION(BlueprintNativeEvent)
	void PreApply(UPARAM(ref) FAnimNode_KawaiiPhysics& Node,
	              const USkeletalMeshComponent* SkelComp);

	virtual void PreApply_Implementation(
		UPARAM(ref) FAnimNode_KawaiiPhysics& Node, const USkeletalMeshComponent* SkelComp)PURE_VIRTUAL(,);

	UFUNCTION(BlueprintNativeEvent)
	void Apply(UPARAM(ref) FAnimNode_KawaiiPhysics& Node, int32 ModifyBoneIndex,
	           const USkeletalMeshComponent* SkelComp, const FTransform& BoneTransform);

	virtual void Apply_Implementation(
		UPARAM(ref) FAnimNode_KawaiiPhysics& Node, int32 ModifyBoneIndex, const USkeletalMeshComponent* SkelComp,
		const FTransform& BoneTransform)
	{
	}

	UFUNCTION(BlueprintCallable, Category="KawaiiPhysics|CustomExternalForce")
	virtual bool IsDebugEnabled()
	{
#if ENABLE_ANIM_DEBUG
		if (CVarAnimNodeKawaiiPhysicsDebug.GetValueOnAnyThread())
		{
			return bDrawDebug;
		}
#endif

		return false;
	}
};
