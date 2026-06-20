# KawaiiPhysics 自動テスト / Automated Tests

物理計算の回帰を自動検出するための、UE Automation ベースのヘッドレステスト。
コード追加・変更が物理挙動へ与える影響を機械的に検出することを目的とする。

Headless UE-Automation tests that mechanically detect regressions in the physics core when
code is added or changed.

## 何を守るか / What it guards

| テスト名 / Test | 性質 / Property |
| --- | --- |
| `KawaiiPhysics.Simulation.IntegrationCore` | 抽出した物理計算関数（ExtraVelocity・legacy gravity・simple external force）を解析的に検証 |
| `KawaiiPhysics.Simulation.BoneConstraintStepDeltaTime` | BoneConstraint XPBD compliance がサブステップ中に `StepDeltaTime` で正規化されることを解析的に検証 |
| `KawaiiPhysics.Simulation.Determinism` | 同一入力 → 同一出力（決定性） |
| `KawaiiPhysics.Simulation.ParameterResponse` | 重力方向・剛性単調性・減衰オーバーシュート＋スナップショット |
| `KawaiiPhysics.Simulation.FramerateIndependence` | フレームレート非依存性（固定サブステップで実現、BoneConstraint 横チェーンケースを含む） |
| `KawaiiPhysics.Simulation.NumericalStability` | NaN/Inf・発散なし（極端な dt・重力・ゼロ長・境界パラメータ） |
| `KawaiiPhysics.Collision.SphereOuterPushOut` | 球コリジョンの押し出し |
| `KawaiiPhysics.Collision.CapsulePushOut` | カプセルコリジョンの押し出し |
| `KawaiiPhysics.Collision.BoxPushOut` | ボックスコリジョンの押し出し |
| `KawaiiPhysics.Collision.PlanarPushOut` | 平面コリジョンの押し出し |
| `KawaiiPhysics.Collision.AngleLimit` | 角度制限（ポーズからの角度クランプ・ボーン長保存） |
| `KawaiiPhysics.Simulation.SyncBoneSubdivisionTargets` | BoneSubdivision の内部 dummy が SyncBone ターゲット/プレビューから除外されること |
| `KawaiiPhysics.Simulation.SyncBoneSubdivisionPoseRefresh` | SyncBone 後に BoneSubdivision dummy の Pose が実親/実子から再補間されること |
| `KawaiiPhysics.Simulation.SyncBoneSubdivisionRigidTranslation` | SyncBone+BoneSubdivision で子が stale inter-bone dummy 基準で歪まず剛体並進すること（残留歪み回帰） |
| `KawaiiPhysics.Simulation.SyncBoneSubdivisionLengthPreserved` | SyncBone+BoneSubdivision の非剛体（root/child で delta が異なる）でも実祖父基準で segment 長が保存されること |

## 設計 / Design

UE の AnimNode は本来 `FComponentSpacePoseContext`（実スケルタルメッシュ＋AnimInstanceProxy）が
必要でヘッドレス実行が難しい。そこで物理計算を **`Output` を引数に取らない関数**として切り出し、
本番 `Simulate()` とテストの双方が同じ関数を呼ぶ構成にした。

The physics core is factored into **`Output`-free core functions** that both the production `Simulate()`
and the tests call, so tests exercise the real production math without a skeletal mesh.

- **物理計算の関数群（メンバ関数, `AnimNode_KawaiiPhysicsSimulation.cpp`）**
  - `ComputeVerletStepVelocity()` — 速度再構成 → damping → +wind → gravity（戻り値の速度に、呼び出し側で ApplyToVelocity を適用）
  - `IntegrateVerletStepPosition()` — 速度のぶんだけ位置を更新
  - `ApplySimpleExternalForce()` — 速度を経由しない位置オフセット（位置空間の後処理）
  - `ApplyWorldMoveFollowNonBaseBone()` — Component/World 空間の移動追従
  - `ApplyStiffnessPull()` — Pull to Pose（剛性）
  - wind / ExternalForce velocity は `Simulate()` 側で集約し `ExtraVelocity` として渡す（加算のみ）。
- **すでにピュアなコリジョン/拘束関数**（`AnimNode_KawaiiPhysicsCollision.cpp`）
  - `AdjustBySphere/Capsule/Box/PlanerCollision`, `AdjustByAngleLimit`, `AdjustByPlanarConstraint` 等は
    `Output` を受けないため、ダミーボーンに対し直接呼び出してテストできる。
- **テストハーネス**（`Private/Tests/KawaiiPhysicsTestHarness.h`, `friend struct FKawaiiPhysicsTestAccessor`）
  - ダミーの縦チェーンを生成し、`SimulateModifyBones`/`SimulateOnce` の純粋部分の**処理順序を複製**して
    上記の物理計算関数＋コリジョン関数を呼ぶ。固定サブステップ（フレームレート非依存化）の積算ロジックも再現する。

### ハーネスが複製する範囲外 / Out of scope for the headless harness

以下は `Output`/実シーン依存のため、ヘッドレスハーネスの対象外（将来は実メッシュ統合テストで）。
The following depend on `Output`/the live scene and are not covered headlessly (future: integration tests):
wind、CustomExternalForce、ワールドコリジョン、BaseBoneSpace 変換、inter-bone/bridge ダミーの実シミュレーション、
SyncBone の完全な `ApplySyncBones`（シミュレーション空間変換を含む）。
※ SyncBone の target 収集・per-target `Apply`・dummy 再補間は上表のとおりヘッドレスで個別カバー済み。
※ wind/SkelComp移動の component 追従（固定サブステップの繰り越し含む）も実シーン依存のため要 PIE 検証。

> ハーネスの `StepOnce()`/`StepFrame()` は本番の処理の**呼び出し順序**を複製している。
> `SimulateOnce()`/`SimulateModifyBones()` のステップ順序を変えた場合は、ハーネスも合わせて更新すること
> （per-step の数式は共有関数なので二重管理にならない）。

## ビルド＆実行 / Build & Run

テストは `KawaiiPhysics` ランタイムモジュール内の `Private/Tests/` にあり、`WITH_DEV_AUTOMATION_TESTS`
でガードされる（出荷ビルドでは除外）。新規 `.cpp` は UBT が自動でコンパイルするため Build.cs 変更は不要。

```powershell
# 1) エディタターゲットをビルド / build the editor target
& "E:/Launcher/UE_5.7/Engine/Build/BatchFiles/Build.bat" KawaiiPhysicsSampleEditor Win64 Development -Project="F:/Github/KawaiiPhysics/KawaiiPhysicsSample.uproject"

# 2) ヘッドレスでテスト実行 / run tests headless
& "E:/Launcher/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "F:/Github/KawaiiPhysics/KawaiiPhysicsSample.uproject" `
  -ExecCmds="Automation RunTests KawaiiPhysics; Quit" `
  -unattended -nullrhi -nosplash -nopause -stdout -FullStdOutLogOutput `
  -testexit="Automation Test Queue Empty"
```

結果はログの `LogAutomationController` 行（`... Test Completed. Result={Success/Fail}`）と末尾の
サマリ（`Automation Test Succeeded/Failed`）で確認する。

> 注: 日本語パス対策として、ビルド前に一度だけ `git config core.quotePath false` が必要
> （無いと UBT が exit 82 でクラッシュする）。

## 新しいテストの追加 / Adding tests

1. `Private/Tests/` に `.cpp` を追加し、`#if WITH_DEV_AUTOMATION_TESTS` で囲む。
2. `IMPLEMENT_SIMPLE_AUTOMATION_TEST(FName, "KawaiiPhysics.<Category>.<Name>", EditorContext | EngineFilter)`。
3. `FKawaiiPhysicsTestAccessor` でチェーン構築・ステップ実行・コリジョン関数呼び出し。
4. 基準値は可能なら**解析的**に（コリジョンは表面+半径など）。積分系は決定性＋単調性＋スナップショット基準値で。
