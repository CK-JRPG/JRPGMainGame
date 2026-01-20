#include "CombatPlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "JRPG/Combat/Battle/BattleSessionSubsystem.h"
#include "JRPG/Combat/Tactical/TacticalModeSubsystem.h"
#include "JRPG/Combat/Chain/TrinityChainSubsystem.h"
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "JRPG/Combat/Skill/SkillComponent.h"

// #include "UI/CombatUIManagerSubsystem.h"
// #include "UI/TacticalHUDWidget.h"
// #include "UI/ChainHUDWidget.h"

void ACombatPlayerController::BeginPlay()
{
    Super::BeginPlay();
    EnsureUI();
}

void ACombatPlayerController::EnsureUI()
{
    /*if (UCombatUIManagerSubsystem* UI = GetWorld()->GetSubsystem<UCombatUIManagerSubsystem>())
    {
        UI->InitializeUI();
        ApplySelectedIndexToWidgets();
    }*/
}

void ACombatPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ACombatPlayerController::ToggleTactical);

    InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ACombatPlayerController::SelectParty1);
    InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ACombatPlayerController::SelectParty2);
    InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ACombatPlayerController::SelectParty3);

    InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &ACombatPlayerController::ReserveSlot1);
    InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &ACombatPlayerController::ReserveSlot2);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ACombatPlayerController::ReserveSlot3);
    InputComponent->BindKey(EKeys::Four,  IE_Pressed, this, &ACombatPlayerController::ReserveSlot4);

    InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &ACombatPlayerController::ClearReservation);

    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACombatPlayerController::StartChain);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACombatPlayerController::ConfirmChain);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACombatPlayerController::CancelChain);
    
    InputComponent->BindAxis("MoveForward", this, &ACombatPlayerController::MoveForward);
    InputComponent->BindAxis("MoveRight", this, &ACombatPlayerController::MoveRight);
    
    InputComponent->BindAxis("Turn", this, &APlayerController::AddYawInput);
    InputComponent->BindAxis("LookUp", this, &APlayerController::AddPitchInput);
}

TArray<AActor*> ACombatPlayerController::GetParty() const
{
    if (!GetWorld()) return {};
    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
         return Battle->GetPartyRaw();
    return {};
}

AActor* ACombatPlayerController::GetPartyMember(int32 Index) const
{
    const TArray<AActor*> Party = GetParty();
    return Party.IsValidIndex(Index) ? Party[Index] : nullptr;
}

AActor* ACombatPlayerController::GetCurrentChainTarget() const
{
    if (!GetWorld()) return nullptr;

    if (UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
    {
        if (Chain->GetState() != EChainState::None)
            return Chain->GetChainTarget();
    }

    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
        return Battle->GetMainEnemy();

    return nullptr;
}

void ACombatPlayerController::ApplySelectedIndexToWidgets() const
{
    if (!GetWorld()) return;

    /*if (UCombatUIManagerSubsystem* UI = GetWorld()->GetSubsystem<UCombatUIManagerSubsystem>())
    {
        if (UTacticalHUDWidget* TacW = UI->GetTacticalHUD())
            TacW->SetSelectedPartyIndex(SelectedPartyIndex);

        if (UChainHUDWidget* ChainW = UI->GetChainHUD())
            ChainW->SetSelectedPartyIndex(SelectedPartyIndex);
    }*/
}

void ACombatPlayerController::SelectParty1(){ SelectedPartyIndex = 0; ApplySelectedIndexToWidgets(); }
void ACombatPlayerController::SelectParty2(){ SelectedPartyIndex = 1; ApplySelectedIndexToWidgets(); }
void ACombatPlayerController::SelectParty3(){ SelectedPartyIndex = 2; ApplySelectedIndexToWidgets(); }

void ACombatPlayerController::ToggleTactical()
{
    EnsureUI();

    if (!GetWorld()) return;
    if (UTacticalModeSubsystem* Tac = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        if (Tac->IsTacticalActive()) Tac->ExitTactical();
        else Tac->EnterTactical(5.0f, 0.15f);
    }
}

void ACombatPlayerController::ReserveSlot1(){ ReserveSlot(1); }
void ACombatPlayerController::ReserveSlot2(){ ReserveSlot(2); }
void ACombatPlayerController::ReserveSlot3(){ ReserveSlot(3); }
void ACombatPlayerController::ReserveSlot4(){ ReserveSlot(4); }

void ACombatPlayerController::ReserveSlot(int32 Slot)
{
    if (!GetWorld()) return;

    // 체인 선택 중이면 체인 슬롯 선택
    if (UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
    {
        if (Chain->GetState() == EChainState::Selecting)
        {
            PickChainSlot(Slot);
            return;
        }
    }

    // 전술 예약
    if (UTacticalModeSubsystem* Tac = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        if (!Tac->IsTacticalActive()) return;

        AActor* Member = GetPartyMember(SelectedPartyIndex);
        if (!Member) return;

        if (USkillComponent* Skills = Member->FindComponentByClass<USkillComponent>())
        {
            if (UCombatSkill* Skill = Skills->GetSkillBySlot(Slot))
                Tac->ReserveSkillById(Member, Skill->SkillId);
        }
    }
}

void ACombatPlayerController::PickChainSlot(int32 Slot)
{
    if (!GetWorld()) return;

    UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>();
    if (!Chain || Chain->GetState() != EChainState::Selecting) return;

    AActor* Member = GetPartyMember(SelectedPartyIndex);
    if (!Member) return;

    if (USkillComponent* Skills = Member->FindComponentByClass<USkillComponent>())
    {
        if (UCombatSkill* Skill = Skills->GetSkillBySlot(Slot))
            Chain->SelectSkillFor(Member, Skill->SkillId);
    }
}

void ACombatPlayerController::MoveForward(float Value)
{
    if (Value == 0.0f && GetPawn()) return;
    
    const FRotator Rot = GetControlRotation();
    const FRotator YawRotation = FRotator(0, Rot.Yaw, 0);
    
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    
    GetPawn()->AddMovementInput(Direction, Value);
}

void ACombatPlayerController::MoveRight(float Value)
{
    if (Value == 0.0f && GetPawn()) return;
    
    const FRotator Rot = GetControlRotation();
    const FRotator YawRotation = FRotator(0, Rot.Yaw, 0);
    
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    
    GetPawn()->AddMovementInput(Direction, Value);
}

void ACombatPlayerController::ClearReservation()
{
    if (!GetWorld()) return;
    if (UTacticalModeSubsystem* Tac = GetWorld()->GetSubsystem<UTacticalModeSubsystem>())
    {
        AActor* Member = GetPartyMember(SelectedPartyIndex);
        if (Member) Tac->ClearReservation(Member);
    }
}

void ACombatPlayerController::StartChain()
{
    EnsureUI();
    if (!GetWorld()) return;

    if (UBattleSessionSubsystem* Battle = GetWorld()->GetSubsystem<UBattleSessionSubsystem>())
        if (!Battle->IsInBattle()) return;

    if (UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
    {
        AActor* Target = GetCurrentChainTarget();
        if (!Target) return;

        Chain->StartChain(Target, false);
        ApplySelectedIndexToWidgets();
    }
}

void ACombatPlayerController::ConfirmChain()
{
    if (!GetWorld()) return;
    if (UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
        if (Chain->GetState() == EChainState::Selecting) Chain->ConfirmAndExecute();
}

void ACombatPlayerController::CancelChain()
{
    if (!GetWorld()) return;
    if (UTrinityChainSubsystem* Chain = GetWorld()->GetSubsystem<UTrinityChainSubsystem>())
        if (Chain->GetState() != EChainState::None) Chain->CancelChain();
}
