#pragma once

#include "CoreMinimal.h"

struct FShanmenItemRewardMetadata;
struct FShanmenItemTransactionReceipt;
struct FShanmenItemInstance;
struct FShanmenItemDefinition;

/** Stack quantities stay in ItemInstance. Birth metadata is immutable, not a
 * claim that every current unit was born there. Merge/split edges are retained
 * in the existing durable receipt ledger; this is not another balance store. */
class SHANMENITEMS_API FShanmenItemStackTransferPolicy
{
public:
	/** Only ordinary origin-role differences may mix. Special reward/affix facts
	 * still require exact equality, including their event identities. */
	static bool MetadataCompatible(const FShanmenItemRewardMetadata& A, const FShanmenItemRewardMetadata& B);
	static bool HasWitness(const FShanmenItemTransactionReceipt& Receipt);
	static bool IsWitnessShapeValid(const FShanmenItemTransactionReceipt& Receipt);
	/** Grid receipt ReservationIds: [source item, recipient/child item, witness].
	 * The last ID is an integrity witness, not a reservation or item identity. */
	static void Stamp(FShanmenItemTransactionReceipt& Receipt, const FGuid& Source, const FGuid& Recipient, const FGuid& Fingerprint);
	static bool Validate(const FShanmenItemTransactionReceipt& Receipt, const FGuid& Fingerprint,
		const FShanmenItemInstance& Source, const FShanmenItemInstance& Recipient, const FShanmenItemDefinition& Definition);
};
