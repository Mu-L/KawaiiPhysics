// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsSyncBone.h"
#include "AnimNode_KawaiiPhysics.h"

#if WITH_EDITOR
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneManagement.h" // DrawDirectionalArrow, DrawSphere
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
		return;;
	}

	FKawaiiPhysicsModifyBone& Bone = ModifyBones[ModifyBoneIndex];
	if (Bone.bSkipSimulate)
	{
		return;;
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
			// 即時親が inter-bone dummy の場合、その Pose は Apply ループ後の UpdateSubdivisionDummyPoseAfterSyncBones()
			// まで stale。stale dummy を基準に長さ拘束すると剛体/非剛体とも歪む。
			// dummy は実祖父 gp と本ボーン child を InterBoneAlpha で内分する点なので |child-dummy| = (1-alpha)*|child-gp|。
			// よって実祖父 gp を基準に距離 BoneLength/(1-alpha) で拘束すれば、stale を使わず長さを正しく保存できる
			// （root+child 同一 delta の剛体移動でも、attenuation/curve で delta が異なる非剛体でも segment 長を維持）。
			// The immediate parent is an inter-bone dummy whose pose is stale until the post-apply re-interpolation.
			// The dummy divides gp(real grandparent)..child by InterBoneAlpha, so |child-dummy| = (1-alpha)*|child-gp|.
			// Constrain against gp at distance BoneLength/(1-alpha): preserves segment length without the stale dummy,
			// for both rigid (root+child same delta) and non-rigid (attenuation/curve) sync.
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
