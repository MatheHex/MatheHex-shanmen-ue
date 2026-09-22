#include "demo_mapShanmenItemDefinitionAdapter.h"
#include "demo_mapItemDefinitions.h"
#include "ShanmenItemTags.h"

bool Fdemo_mapShanmenItemDefinitionAdapter::Build(
	FName DefinitionId, FShanmenItemDefinition& OutDefinition)
{
	OutDefinition = FShanmenItemDefinition();
	const auto* Product = Fdemo_mapItemDefinitions::Find(DefinitionId);
	if (!Product || !Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
		Product->ContentVersionId, Product->ContentDigest)) { return false; }
	FShanmenItemDefinition Candidate;
	Candidate.DefinitionId = Product->DefinitionId;
	Candidate.MaxStack = Product->MaxStackSize;
	Candidate.MaxDurability = Product->MaxDurability;
	Candidate.MaxCharges = Product->MaxCharges;
	if (Product->MaxStackSize > 1)
	{
		Candidate.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityConsumeQuantity());
	}
	else if (Product->MaxStackSize == 1 && !Product->CompatibleSlotIds.IsEmpty())
	{
		Candidate.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityDeploy());
	}
	if (Product->MaxDurability > 0) { Candidate.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityDurability()); }
	if (Product->MaxCharges > 0) { Candidate.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityCharges()); }
	if (Product->HasGameplaySemantic(Edemo_mapItemGameplaySemantic::ThrownWeapon))
	{
		Candidate.ItemTags.AddTag(FShanmenItemNativeTags::ItemWeaponThrown());
	}
	if (Product->HasGameplaySemantic(Edemo_mapItemGameplaySemantic::FlyingSword))
	{
		Candidate.ItemTags.AddTag(FShanmenItemNativeTags::ItemWeaponFlyingSword());
	}
	if (!Candidate.IsValid()) { return false; }
	OutDefinition = MoveTemp(Candidate);
	return true;
}
