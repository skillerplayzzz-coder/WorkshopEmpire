#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WEComputerWidgets.generated.h"

class AWETraderTerminal;
class UTextBlock;

/**
 * Phase 17.5: what the workshop PC's monitor shows in the world (Ferrit OS lock screen / "in use"), rendered on the
 * monitor by a widget component for everybody in the room. The interactive desktop itself is UWETradeWidget.
 */
UCLASS()
class WORKSHOPEMPIRE_API UWEDesktopIdleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTerminal(AWETraderTerminal* InTerminal);
	/** The local player sits at this PC (the screen then shows the desktop look). */
	void SetLocalUse(bool bInUse) { bLocalUse = bInUse; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> ClockText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DateText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	TWeakObjectPtr<AWETraderTerminal> Terminal;
	bool bLocalUse = false;
	float Accumulated = 1.f;
};
