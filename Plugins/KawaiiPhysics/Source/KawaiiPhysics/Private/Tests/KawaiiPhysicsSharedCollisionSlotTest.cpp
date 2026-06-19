// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KawaiiPhysicsSharedCollisionSubsystem.h"

namespace
{
	constexpr float GSharedCollisionSlotTol = 0.001f;

	FSphericalLimit MakeSphere(float Base)
	{
		FSphericalLimit Limit;
		Limit.Location = FVector(Base, Base + 1.0f, Base + 2.0f);
		Limit.Radius = Base + 3.0f;
		Limit.LimitType = ESphericalLimitType::Outer;
		Limit.bEnable = true;
		return Limit;
	}

	FKawaiiPhysicsSharedCollisionData MakeSphericalData(float Base, int32 Count = 1)
	{
		FKawaiiPhysicsSharedCollisionData Data;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Data.SphericalLimits.Add(MakeSphere(Base + static_cast<float>(Index) * 10.0f));
		}
		return Data;
	}

	FKawaiiPhysicsSharedCollisionData MakeFullData(float Base)
	{
		FKawaiiPhysicsSharedCollisionData Data = MakeSphericalData(Base);

		FCapsuleLimit Capsule;
		Capsule.Location = FVector(Base + 10.0f, Base + 11.0f, Base + 12.0f);
		Capsule.Radius = Base + 13.0f;
		Capsule.Length = Base + 14.0f;
		Capsule.bEnable = true;
		Data.CapsuleLimits.Add(Capsule);

		FBoxLimit Box;
		Box.Location = FVector(Base + 20.0f, Base + 21.0f, Base + 22.0f);
		Box.Extent = FVector(Base + 23.0f, Base + 24.0f, Base + 25.0f);
		Box.bEnable = true;
		Data.BoxLimits.Add(Box);

		FPlanarLimit Planar;
		Planar.Location = FVector(Base + 30.0f, Base + 31.0f, Base + 32.0f);
		Planar.Plane = FPlane(0.0f, 0.0f, 1.0f, Base + 33.0f);
		Planar.bEnable = true;
		Data.PlanarLimits.Add(Planar);

		return Data;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiPhysicsSharedCollisionSourceSlotTest,
                                 "KawaiiPhysics.SharedCollision.SourceSlot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKawaiiPhysicsSharedCollisionSourceSlotTest::RunTest(const FString& Parameters)
{
	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		FKawaiiPhysicsSharedCollisionData OutData;

		TestTrue(TEXT("Default slot is expired"), Slot.IsExpired(GFrameCounter, 1));
		Slot.AppendTo(OutData);
		TestTrue(TEXT("Default slot appends no data"), OutData.IsEmpty());
	}

	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		Slot.Publish(MakeFullData(10.0f));

		FKawaiiPhysicsSharedCollisionData OutData;
		Slot.AppendTo(OutData);

		TestTrue(TEXT("Published data appends one sphere"), OutData.SphericalLimits.Num() == 1);
		TestTrue(TEXT("Published data appends one capsule"), OutData.CapsuleLimits.Num() == 1);
		TestTrue(TEXT("Published data appends one box"), OutData.BoxLimits.Num() == 1);
		TestTrue(TEXT("Published data appends one plane"), OutData.PlanarLimits.Num() == 1);
		TestTrue(TEXT("Sphere location is preserved"),
		         OutData.SphericalLimits[0].Location.Equals(FVector(10.0f, 11.0f, 12.0f), GSharedCollisionSlotTol));
		TestTrue(TEXT("Sphere radius is preserved"),
		         FMath::IsNearlyEqual(OutData.SphericalLimits[0].Radius, 13.0f, GSharedCollisionSlotTol));
		TestTrue(TEXT("Capsule location is preserved"),
		         OutData.CapsuleLimits[0].Location.Equals(FVector(20.0f, 21.0f, 22.0f), GSharedCollisionSlotTol));
		TestTrue(TEXT("Capsule radius is preserved"),
		         FMath::IsNearlyEqual(OutData.CapsuleLimits[0].Radius, 23.0f, GSharedCollisionSlotTol));
		TestTrue(TEXT("Box extent is preserved"),
		         OutData.BoxLimits[0].Extent.Equals(FVector(33.0f, 34.0f, 35.0f), GSharedCollisionSlotTol));
		TestTrue(TEXT("Planar limit is preserved"),
		         FMath::IsNearlyEqual(OutData.PlanarLimits[0].Plane.W, 43.0f, GSharedCollisionSlotTol));
	}

	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		Slot.Publish(MakeFullData(1.0f));
		Slot.Publish(MakeSphericalData(50.0f, 2));

		FKawaiiPhysicsSharedCollisionData OutData;
		Slot.AppendTo(OutData);

		TestTrue(TEXT("Second publish replaces spherical data"), OutData.SphericalLimits.Num() == 2);
		TestTrue(TEXT("Second publish removes old capsule data"), OutData.CapsuleLimits.Num() == 0);
		TestTrue(TEXT("Second publish removes old box data"), OutData.BoxLimits.Num() == 0);
		TestTrue(TEXT("Second publish removes old planar data"), OutData.PlanarLimits.Num() == 0);
		TestTrue(TEXT("Second publish first sphere radius"),
		         FMath::IsNearlyEqual(OutData.SphericalLimits[0].Radius, 53.0f, GSharedCollisionSlotTol));
		TestTrue(TEXT("Second publish second sphere radius"),
		         FMath::IsNearlyEqual(OutData.SphericalLimits[1].Radius, 63.0f, GSharedCollisionSlotTol));
	}

	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		Slot.Publish(MakeSphericalData(100.0f));

		FKawaiiPhysicsSharedCollisionData OutData;
		Slot.AppendTo(OutData);
		Slot.AppendTo(OutData);

		TestTrue(TEXT("AppendTo does not reset output data"), OutData.SphericalLimits.Num() == 2);
		TestTrue(TEXT("AppendTo preserves appended values"),
		         FMath::IsNearlyEqual(OutData.SphericalLimits[0].Radius, 103.0f, GSharedCollisionSlotTol)
		         && FMath::IsNearlyEqual(OutData.SphericalLimits[1].Radius, 103.0f, GSharedCollisionSlotTol));
	}

	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		Slot.Publish(MakeSphericalData(200.0f));

		const uint64 CurrentFrame = GFrameCounter;
		TestTrue(TEXT("Recently published slot is not expired"),
		         !Slot.IsExpired(CurrentFrame, 1));
		TestTrue(TEXT("MaxAge boundary is inclusive"),
		         !Slot.IsExpired(CurrentFrame + 1, 1));
		TestTrue(TEXT("Slot expires after MaxAge"),
		         Slot.IsExpired(CurrentFrame + 10, 1));
	}

	{
		FKawaiiPhysicsSharedCollisionSourceSlot Slot;
		Slot.Publish(MakeSphericalData(300.0f));
		Slot.MarkExpired();

		TestTrue(TEXT("MarkExpired makes the slot expired"), Slot.IsExpired(GFrameCounter, 1));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
