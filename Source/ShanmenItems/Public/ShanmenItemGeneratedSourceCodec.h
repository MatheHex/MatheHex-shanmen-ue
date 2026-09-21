#pragma once

#include "ShanmenItemGeneratedSource.h"

class FJsonObject;

/**
 * Embedded resolved-plan wire format, not an authority document or durable receipt.
 * All 64-bit values use canonical unsigned decimal strings (the domain's signed
 * values are nonnegative). Never routes them through JSON's double representation.
 * Decoding requires registered gameplay tags and rejects unknown/missing fields,
 * unsupported versions, wrong types, out-of-range values and invalid domain plans.
 * Entry/affix order is retained; tag sets and FName spelling are canonicalized.
 * No IO, mutable ledger, source acceptance, or item-authority schema migration.
 */
struct SHANMENITEMS_API FShanmenItemGeneratedSourceCodec
{
	static constexpr int32 FormatVersion = 1;
	static bool Encode(const FShanmenItemGeneratedSourcePlan& Plan,
		TSharedPtr<FJsonObject>& OutObject, FString* OutError = nullptr);
	/** Failure clears OutPlan; it never publishes a partly decoded plan. */
	static bool Decode(const TSharedPtr<FJsonObject>& Object,
		FShanmenItemGeneratedSourcePlan& OutPlan, FString* OutError = nullptr);
};
