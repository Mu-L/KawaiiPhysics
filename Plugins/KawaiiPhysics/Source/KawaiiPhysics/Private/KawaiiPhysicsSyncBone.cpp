// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsSyncBone.h"
#include "AnimNode_KawaiiPhysics.h"

#if WITH_EDITOR
#include "Engine/Engine.h" 
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneManagement.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(KawaiiPhysicsSyncBone)

void FKawaiiPhysicsSyncTarget::UpdateScaleByLengthRate(const FRichCurve* ScaleCurveByBoneLengthRate)
{
	if (!ScaleCurveByBoneLengthRate)
	{
		return;
	}

	ScaleByLengthRateCurve = ScaleCurveByBoneLengthRate->Eval(LengthRateFromSyncTargetRoot);
}

void FKawaiiPhysicsSyncTarget::Apply(TArray<FKawaiiPhysicsModifyBone>& ModifyBones, const FVector& Translation)
{
	if (ModifyBoneIndex < 0 || !ModifyBones.IsValidIndex(ModifyBoneIndex))
	{
		return;
	}

	FKawaiiPhysicsModifyBone& Bone = ModifyBones[ModifyBoneIndex];
	if (Bone.bSkipSimulate)
	{
		return;
	}

	// // Apply Alpha per target
	const FVector ScaledTranslation = Translation * ScaleByLengthRateCurve;

#if WITH_EDITORONLY_DATA
	TranslationBySyncBone = ScaledTranslation;
#endif

	if (Bone.ParentIndex >= 0)
	{
		const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];
		const FVector NewPoseLocation = Bone.PoseLocation + ScaledTranslation;
		if (ParentBone.bInterBoneDummy)
		{
			// 解決したい問題: childの長さ拘束を「すぐ上の親」基準で行うと、その親が分割dummy(コリジョン用に親子間へ
			// 挿入した中間ボーン)の場合に伸縮バグが出る。dummyの位置はこの時点では古いまま(後段で再計算される)で、
			// それを基準にすると長さが合わないため。
			// 対策: dummyを飛ばし、その1つ上＝本来の親ボーン(=祖父)を基準に拘束する。dummyは祖父とchildをInterBoneAlphaで
			// 内分するので |child-dummy| = (1-alpha)*|祖父-child| → 距離 BoneLength/(1-alpha) で拘束すれば古いdummyを使わず長さを保てる。
			// Problem: constraining the child against its immediate parent stretches the bone when that parent is a
			// subdivision dummy (inserted between parent and child for collision) whose position is still old here
			// (recomputed later). Fix: skip the dummy and constrain against the bone above it (the real parent =
			// grandparent) at distance BoneLength/(1-alpha); the dummy splits grandparent..child by InterBoneAlpha.
			const float OneMinusAlpha = 1.0f - ParentBone.InterBoneAlpha;
			const int32 GrandParentIndex = ParentBone.InterBoneRealParentIndex;
			if (OneMinusAlpha > KINDA_SMALL_NUMBER && ModifyBones.IsValidIndex(GrandParentIndex))
			{
				const FKawaiiPhysicsModifyBone& GrandParent = ModifyBones[GrandParentIndex];
				const float TargetLength = Bone.BoneLength / OneMinusAlpha;
				Bone.PoseLocation =
					(NewPoseLocation - GrandParent.PoseLocation).GetSafeNormal() * TargetLength + GrandParent.PoseLocation;
			}
			else
			{
				// 退化(alpha≈1)・祖父無効時は並進のみ（長さは後段の再補間に委ねる）
				// Degenerate (alpha≈1) or invalid grandparent: translate only (length handled by re-interpolation).
				Bone.PoseLocation = NewPoseLocation;
			}
		}
		else
		{
			// Maintain bone length relative to parent
			Bone.PoseLocation = (NewPoseLocation - ParentBone.PoseLocation).GetSafeNormal() * Bone.BoneLength +
				ParentBone.PoseLocation;
		}
	}
	else
	{
		Bone.PoseLocation += ScaledTranslation;
	}
}

#if WITH_EDITOR
void FKawaiiPhysicsSyncTarget::DebugDraw(FPrimitiveDrawInterface* PDI, const FAnimNode_KawaiiPhysics* Node) const
{
	auto DrawForceArrow = [&](const FVector& Force, const FVector& Location)
	{
		const FRotator Rotation = FRotationMatrix::MakeFromX(Force.GetSafeNormal()).Rotator();
		const FMatrix TransformMatrix = FRotationMatrix(Rotation) * FTranslationMatrix(Location);
		DrawDirectionalArrow(PDI, TransformMatrix, FLinearColor::Green, Force.Length(), 2.0f, SDPG_Foreground);
	};

	if (ModifyBoneIndex >= 0 && Node->ModifyBones.IsValidIndex(ModifyBoneIndex))
	{
		// Target Bone Location
		FVector TargetBoneLocation = Node->ModifyBones[ModifyBoneIndex].Location;
		if (Node->SimulationSpace == EKawaiiPhysicsSimulationSpace::BaseBoneSpace)
		{
			const FTransform& BaseBoneSpace2ComponentSpace = Node->GetBaseBoneSpace2ComponentSpace();
			TargetBoneLocation = BaseBoneSpace2ComponentSpace.TransformPosition(TargetBoneLocation);
		}
		DrawSphere(PDI, TargetBoneLocation, FRotator::ZeroRotator,
		           FVector(1.0f), 12, 6,
		           GEngine->ConstraintLimitMaterialY->GetRenderProxy(), SDPG_World);

		// Force by SyncBone
		DrawForceArrow(TranslationBySyncBone, TargetBoneLocation);
	}
}
#endif
