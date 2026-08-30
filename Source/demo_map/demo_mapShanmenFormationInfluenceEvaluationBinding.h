#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceModifierEvaluator.h"

/**
 * Frozen evaluation payload carried by one influence execution request.
 *
 * Apply requires one self-validating receipt whose run, subject, policy,
 * influence, and content identities match the Host-owned intent. Remove uses
 * the canonical empty binding so the executor must reuse the active lease's
 * receipt instead of accepting newly sampled modifier evidence.
 */
class Fdemo_mapShanmenFormationInfluenceEvaluationBinding
{
public:
	static bool TryCaptureApply(
		const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Receipt,
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding& OutBinding);
	static Fdemo_mapShanmenFormationInfluenceEvaluationBinding MakeRemove();

	bool IsStructurallyValid() const;
	bool MatchesIntent(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent) const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceEvaluationBinding& Other)
		const;
	bool HasReceipt() const { return bHasReceipt; }
	const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& GetReceipt()
		const
	{
		return Receipt;
	}
	FGuid GetReceiptId() const
	{
		return bHasReceipt ? Receipt.ReceiptId : FGuid();
	}

	static bool ReceiptMatchesInfluenceIdentity(
		const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Receipt,
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent);

private:
	bool bHasReceipt = false;
	Fdemo_mapShanmenFormationInfluenceEvaluationReceipt Receipt;
};
