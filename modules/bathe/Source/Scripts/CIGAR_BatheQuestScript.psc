ScriptName CIGAR_BatheQuestScript extends CIGAR_ModuleBase
{CIGAR Bathe. Offers bathing (in water, nothing strippable worn) and showering
(under a waterfall) and runs Bathing in Skyrim - Renewed's own wash: dirt reset,
soap, followers, BiS animations and restrictions. Replaces Streamlined
Interactions' Bathe module, which only plays an animation.}

int Property EventBathe = 0 AutoReadOnly
int Property EventShower = 1 AutoReadOnly
int Property KeyBathe = 0x02 AutoReadOnly
int Property KeyShower = 0x03 AutoReadOnly

mzinBatheQuest bis = None
bool batheOffered = false
bool showerOffered = false
bool wasInWater = false
bool warnedBisOff = false

string Function ModuleName()
    return "Bathe"
EndFunction

Function ModuleReset()
    batheOffered = false
    showerOffered = false
    wasInWater = false
    warnedBisOff = false
EndFunction

string Function ModuleStartup()
    bis = Quest.GetQuest("mzinBatheQuest") as mzinBatheQuest
    if !bis
        return "Bathing in Skyrim - Renewed not found (mzinBatheQuest)"
    endif
    return ""
EndFunction

string Function ModuleStatus()
    return " BiS=" + mzinAPI.GetModVersion() + " BiSEnabled=" + mzinAPI.GetModState() + " SIBathe=" + SISetting(".MCP.modules.Bathe.enabled")
EndFunction

Function Tick()
    ; While BiS is animating the player, leave the offer state alone so the
    ; prompt does not come back the moment the wash finishes.
    if bis.IsActorAnimating(PlayerRef)
        return
    endif

    bool inWater = PO3_SKSEFunctions.IsActorInWater(PlayerRef)
    bool bisOn = mzinAPI.GetModState() > 0.0
    bool busy = CIGAR_Util.IsBusy(PlayerRef)
    int strippable = 0
    if inWater && !busy
        strippable = CIGAR_Util.CountStrippable(PlayerRef)
    endif
    ; With BiS's water restriction off, IsUnderWaterfall() is true everywhere,
    ; so only a real waterfall check is worth a separate prompt.
    bool underFall = false
    if inWater && strippable == 0 && !busy && bis.WaterRestrictionEnabled.GetValue() != 0.0
        underFall = bis.IsUnderWaterfall(PlayerRef)
    endif
    bool canBathe = inWater && !busy && strippable == 0 && bisOn
    bool canShower = underFall && bisOn

    LogGate("water=" + inWater + " waterfall=" + underFall + " bis=" + bisOn + " strippable=" + strippable + " busy=" + busy)
    if inWater && !wasInWater
        OnEnterWater(bisOn)
    endif
    wasInWater = inWater

    string text = ""
    if canBathe && !batheOffered
        text = "@CIGAR:bathe@" + DirtText()
    endif
    batheOffered = UpdatePrompt(canBathe, batheOffered, EventBathe, KeyBathe, text)
    text = ""
    if canShower && !showerOffered
        text = "@CIGAR:shower@" + DirtText()
    endif
    showerOffered = UpdatePrompt(canShower, showerOffered, EventShower, KeyShower, text)
EndFunction

string Function DirtText()
    return " (" + Math.Floor(bis.DirtinessPercentage.GetValue() * 100.0) + "%)"
EndFunction

Function OnEnterWater(bool bisOn)
    if !bisOn && !warnedBisOff
        warnedBisOff = true
        Log("WARN Bathing in Skyrim is disabled in its MCM; no bathe prompt is offered")
        Debug.Notification("@CIGAR:bisoff@")
    endif
    WarnIfSIModuleOn("Bathe", ".MCP.modules.Bathe.enabled")
EndFunction

Function OnPromptAccepted(int eventID)
    if eventID != EventBathe && eventID != EventShower
        return
    endif
    Withdraw(EventBathe)
    Withdraw(EventShower)
    bool shower = eventID == EventShower
    bool washed = bis.TryWashActor(PlayerRef, None, shower, true)
    Log("wash shower=" + shower + " result=" + washed)
EndFunction
