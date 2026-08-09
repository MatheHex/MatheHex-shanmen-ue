#pragma once

#include "CoreMinimal.h"

class Ademo_mapGameMode;
struct Fdemo_map0909BWarehousePresentation;
struct Fdemo_map0909BStartDiagnostic;

/** Read-only Editor/game-entry diagnostic for the 0.0.9B default shell. */
class Fdemo_map0909BEditorSupport
{
public:
	static bool ValidateDefaultEntry(
		const Ademo_mapGameMode& GameMode,
		FString& OutDiagnostic);
	/** Read-only P2 projection audit; it exposes no inventory command or writer. */
	static bool ValidateWarehouseProjection(
		const Fdemo_map0909BWarehousePresentation& Presentation,
		FString& OutDiagnostic);
	/** Read-only P3 attempt-correlation audit; it owns no callback or state. */
	static bool ValidateStartAttemptAudit(
		const Fdemo_map0909BStartDiagnostic& Diagnostic,
		FString& OutDiagnostic);
};
