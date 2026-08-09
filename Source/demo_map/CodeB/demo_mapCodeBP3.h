// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CodeB/demo_mapCodeBP2.h"

#include "Misc/Optional.h"
#include "Templates/UniquePtr.h"

namespace demo_map_code_b
{
	enum class ECodeBP3CellState : uint8
	{
		Empty,
		Hidden,
		Searching,
		Revealed
	};

	enum class ECodeBP3InventoryScope : uint8
	{
		Unknown,
		OutOfRaidPlayer,
		OutOfRaidWarehouse,
		InRunPlayer,
		ExternalTarget
	};

	enum class ECodeBP3SearchTargetKind : uint8
	{
		None,
		NormalContainer,
		BodyContainer
	};

	/**
	 * P4's non-secret search identity. Hidden cells never expose an ItemId to the
	 * widget/input route; the action service resolves the persisted target slot.
	 */
	struct FCodeBP3SearchLocator
	{
		ECodeBP3SearchTargetKind TargetKind = ECodeBP3SearchTargetKind::None;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid TargetId;
		FGuid ContainerId;
		int32 SlotIndex = INDEX_NONE;
		int32 TargetRevision = INDEX_NONE;
		FGuid ActiveActionId;

		bool IsValid() const
		{
			return TargetKind != ECodeBP3SearchTargetKind::None
				&& OwnerId.IsValid() && RunInstanceId.IsValid() && TargetId.IsValid()
				&& ContainerId.IsValid() && SlotIndex != INDEX_NONE
				&& TargetRevision != INDEX_NONE;
		}
	};

	/** A stable, projection-derived address.  P3 only retains this display address and ItemId; P1 remains mutable authority. */
	struct FCodeBP3SlotAddress
	{
		FGuid ContainerId;
		int32 SlotIndex = INDEX_NONE;
		FName SlotId;
		FGuid ItemId;
		bool bOccupied = false;
		ECodeBP3InventoryScope Scope = ECodeBP3InventoryScope::Unknown;
		FGuid OwnerId;
		FGuid RunInstanceId;
		ECodeBP3CellState CellState = ECodeBP3CellState::Empty;
		FCodeBP3SearchLocator SearchLocator;

		bool IsValid() const { return ContainerId.IsValid() && SlotIndex != INDEX_NONE; }
		bool IsRevealed() const { return CellState == ECodeBP3CellState::Revealed && bOccupied && ItemId.IsValid(); }
	};

	enum class ECodeBP3OperationMode : uint8
	{
		None,
		Move,
		Equip,
		Unequip,
		Split,
		Merge
	};

	/** The same mounted P3/P4 surface can bind either the P5 Profile or the P6 Run session. */
	enum class ECodeBP3PresentationScope : uint8
	{
		OutOfRaidProfile,
		ActiveRun
	};

	/** P23's authority-facing mode for the one shared inventory workspace. */
	enum class ECodeBP3WorkspaceScope : uint8
	{
		Development,
		OutOfRaidP5,
		InRunP6
	};

	/** A narrow top-level write gate; it never mirrors Profile or Run inventory state. */
	enum class ECodeBP3WorkspaceWriteGate : uint8
	{
		Unavailable,
		AtSect,
		StartAttemptPending,
		InRun,
		ResolvingTerminal
	};

	/**
	 * Transient workspace identity and interaction state. Items and placements stay
	 * exclusively in the current P2 projection/P1 repository.
	 */
	struct FCodeBP3InventoryWorkspaceContext
	{
		ECodeBP3WorkspaceScope Scope = ECodeBP3WorkspaceScope::Development;
		FGuid OwnerId;
		FGuid RunInstanceId;
		int32 SessionRevision = INDEX_NONE;
		ECodeBP3WorkspaceWriteGate WriteGate = ECodeBP3WorkspaceWriteGate::Unavailable;
		FName PlayerPaneId = FName(TEXT("Player"));
		FName TargetPaneId = FName(TEXT("Target"));
		TOptional<FGuid> ActiveDestinationContainerId;
		TOptional<FCodeBP3SlotAddress> HoveredAddress;
		TOptional<FCodeBP3SlotAddress> SelectedAddress;
		float PlayerScrollOffset = 0.0f;
		float TargetScrollOffset = 0.0f;

		bool IsOutOfRaidP5() const { return Scope == ECodeBP3WorkspaceScope::OutOfRaidP5; }
		bool IsInRun() const { return Scope == ECodeBP3WorkspaceScope::InRunP6; }
	};

	/**
	 * P24/P25's one transient quantity intent. It captures only stable authority
	 * identities and the requested quantity; P1 alone either creates the split
	 * identity for an empty target or applies the exact Merge to an existing stack.
	 */
	struct FCodeBP3SplitDraft
	{
		ECodeBP3WorkspaceScope WorkspaceScope = ECodeBP3WorkspaceScope::Development;
		ECodeBP3InventoryScope SourceScope = ECodeBP3InventoryScope::Unknown;
		FGuid OwnerId;
		FGuid RunInstanceId;
		FGuid GraphIdentity;
		FCodeBP3SlotAddress Source;
		FGuid SourceItemId;
		int32 ExpectedRevision = INDEX_NONE;
		int32 RequestedQuantity = 0;

		bool IsValid() const
		{
			return Source.IsValid() && SourceItemId.IsValid() && GraphIdentity.IsValid()
				&& ExpectedRevision != INDEX_NONE && RequestedQuantity > 0;
		}
	};

	/** P17's read-only active-Run view of one actual item-owned space container. */
	struct FCodeBP7SpatialContainerProjection
	{
		FGuid ParentItemId;
		FName ParentDefinitionId;
		FGuid ContainerId;
		ECodeBSpatialContainerSemantic Semantic = ECodeBSpatialContainerSemantic::None;
		bool bQuickEntry = false;
		FCodeBP2ContainerView Container;
	};

	/**
	 * Presentation controller shared by the P3 native widget and automation.  It owns only an isolated
	 * development fixture/service and UI transient state.  It never edits P1 data outside P2::Apply.
	 */
	class FCodeBP3UIController
	{
	public:
		using FProfileCommit = TFunction<bool(const FCodeBSnapshot&, FString&)>;

		bool Open(FString* OutError = nullptr);
		/** Opens a real Profile-owned repository without constructing the development fixture. */
		bool OpenProfile(
			FCodeBRepository& InRepository,
			const FCodeBP2PlayerLayout& InLayout,
			FProfileCommit InCommit,
			FString* OutError = nullptr);
		/** P7 production entry. It reuses P3/P4 and changes only the presentation scope. */
		bool OpenActiveRun(
			FCodeBRepository& InRepository,
			const FCodeBP2PlayerLayout& InLayout,
			FProfileCommit InCommit,
			FString* OutError = nullptr);
		void Close();
		void Shutdown();

		bool IsOpen() const { return bOpen; }
		bool HasFixture() const { return Fixture.IsValid() && Service.IsValid(); }
		bool IsProfileBacked() const { return ProfileRepository != nullptr; }
		bool IsActiveRunBacked() const { return PresentationScope == ECodeBP3PresentationScope::ActiveRun; }
		const FCodeBP2Projection& GetProjection() const { return Projection; }
		const FCodeBP2FixtureIds* GetFixtureIds() const;
		const FString& GetFeedback() const { return Feedback; }
		ECodeBP3OperationMode GetOperationMode() const { return OperationMode; }
		const TOptional<FCodeBP3SlotAddress>& GetSelectedAddress() const { return SelectedAddress; }
		const TOptional<FCodeBP3SlotAddress>& GetPendingSource() const { return PendingSource; }
		int32 GetSplitQuantity() const { return SplitQuantity; }

		bool MakeAddress(const FGuid& ContainerId, int32 SlotIndex, FCodeBP3SlotAddress& OutAddress) const;
		bool SelectAddress(const FCodeBP3SlotAddress& Address);
		/** Authority-backed P24 source check; it never mutates the repository. */
		bool ValidateSplitSource(const FCodeBP3SlotAddress& Address, int32 RequestedQuantity, FString& OutError) const;
		bool BeginOperation(ECodeBP3OperationMode InOperationMode);
		void CancelOperation(const FString& Reason = TEXT("已取消当前操作"));
		/** Clears an item selection whose authoritative P6 graph just left this transient page. */
		void ClearTransientSelection(const FString& Reason = TEXT("物品图已离开当前页面"));
		void SetSplitQuantity(int32 InQuantity) { SplitQuantity = InQuantity; }
		bool ActivateAddress(const FCodeBP3SlotAddress& TargetAddress, int32 ExpectedRevisionOverride = INDEX_NONE);
		/** P4-only UI gesture bridge.  The caller supplies stable projection values; this controller remains the sole UI route into P2. */
		bool CommitP4Operation(ECodeBOperation Operation, const FCodeBP3SlotAddress& Source, const FCodeBP3SlotAddress& Target, int32 ExpectedRevision, int32 Quantity, const FString& OperationLabel);
		void SetP4Feedback(const FString& InFeedback) { Feedback = InFeedback; }
		bool RefreshProjection(FString* OutError = nullptr);
		/** P17 read-only entry. Quick rings require their exact current P6 equipment slot. */
		bool EnterActiveRunSpatialContainer(const FGuid& ParentItemId, FCodeBP7SpatialContainerProjection& OutProjection, FString* OutError = nullptr);
		/** Closes only P7's transient read-only entry; it never writes P1/P6 data. */
		void ReturnFromActiveRunSpatialContainer();
		const TOptional<FCodeBP7SpatialContainerProjection>& GetActiveRunSpatialContainer() const { return ActiveRunSpatialContainer; }

		static FString GetModeLabel(ECodeBP3OperationMode InMode);
		static FString GetResultFeedback(const FCodeBP2ApplicationResult& Result);

	private:
		const FCodeBP2ContainerView* FindContainerView(const FGuid& ContainerId) const;
		bool BuildActiveRunSpatialContainerProjection(
			const FGuid& ParentItemId,
			FCodeBP7SpatialContainerProjection& OutProjection,
			FString* OutError = nullptr) const;
		bool FindAddressForItem(const FGuid& ItemId, FCodeBP3SlotAddress& OutAddress) const;
		void RestoreSelectionAfterResult(const FCodeBP2ApplicationResult& Result, const FGuid& PreferredItemId);
		FCodeBP2Command MakeCommand(const FCodeBP3SlotAddress& Source, const FCodeBP3SlotAddress& Target, int32 ExpectedRevision) const;
		bool PersistProfileSnapshotAfterAcceptedP1(const FCodeBSnapshot& BeforeSnapshot, FString& OutError);
		bool OpenRepository(
			FCodeBRepository& InRepository,
			const FCodeBP2PlayerLayout& InLayout,
			FProfileCommit InCommit,
			ECodeBP3PresentationScope InPresentationScope,
			FString* OutError);
		static FGuid MakeTransactionId(uint32 Serial);

		TUniquePtr<FCodeBP2Fixture> Fixture;
		TUniquePtr<FCodeBP2ApplicationService> Service;
		FCodeBRepository* ProfileRepository = nullptr;
		FProfileCommit ProfileCommit;
		FCodeBP2Projection Projection;
		FCodeBP2PlayerLayout ActiveLayout;
		TOptional<FCodeBP7SpatialContainerProjection> ActiveRunSpatialContainer;
		TOptional<FCodeBP3SlotAddress> SelectedAddress;
		TOptional<FCodeBP3SlotAddress> PendingSource;
		ECodeBP3OperationMode OperationMode = ECodeBP3OperationMode::None;
		FString Feedback = TEXT("开发 Host 未启动");
		int32 SplitQuantity = 1;
		uint32 TransactionSerial = 1;
		ECodeBP3PresentationScope PresentationScope = ECodeBP3PresentationScope::OutOfRaidProfile;
		bool bOpen = false;
	};
}
