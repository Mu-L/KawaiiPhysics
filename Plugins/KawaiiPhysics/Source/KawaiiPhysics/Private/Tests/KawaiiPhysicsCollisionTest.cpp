// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KawaiiPhysicsTestHarness.h"

// コリジョン押し出しの正しさ（解析的基準値）。
// Collision push-out correctness against analytically derived baselines.
// 各形状: ボーン(半径r)が形状に食い込んだとき、表面+r へ正しく押し出されることを検証。
// For each shape: a bone (radius r) penetrating the shape must be pushed exactly to surface + r.

namespace
{
	// ボーン1個を生成（位置・前フレーム位置・コリジョン半径） / Make a single bone.
	FKawaiiPhysicsModifyBone MakeBone(const FVector& Location, float Radius,
	                                  const FVector& PrevLocation)
	{
		FKawaiiPhysicsModifyBone Bone;
		Bone.Location = Location;
		Bone.PrevLocation = PrevLocation;
		Bone.PoseLocation = Location;
		Bone.PhysicsSettings.Radius = Radius;
		return Bone;
	}

	constexpr float GCollisionTol = 0.01f; // 0.1mm スケール / sub-millimeter tolerance
}

// ---------------------------------------------------------------------------
//  Sphere (Outer)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsSphereOuterTest,
                                 "KawaiiPhysics.Collision.SphereOuterPushOut",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsSphereOuterTest::RunTest(const FString& Parameters)
{
	FKawaiiPhysicsTestAccessor A;

	// 中心 O, 半径 10。ボーン半径 3 が (5,0,0) に食い込み → 中心から 13 の表面 (13,0,0) へ。
	FKawaiiPhysicsModifyBone Bone = MakeBone(FVector(5, 0, 0), 3.0f, FVector(5, 0, 0));

	TArray<FSphericalLimit> Limits;
	FSphericalLimit Sphere;
	Sphere.Location = FVector::ZeroVector;
	Sphere.Radius = 10.0f;
	Sphere.LimitType = ESphericalLimitType::Outer;
	Sphere.bEnable = true;
	Limits.Add(Sphere);

	A.CallSphereCollision(Bone, Limits);

	const FVector Expected(13, 0, 0);
	TestTrue(FString::Printf(TEXT("Sphere push-out: got %s expected %s"),
	                         *Bone.Location.ToString(), *Expected.ToString()),
	         Bone.Location.Equals(Expected, GCollisionTol));

	// 表面の外側にあるボーンは動かさない / a bone already outside must not move.
	FKawaiiPhysicsModifyBone Outside = MakeBone(FVector(20, 0, 0), 3.0f, FVector(20, 0, 0));
	A.CallSphereCollision(Outside, Limits);
	TestTrue(TEXT("Sphere: bone outside is untouched"),
	         Outside.Location.Equals(FVector(20, 0, 0), GCollisionTol));

	// Inner タイプ: 内側に閉じ込める。inner limit = max(R - boneR, 0) = 7。距離 10 は外なので 7 へ引き戻し。
	FKawaiiPhysicsModifyBone Inner = MakeBone(FVector(10, 0, 0), 3.0f, FVector(10, 0, 0));
	TArray<FSphericalLimit> InnerLimits;
	FSphericalLimit InnerSphere;
	InnerSphere.Location = FVector::ZeroVector;
	InnerSphere.Radius = 10.0f;
	InnerSphere.LimitType = ESphericalLimitType::Inner;
	InnerSphere.bEnable = true;
	InnerLimits.Add(InnerSphere);
	A.CallSphereCollision(Inner, InnerLimits);
	TestTrue(FString::Printf(TEXT("Sphere inner pull-in: got %s expected (7,0,0)"), *Inner.Location.ToString()),
	         Inner.Location.Equals(FVector(7, 0, 0), GCollisionTol));

	return true;
}

// ---------------------------------------------------------------------------
//  Capsule
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsCapsuleTest,
                                 "KawaiiPhysics.Collision.CapsulePushOut",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsCapsuleTest::RunTest(const FString& Parameters)
{
	FKawaiiPhysicsTestAccessor A;

	// Z 軸カプセル, 中心 O, 長さ 10, 半径 4。ボーン半径 3 が (2,0,0) に食い込み。
	// 軸への最近点 (0,0,0)、LimitDistance = 3+4 = 7 → (7,0,0)。
	FKawaiiPhysicsModifyBone Bone = MakeBone(FVector(2, 0, 0), 3.0f, FVector(2, 0, 0));

	TArray<FCapsuleLimit> Limits;
	FCapsuleLimit Capsule;
	Capsule.Location = FVector::ZeroVector;
	Capsule.Rotation = FQuat::Identity;
	Capsule.Length = 10.0f;
	Capsule.Radius = 4.0f;
	Capsule.bEnable = true;
	Limits.Add(Capsule);

	A.CallCapsuleCollision(Bone, Limits);

	const FVector Expected(7, 0, 0);
	TestTrue(FString::Printf(TEXT("Capsule push-out: got %s expected %s"),
	                         *Bone.Location.ToString(), *Expected.ToString()),
	         Bone.Location.Equals(Expected, GCollisionTol));

	return true;
}

// ---------------------------------------------------------------------------
//  Box
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsBoxTest,
                                 "KawaiiPhysics.Collision.BoxPushOut",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsBoxTest::RunTest(const FString& Parameters)
{
	FKawaiiPhysicsTestAccessor A;

	// 原点・無回転・extent 10 のボックス。ボーン半径 3 が (12,0,0)（面 X=10 の外側 2）に食い込み。
	// 最近点 (10,0,0)、押し出し → (10,0,0)+(1,0,0)*3 = (13,0,0)。
	FKawaiiPhysicsModifyBone Bone = MakeBone(FVector(12, 0, 0), 3.0f, FVector(12, 0, 0));

	TArray<FBoxLimit> Limits;
	FBoxLimit Box;
	Box.Location = FVector::ZeroVector;
	Box.Rotation = FQuat::Identity;
	Box.Extent = FVector(10, 10, 10);
	Box.bEnable = true;
	Limits.Add(Box);

	A.CallBoxCollision(Bone, Limits);

	const FVector Expected(13, 0, 0);
	TestTrue(FString::Printf(TEXT("Box push-out: got %s expected %s"),
	                         *Bone.Location.ToString(), *Expected.ToString()),
	         Bone.Location.Equals(Expected, GCollisionTol));

	// 完全に内部（buried）ケース。注: 現行アルゴリズムは中心方向へ「半径ぶん」だけ押すため箱から出きらず、
	// (5,0,0)+(1,0,0)*3 = (8,0,0) で止まる（理想の押し出し (13,0,0) ではない）。現挙動の固定＝リグレッション検出用。
	// Buried case. NOTE: the current algorithm pushes only "radius" along the center direction, so the bone does
	// not fully exit the box and stops at (8,0,0) (not the ideal (13,0,0)). This pins current behavior (regression).
	FKawaiiPhysicsModifyBone Buried = MakeBone(FVector(5, 0, 0), 3.0f, FVector(5, 0, 0));
	A.CallBoxCollision(Buried, Limits);
	TestTrue(FString::Printf(TEXT("Box buried push (pins current behavior): got %s expected (8,0,0)"),
	                         *Buried.Location.ToString()),
	         Buried.Location.Equals(FVector(8, 0, 0), GCollisionTol));

	// 中心ちょうど一致の縮退ケース。修正前は半径方向が定まらず押し出されない（中心に留まる）バグ。
	// 修正後は最小貫通軸（X==Y==Z のため +X）へ決定的に押し出され、closest点(=中心)+ (1,0,0)*3 = (3,0,0)。
	// 注: (3,0,0) はまだ Box 内部だが、これは上の buried ケース(5→8)と同じ既存挙動（半径ぶんのみ押す）。
	// 本修正の目的は完全脱出ではなく「中心に留まって全く動かない」ゼロ法線バグの解消。
	// Center-coincident degenerate case. Before the fix the radial direction was undefined and the bone was not
	// pushed at all (stuck at the center). After the fix it is pushed deterministically along the smallest-penetration
	// axis (+X since X==Y==Z), landing at closestPoint(=center) + (1,0,0)*3 = (3,0,0).
	// NOTE: (3,0,0) is still inside the box, matching the existing buried behavior (5->8 above): the fix targets the
	// zero-normal stuck-at-center bug, not full de-penetration.
	FKawaiiPhysicsModifyBone Center = MakeBone(FVector(0, 0, 0), 3.0f, FVector(0, 0, 0));
	A.CallBoxCollision(Center, Limits);
	TestTrue(FString::Printf(TEXT("Box center-coincident push-out: got %s expected (3,0,0)"),
	                         *Center.Location.ToString()),
	         Center.Location.Equals(FVector(3, 0, 0), GCollisionTol));

	return true;
}

// ---------------------------------------------------------------------------
//  Planar
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsPlanarTest,
                                 "KawaiiPhysics.Collision.PlanarPushOut",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsPlanarTest::RunTest(const FString& Parameters)
{
	FKawaiiPhysicsTestAccessor A;

	// z=0 平面（法線 +Z）。ボーン半径 3 が平面の下 (0,0,-5)、前フレーム (0,0,5) で平面をまたぐ。
	// → 平面射影 (0,0,0) + UpVector*3 = (0,0,3)。
	FKawaiiPhysicsModifyBone Bone = MakeBone(FVector(0, 0, -5), 3.0f, FVector(0, 0, 5));

	TArray<FPlanarLimit> Limits;
	FPlanarLimit Planar;
	Planar.Location = FVector::ZeroVector;
	Planar.Rotation = FQuat::Identity; // UpVector = +Z
	Planar.Plane = FPlane(0, 0, 1, 0); // z = 0
	Planar.bEnable = true;
	Limits.Add(Planar);

	A.CallPlanarCollision(Bone, Limits);

	const FVector Expected(0, 0, 3);
	TestTrue(FString::Printf(TEXT("Planar push-out: got %s expected %s"),
	                         *Bone.Location.ToString(), *Expected.ToString()),
	         Bone.Location.Equals(Expected, GCollisionTol));

	return true;
}

// ---------------------------------------------------------------------------
//  Angle Limit
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsAngleLimitTest,
                                 "KawaiiPhysics.Collision.AngleLimit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsAngleLimitTest::RunTest(const FString& Parameters)
{
	FKawaiiPhysicsTestAccessor A;

	// 親原点、ポーズ方向 +X（child pose (10,0,0)）。現在 child は (0,10,0)=+Y（ポーズから 90°）。
	// LimitAngle 30° → 30° まで戻され、長さ 10 を保って (10cos30, 10sin30, 0)=(8.660,5,0)。
	FKawaiiPhysicsModifyBone Parent;
	Parent.Location = FVector::ZeroVector;
	Parent.PoseLocation = FVector::ZeroVector;

	FKawaiiPhysicsModifyBone Child;
	Child.Location = FVector(0, 10, 0);
	Child.PoseLocation = FVector(10, 0, 0);
	Child.PhysicsSettings.LimitAngle = 30.0f;

	A.CallAngleLimit(Child, Parent);

	const FVector Expected(10.0f * FMath::Cos(FMath::DegreesToRadians(30.0f)),
	                       10.0f * FMath::Sin(FMath::DegreesToRadians(30.0f)), 0.0f);
	TestTrue(FString::Printf(TEXT("Angle limit: got %s expected %s"),
	                         *Child.Location.ToString(), *Expected.ToString()),
	         Child.Location.Equals(Expected, GCollisionTol));

	// 距離（ボーン長）が保存されること / bone length preserved.
	TestTrue(TEXT("Angle limit preserves bone length"),
	         FMath::IsNearlyEqual(static_cast<float>((Child.Location - Parent.Location).Size()), 10.0f, GCollisionTol));

	// 制限角度内のボーンは動かさない / a bone within the limit must not move.
	FKawaiiPhysicsModifyBone Within;
	Within.Location = FVector(10, 0, 0); // ポーズと一致（0°）
	Within.PoseLocation = FVector(10, 0, 0);
	Within.PhysicsSettings.LimitAngle = 30.0f;
	A.CallAngleLimit(Within, Parent);
	TestTrue(TEXT("Angle limit: bone within limit is untouched"),
	         Within.Location.Equals(FVector(10, 0, 0), GCollisionTol));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
