#include "demo_mapShanmenControlledWeaponThreatSampleRouter.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "ShanmenWorldHitAdapter.h"

namespace
{
	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return Left.ToString(EGuidFormats::Digits)
			< Right.ToString(EGuidFormats::Digits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	int32 CompareFloat(float Left, float Right)
	{
		return Left < Right ? -1 : (Left > Right ? 1 : 0);
	}

	int32 CompareVector(const FVector& Left, const FVector& Right)
	{
		if (const int32 X = CompareFloat(Left.X, Right.X); X != 0)
		{
			return X;
		}
		if (const int32 Y = CompareFloat(Left.Y, Right.Y); Y != 0)
		{
			return Y;
		}
		return CompareFloat(Left.Z, Right.Z);
	}

	bool ContactLess(
		const Fdemo_mapShanmenControlledWeaponThreatContactIdentity& Left,
		const Fdemo_mapShanmenControlledWeaponThreatContactIdentity& Right)
	{
		if (Left.TargetEntityId != Right.TargetEntityId)
		{
			return GuidLess(Left.TargetEntityId, Right.TargetEntityId);
		}
		if (Left.BodyIndex != Right.BodyIndex)
		{
			return Left.BodyIndex < Right.BodyIndex;
		}
		if (const int32 Location = CompareVector(
			Left.ContactLocation, Right.ContactLocation); Location != 0)
		{
			return Location < 0;
		}
		if (const int32 Normal = CompareVector(
			Left.ContactNormal, Right.ContactNormal); Normal != 0)
		{
			return Normal < 0;
		}
		return static_cast<uint8>(Left.bBlockingHit)
			< static_cast<uint8>(Right.bBlockingHit);
	}

	bool ItemIdentitiesMatch(
		const TArray<Fdemo_mapShanmenControlledWeaponThreatItemIdentity>& Left,
		const TArray<Fdemo_mapShanmenControlledWeaponThreatItemIdentity>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!Left[Index].Matches(Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	struct FCapturedContact
	{
		Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact;
		Fdemo_mapShanmenControlledWeaponThreatContactIdentity Identity;
	};

	struct FCapturedItem
	{
		Fdemo_mapShanmenControlledWeaponThreatSampleRequest Request;
		Fdemo_mapShanmenControlledWeaponThreatItemIdentity Identity;
	};
}

bool Fdemo_mapShanmenControlledWeaponThreatContactIdentity::IsValid() const
{
	return TargetEntityId.IsValid()
		&& IsFiniteVector(ContactLocation)
		&& IsFiniteVector(ContactNormal);
}

bool Fdemo_mapShanmenControlledWeaponThreatContactIdentity::Matches(
	const Fdemo_mapShanmenControlledWeaponThreatContactIdentity& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& TargetEntityId == Other.TargetEntityId
		&& BodyIndex == Other.BodyIndex
		&& ContactLocation == Other.ContactLocation
		&& ContactNormal == Other.ContactNormal
		&& bBlockingHit == Other.bBlockingHit;
}

bool Fdemo_mapShanmenControlledWeaponThreatItemIdentity::IsValid() const
{
	if (!ItemInstanceId.IsValid())
	{
		return false;
	}
	for (int32 Index = 0; Index < Contacts.Num(); ++Index)
	{
		if (!Contacts[Index].IsValid()
			|| (Index > 0 && ContactLess(Contacts[Index], Contacts[Index - 1])))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatItemIdentity::Matches(
	const Fdemo_mapShanmenControlledWeaponThreatItemIdentity& Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| ItemInstanceId != Other.ItemInstanceId
		|| Contacts.Num() != Other.Contacts.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Contacts.Num(); ++Index)
	{
		if (!Contacts[Index].Matches(Other.Contacts[Index]))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleIntent::TryCapture(
	const FGuid& RequestedIntentId,
	const FGuid& RequestedRunId,
	int64 RequestedSampleSequence,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>&
		RequestedRequests,
	Fdemo_mapShanmenControlledWeaponThreatSampleIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenControlledWeaponThreatSampleIntent();
	if (!RequestedIntentId.IsValid()
		|| !RequestedRunId.IsValid()
		|| RequestedSampleSequence < 0
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != RequestedRunId
		|| RequestedRequests.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> UniqueItems;
	TArray<FCapturedItem> CapturedItems;
	CapturedItems.Reserve(RequestedRequests.Num());
	for (const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Request :
		RequestedRequests)
	{
		if (!Request.IsValid()
			|| UniqueItems.Contains(Request.ItemInstanceId))
		{
			return false;
		}
		UniqueItems.Add(Request.ItemInstanceId);

		FCapturedItem CapturedItem;
		CapturedItem.Request.ItemInstanceId = Request.ItemInstanceId;
		CapturedItem.Identity.ItemInstanceId = Request.ItemInstanceId;
		TArray<FCapturedContact> CapturedContacts;
		CapturedContacts.Reserve(Request.Contacts.Num());
		for (const Fdemo_mapShanmenControlledWeaponOrbitThreatContact& Contact :
			Request.Contacts)
		{
			if (!IsFiniteVector(Contact.ContactLocation)
				|| !IsFiniteVector(Contact.ContactNormal))
			{
				return false;
			}
			AActor* TargetActor = Contact.Overlap.GetActor();
			UPrimitiveComponent* TargetComponent =
				Contact.Overlap.GetComponent();
			FGuid TargetEntityId;
			if (!TargetActor
				|| !Coordinator.GetEntityRegistry().TryResolveEntityId(
					RequestedRunId,
					EShanmenWorldContactSource::Overlap,
					TargetActor,
					TargetComponent,
					Contact.Overlap.ItemIndex,
					TargetEntityId)
				|| !TargetEntityId.IsValid())
			{
				return false;
			}

			FCapturedContact CapturedContact;
			CapturedContact.Contact = Contact;
			CapturedContact.Identity.TargetEntityId = TargetEntityId;
			CapturedContact.Identity.BodyIndex = Contact.Overlap.ItemIndex;
			CapturedContact.Identity.ContactLocation = Contact.ContactLocation;
			CapturedContact.Identity.ContactNormal = Contact.ContactNormal;
			CapturedContact.Identity.bBlockingHit =
				Contact.Overlap.bBlockingHit;
			if (!CapturedContact.Identity.IsValid())
			{
				return false;
			}
			CapturedContacts.Add(MoveTemp(CapturedContact));
		}
		CapturedContacts.Sort([](
			const FCapturedContact& Left,
			const FCapturedContact& Right)
		{
			return ContactLess(Left.Identity, Right.Identity);
		});
		for (FCapturedContact& CapturedContact : CapturedContacts)
		{
			CapturedItem.Request.Contacts.Add(
				MoveTemp(CapturedContact.Contact));
			CapturedItem.Identity.Contacts.Add(
				MoveTemp(CapturedContact.Identity));
		}
		CapturedItems.Add(MoveTemp(CapturedItem));
	}

	CapturedItems.Sort([](
		const FCapturedItem& Left,
		const FCapturedItem& Right)
	{
		return GuidLess(
			Left.Identity.ItemInstanceId,
			Right.Identity.ItemInstanceId);
	});

	OutIntent.IntentId = RequestedIntentId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.SampleSequence = RequestedSampleSequence;
	for (FCapturedItem& CapturedItem : CapturedItems)
	{
		OutIntent.Requests.Add(MoveTemp(CapturedItem.Request));
		OutIntent.ItemIdentities.Add(MoveTemp(CapturedItem.Identity));
	}
	return OutIntent.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleIntent::IsValid() const
{
	if (!IntentId.IsValid()
		|| !RunId.IsValid()
		|| SampleSequence < 0
		|| Requests.IsEmpty()
		|| Requests.Num() != ItemIdentities.Num())
	{
		return false;
	}
	for (int32 ItemIndex = 0; ItemIndex < Requests.Num(); ++ItemIndex)
	{
		const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Request =
			Requests[ItemIndex];
		const Fdemo_mapShanmenControlledWeaponThreatItemIdentity& Identity =
			ItemIdentities[ItemIndex];
		if (!Request.IsValid()
			|| !Identity.IsValid()
			|| Request.ItemInstanceId != Identity.ItemInstanceId
			|| Request.Contacts.Num() != Identity.Contacts.Num()
			|| (ItemIndex > 0
				&& !GuidLess(
					Requests[ItemIndex - 1].ItemInstanceId,
					Request.ItemInstanceId)))
		{
			return false;
		}
		for (int32 ContactIndex = 0;
			ContactIndex < Request.Contacts.Num();
			++ContactIndex)
		{
			const Fdemo_mapShanmenControlledWeaponOrbitThreatContact& Contact =
				Request.Contacts[ContactIndex];
			const Fdemo_mapShanmenControlledWeaponThreatContactIdentity&
				ContactIdentity = Identity.Contacts[ContactIndex];
			if (!Contact.Overlap.GetActor()
				|| !IsFiniteVector(Contact.ContactLocation)
				|| !IsFiniteVector(Contact.ContactNormal)
				|| Contact.Overlap.ItemIndex != ContactIdentity.BodyIndex
				|| Contact.ContactLocation != ContactIdentity.ContactLocation
				|| Contact.ContactNormal != ContactIdentity.ContactNormal
				|| Contact.Overlap.bBlockingHit
					!= ContactIdentity.bBlockingHit)
			{
				return false;
			}
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleIntent::MatchesCoordinator(
	const Fdemo_mapCombatRunCoordinator& Coordinator) const
{
	if (!IsValid()
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != RunId)
	{
		return false;
	}
	for (int32 ItemIndex = 0; ItemIndex < Requests.Num(); ++ItemIndex)
	{
		for (int32 ContactIndex = 0;
			ContactIndex < Requests[ItemIndex].Contacts.Num();
			++ContactIndex)
		{
			const Fdemo_mapShanmenControlledWeaponOrbitThreatContact& Contact =
				Requests[ItemIndex].Contacts[ContactIndex];
			FGuid CurrentTargetEntityId;
			if (!Coordinator.GetEntityRegistry().TryResolveEntityId(
					RunId,
					EShanmenWorldContactSource::Overlap,
					Contact.Overlap.GetActor(),
					Contact.Overlap.GetComponent(),
					Contact.Overlap.ItemIndex,
					CurrentTargetEntityId)
				|| CurrentTargetEntityId
					!= ItemIdentities[ItemIndex].Contacts[ContactIndex]
						.TargetEntityId)
			{
				return false;
			}
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleIntent::Matches(
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& IntentId == Other.IntentId
		&& RunId == Other.RunId
		&& SampleSequence == Other.SampleSequence
		&& ItemIdentitiesMatch(ItemIdentities, Other.ItemIdentities);
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleResult::IsAccepted() const
{
	return (Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::Applied
		|| Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::Replayed)
		&& IntentId.IsValid()
		&& RunId.IsValid()
		&& SampleSequence >= 0
		&& ItemCount > 0
		&& Batch.IsFullyFinalized()
		&& Batch.RunId == RunId
		&& Batch.AttemptedCount == ItemCount
		&& Batch.FinalizedCount == ItemCount
		&& Batch.Entries.Num() == ItemCount;
}

Fdemo_mapShanmenControlledWeaponThreatSampleResult
Fdemo_mapShanmenControlledWeaponThreatSampleRouter::TryRoute(
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent)
{
	Fdemo_mapShanmenControlledWeaponThreatSampleResult Result;
	Result.IntentId = Intent.GetIntentId();
	Result.RunId = Intent.GetRunId();
	Result.SampleSequence = Intent.GetSampleSequence();
	Result.ItemCount = Intent.GetRequests().Num();
	if (!Coordinator.IsReady())
	{
		Result.Diagnostic =
			TEXT("Threat sample requires one ready Coordinator Run.");
		return Result;
	}
	if (!Intent.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::IntentInvalid;
		Result.Diagnostic = TEXT("Threat sample intent is invalid.");
		return Result;
	}
	if (!Host.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::HostInvalid;
		Result.Diagnostic = TEXT("Threat sample Host is invalid.");
		return Result;
	}
	if (Intent.GetRunId() != Coordinator.GetRunId()
		|| Intent.GetRunId() != Host.GetRunId()
		|| Host.GetSourceEntityId() != Coordinator.GetPlayerEntityId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::RunMismatch;
		Result.Diagnostic = TEXT("Threat sample identities do not name one Run.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::RouterInvalid;
		Result.Diagnostic = TEXT("Threat sample Router is invalid.");
		return Result;
	}
	if (!IsEmpty() && RunId != Intent.GetRunId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::RouterRunMismatch;
		Result.Diagnostic = TEXT("Threat sample Router belongs to another Run.");
		return Result;
	}

	if (LatestSample.IsSet()
		&& Intent.GetSampleSequence() == LatestSample->SampleSequence)
	{
		if (!LatestSample->Matches(Intent))
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponThreatSampleStatus::IntentConflict;
			Result.Diagnostic =
				TEXT("Latest threat sample identity was reused with another payload.");
			return Result;
		}
		Result = LatestSample->Result;
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::Replayed;
		Result.Diagnostic =
			TEXT("Threat sample returned its latest accepted receipts.");
		return Result;
	}
	if (!Intent.MatchesCoordinator(Coordinator))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::IntentInvalid;
		Result.Diagnostic =
			TEXT("Threat sample contact identity changed after capture.");
		return Result;
	}
	if (Intent.GetSampleSequence() == MAX_int64)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::SequenceExhausted;
		Result.Diagnostic = TEXT("Threat sample sequence cannot advance.");
		return Result;
	}
	if (Intent.GetSampleSequence() < NextSampleSequence)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::SequenceStale;
		Result.Diagnostic = TEXT("Threat sample sequence is stale.");
		return Result;
	}
	if (Intent.GetSampleSequence() > NextSampleSequence)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::SequenceGap;
		Result.Diagnostic = TEXT("Threat sample sequence is not contiguous.");
		return Result;
	}
	if (LatestSample.IsSet()
		&& LatestSample->IntentId == Intent.GetIntentId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::IntentConflict;
		Result.Diagnostic =
			TEXT("Threat sample IntentId cannot identify two sequences.");
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponRunHost HostCandidate = Host;
	if (!HostCandidate.TrySampleOrbitThreatsInOrder(
			Coordinator,
			Intent.GetRequests(),
			Result.Batch))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::BatchRejected;
		Result.Diagnostic =
			TEXT("Threat sample Host rejected the atomic batch.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenControlledWeaponThreatSampleStatus::Applied;
	Result.Diagnostic =
		TEXT("Threat sample committed in stable item/contact order.");
	if (!Result.IsAccepted() || !HostCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::BatchRejected;
		Result.Batch = Fdemo_mapShanmenControlledWeaponThreatSampleBatch();
		Result.Diagnostic =
			TEXT("Threat sample produced invalid staged state.");
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponThreatSampleRouter RouterCandidate = *this;
	if (RouterCandidate.IsEmpty())
	{
		RouterCandidate.RunId = Intent.GetRunId();
	}
	RouterCandidate.NextSampleSequence = Intent.GetSampleSequence() + 1;
	FLatestSample Latest;
	Latest.IntentId = Intent.GetIntentId();
	Latest.RunId = Intent.GetRunId();
	Latest.SampleSequence = Intent.GetSampleSequence();
	Latest.ItemIdentities = Intent.GetItemIdentities();
	Latest.Result = Result;
	RouterCandidate.LatestSample = MoveTemp(Latest);
	if (!RouterCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponThreatSampleStatus::RouterInvalid;
		Result.Batch = Fdemo_mapShanmenControlledWeaponThreatSampleBatch();
		Result.Diagnostic =
			TEXT("Threat sample could not commit its bounded replay record.");
		return Result;
	}

	Host = MoveTemp(HostCandidate);
	*this = MoveTemp(RouterCandidate);
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleRouter::FLatestSample::
IsValid() const
{
	if (!IntentId.IsValid()
		|| !RunId.IsValid()
		|| SampleSequence < 0
		|| ItemIdentities.IsEmpty()
		|| Result.Status
			!= Edemo_mapShanmenControlledWeaponThreatSampleStatus::Applied
		|| !Result.IsAccepted()
		|| Result.IntentId != IntentId
		|| Result.RunId != RunId
		|| Result.SampleSequence != SampleSequence
		|| Result.ItemCount != ItemIdentities.Num()
		|| Result.Batch.Entries.Num() != ItemIdentities.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < ItemIdentities.Num(); ++Index)
	{
		if (!ItemIdentities[Index].IsValid()
			|| (Index > 0
				&& !GuidLess(
					ItemIdentities[Index - 1].ItemInstanceId,
					ItemIdentities[Index].ItemInstanceId))
			|| Result.Batch.Entries[Index].ItemInstanceId
				!= ItemIdentities[Index].ItemInstanceId)
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleRouter::FLatestSample::
Matches(
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent) const
{
	return IsValid()
		&& Intent.IsValid()
		&& IntentId == Intent.GetIntentId()
		&& RunId == Intent.GetRunId()
		&& SampleSequence == Intent.GetSampleSequence()
		&& ItemIdentitiesMatch(ItemIdentities, Intent.GetItemIdentities());
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleRouter::IsValid() const
{
	if (!LatestSample.IsSet())
	{
		return !RunId.IsValid() && NextSampleSequence == 0;
	}
	return RunId.IsValid()
		&& NextSampleSequence > 0
		&& LatestSample->IsValid()
		&& LatestSample->RunId == RunId
		&& LatestSample->SampleSequence == NextSampleSequence - 1;
}

void Fdemo_mapShanmenControlledWeaponThreatSampleRouter::Reset()
{
	*this = Fdemo_mapShanmenControlledWeaponThreatSampleRouter();
}
