#include "ShanmenDeterministicId.h"

#include "Misc/SecureHash.h"

namespace
{
	uint32 ReadBigEndianUint32(const uint8* Bytes)
	{
		return (static_cast<uint32>(Bytes[0]) << 24)
			| (static_cast<uint32>(Bytes[1]) << 16)
			| (static_cast<uint32>(Bytes[2]) << 8)
			| static_cast<uint32>(Bytes[3]);
	}
}

FGuid FShanmenDeterministicId::FromCanonicalString(FName Namespace, const FString& Payload)
{
	return FromCanonicalParts(Namespace, { Payload });
}

FGuid FShanmenDeterministicId::FromCanonicalParts(FName Namespace, const TArray<FString>& Parts)
{
	if (Namespace.IsNone())
	{
		return FGuid();
	}

	FString Canonical;
	Canonical.Reserve(64 + Parts.Num() * 32);
	Canonical.Append(Namespace.ToString());
	Canonical.AppendChar(TEXT('|'));

	for (const FString& Part : Parts)
	{
		Canonical.Appendf(TEXT("%d:"), Part.Len());
		Canonical.Append(Part);
		Canonical.AppendChar(TEXT('|'));
	}

	FTCHARToUTF8 Utf8(*Canonical);
	uint8 Hash[FSHA1::DigestSize];
	FSHA1::HashBuffer(Utf8.Get(), static_cast<uint64>(Utf8.Length()), Hash);

	FGuid Result(
		ReadBigEndianUint32(Hash),
		ReadBigEndianUint32(Hash + 4),
		ReadBigEndianUint32(Hash + 8),
		ReadBigEndianUint32(Hash + 12));

	// A valid canonical input must never degrade into the sentinel invalid identity.
	if (!Result.IsValid())
	{
		Result.D = 1;
	}
	return Result;
}
