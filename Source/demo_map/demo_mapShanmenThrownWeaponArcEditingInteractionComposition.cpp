#include "demo_mapShanmenThrownWeaponArcEditingInteractionComposition.h"

bool Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
TryComposeTargetRequest(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
	const FVector2D& RawTargetIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	OutRequest = Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
	return Read.IsProjected()
		&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureArcTargetIntent(
				Read.GetReadModel(), RawTargetIntent, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
TryComposeApexAdjustmentRequest(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
	const double RawNormalizedDelta,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	OutRequest = Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
	return Read.IsProjected()
		&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureArcApexAdjustment(
				Read.GetReadModel(), RawNormalizedDelta, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
TryComposeTargetClearRequest(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	OutRequest = Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
	return Read.IsProjected()
		&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureArcTargetClear(Read.GetReadModel(), OutRequest);
}
