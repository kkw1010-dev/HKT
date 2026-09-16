ScriptName SIX_BatheQuestScript extends Quest
{SI-Extensions / Bathe. Replaces Streamlined Interactions' Water Undress and Bathe
prompts. Undress skips items that must stay worn (non-playable, no-strip or
locked), dress restores what undress removed, and bathe runs Bathing in Skyrim -
Renewed's real wash (dirt reset, soap, followers, BiS animations and restrictions).
Player-facing text is ASCII placeholders (see strings.ko.json) patched to UTF-8 Korean
after compiling, because the CK compiler reads sources in the system code page.
Each prompt is offered once per water entry or exit; leaving that condition re-arms it.}

Actor Property PlayerRef Auto

string Property LogPath = "Data/SKSE/Plugins/SI-Extensions/SIX_Bathe.log" AutoReadOnly
; JsonUtil paths are relative to Data/SKSE/Plugins/StorageUtilData.
string Property SISettingsFile = "../StreamlinedInteractions/settings.json" AutoReadOnly
float Property TickSeconds = 1.0 AutoReadOnly
float Property RetrySeconds = 10.0 AutoReadOnly

int Property EventBathe = 0 AutoReadOnly
int Property EventShower = 1 AutoReadOnly
int Property EventUndress = 2 AutoReadOnly
int Property EventDress = 3 AutoReadOnly
; Bathe, undress and dress are never offered together, so they share key 1.
int Property KeyPrimary = 0x02 AutoReadOnly
int Property KeyShower = 0x03 AutoReadOnly

int clientID = 0
mzinBatheQuest bis = None
bool ready = false
bool batheOffered = false
bool showerOffered = false
bool undressOffered = false
bool dressOffered = false
int startupFailures = 0
string lastGate = ""
bool wasInWater = false
bool warnedBisOff = false
bool warnedSIModule = false

; Strippable items found by the last ScanStrippable().
Form[] scanned
int scannedCount = 0
; Items removed by the undress prompt, kept across saves until dress restores them.
Form[] removed
int removedCount = 0

Event OnInit()
    Startup("init")
EndEvent

; Called by SIX_BathePlayerAlias on every save load: SkyPrompt client IDs are
; native session state and do not survive a load.
Function Startup(string reason)
    ready = false
    batheOffered = false
    showerOffered = false
    undressOffered = false
    dressOffered = false
    lastGate = ""
    wasInWater = false
    warnedBisOff = false
    warnedSIModule = false
    UnregisterForUpdate()
    if !scanned
        scanned = new Form[32]
    endif
    if !removed
        removed = new Form[32]
    endif

    bis = Quest.GetQuest("mzinBatheQuest") as mzinBatheQuest
    if !bis
        Fail(reason, "Bathing in Skyrim - Renewed not found (mzinBatheQuest)")
        return
    endif

    clientID = SkyPrompt.RegisterForSkyPromptEvent(self, 2, 0)
    if clientID == 0
        Fail(reason, "SkyPrompt registration returned client 0 (SkyPrompt missing or API mismatch)")
        return
    endif

    ready = true
    startupFailures = 0
    Log("READY (" + reason + ") client=" + clientID + " BiS=" + mzinAPI.GetModVersion() + " BiSEnabled=" + mzinAPI.GetModState() + " SIBathe=" + SISetting(".MCP.modules.Bathe.enabled") + " SIWaterUndress=" + SISetting(".MCP.modules.DressActions.enabled_water") + " pendingDress=" + removedCount)
    if reason == "init"
        Debug.Notification("@SIX:ready@")
    endif
    RegisterForSingleUpdate(TickSeconds)
EndFunction

Function Fail(string reason, string why)
    startupFailures += 1
    Log("FAILED (" + reason + ", attempt " + startupFailures + "): " + why)
    if startupFailures == 1
        Debug.Notification("@SIX:fail@" + why)
    endif
    if startupFailures < 6
        RegisterForSingleUpdate(RetrySeconds)
    endif
EndFunction

Event OnUpdate()
    if !ready
        Startup("retry")
        return
    endif
    Tick()
    RegisterForSingleUpdate(TickSeconds)
EndEvent

Function Tick()
    ; While BiS is animating the player, leave the offer state alone so the
    ; prompt does not come back the moment the wash finishes.
    if bis.IsActorAnimating(PlayerRef)
        return
    endif

    bool inWater = PO3_SKSEFunctions.IsActorInWater(PlayerRef)
    bool bisOn = mzinAPI.GetModState() > 0.0
    bool busy = PlayerRef.IsInCombat() || PlayerRef.IsOnMount()
    int strippable = 0
    if inWater && !busy
        strippable = ScanStrippable()
    endif
    ; With BiS's water restriction off, IsUnderWaterfall() is true everywhere,
    ; so only a real waterfall check is worth a separate prompt.
    bool underFall = false
    if inWater && strippable == 0 && !busy && bis.WaterRestrictionEnabled.GetValue() != 0.0
        underFall = bis.IsUnderWaterfall(PlayerRef)
    endif

    bool canUndress = inWater && !busy && strippable > 0
    bool canBathe = inWater && !busy && strippable == 0 && bisOn
    bool canShower = underFall && bisOn
    bool canDress = !inWater && !busy && removedCount > 0

    ; Log every change of the gate inputs so a missing prompt is explained by the log.
    string gate = "water=" + inWater + " waterfall=" + underFall + " bis=" + bisOn + " strippable=" + strippable + " busy=" + busy + " pendingDress=" + removedCount
    if gate != lastGate
        lastGate = gate
        Log("gate " + gate)
    endif
    if inWater && !wasInWater
        OnEnterWater(bisOn)
    endif
    wasInWater = inWater

    undressOffered = UpdatePrompt(canUndress, undressOffered, EventUndress, KeyPrimary, "@SIX:undress@")
    batheOffered = UpdatePrompt(canBathe, batheOffered, EventBathe, KeyPrimary, "@SIX:bathe@")
    showerOffered = UpdatePrompt(canShower, showerOffered, EventShower, KeyShower, "@SIX:shower@")
    dressOffered = UpdatePrompt(canDress, dressOffered, EventDress, KeyPrimary, "@SIX:dress@")
EndFunction

; Offers the prompt when its condition starts holding and withdraws it when the
; condition ends. Returns the new offered state.
bool Function UpdatePrompt(bool can, bool offered, int eventID, int keyCode, string label)
    if can && !offered
        Offer(eventID, keyCode, label)
        return true
    elseif !can && offered
        SkyPrompt.RemovePrompt(clientID, eventID, 0)
        return false
    endif
    return offered
EndFunction

; Fills scanned[] with worn items undress may remove and returns their count.
int Function ScanStrippable()
    ; Clear the previous result first so Find() only sees this scan.
    while scannedCount > 0
        scannedCount -= 1
        scanned[scannedCount] = None
    endwhile
    int slot = 30
    while slot < 62
        Form item = PlayerRef.GetWornForm(Armor.GetMaskForSlot(slot))
        if item && IsStrippable(item, slot) && scanned.Find(item) < 0
            scanned[scannedCount] = item
            scannedCount += 1
        endif
        slot += 1
    endwhile
    return scannedCount
EndFunction

; Hair, ears, tail and decapitation slots belong to the body. Non-playable items
; (the Softbody SMP collision carrier HDTSMPObjectBase in slot 60), no-strip items
; and locked devices must stay on.
bool Function IsStrippable(Form item, int slot)
    if slot == 31 || slot == 40 || slot == 41 || slot == 43 || slot == 50 || slot == 51
        return false
    endif
    if !item.IsPlayable()
        return false
    endif
    if item.HasKeywordString("SexLabNoStrip") || item.HasKeywordString("OStimNoStrip") || item.HasKeywordString("zad_Lockable") || item.HasKeywordString("zad_QuestItem")
        return false
    endif
    return true
EndFunction

Function Undress()
    ScanStrippable()
    string names = ""
    int i = 0
    while i < scannedCount
        Form item = scanned[i]
        PlayerRef.UnequipItem(item, false, true)
        if removed.Find(item) < 0 && removedCount < removed.Length
            removed[removedCount] = item
            removedCount += 1
        endif
        names += " " + item.GetName()
        i += 1
    endwhile
    Log("undress removed" + names + " pendingDress=" + removedCount)
EndFunction

Function Dress()
    string names = ""
    int i = 0
    while i < removedCount
        Form item = removed[i]
        if item && PlayerRef.GetItemCount(item) > 0
            PlayerRef.EquipItem(item, false, true)
            names += " " + item.GetName()
        endif
        removed[i] = None
        i += 1
    endwhile
    removedCount = 0
    Log("dress equipped" + names)
EndFunction

; Once per water entry: record what is worn and held, and surface the
; configuration states that silently break this module.
Function OnEnterWater(bool bisOn)
    string worn = ""
    int slot = 30
    while slot < 62
        Form item = PlayerRef.GetWornForm(Armor.GetMaskForSlot(slot))
        if item
            worn += " " + slot + ":" + item.GetName()
            if !IsStrippable(item, slot)
                worn += "(kept)"
            endif
        endif
        slot += 1
    endwhile
    Log("entered water; worn:" + worn + " | left=" + NameOf(PlayerRef.GetEquippedObject(0)) + " right=" + NameOf(PlayerRef.GetEquippedObject(1)))

    if !bisOn && !warnedBisOff
        warnedBisOff = true
        Log("WARN Bathing in Skyrim is disabled in its MCM; no bathe prompt is offered")
        Debug.Notification("@SIX:bisoff@")
    endif

    WarnIfSIModuleOn("Bathe", ".MCP.modules.Bathe.enabled")
    WarnIfSIModuleOn("Water Undress", ".MCP.modules.DressActions.enabled_water")
EndFunction

Function WarnIfSIModuleOn(string label, string path)
    int value = SISetting(path)
    if value == 1 && !warnedSIModule
        warnedSIModule = true
        Log("WARN Streamlined Interactions " + label + " is on; its prompt will duplicate this module")
        Debug.Notification("@SIX:sion@" + label)
    elseif value < 0
        Log("WARN could not read " + path + " from Streamlined Interactions settings.json")
    endif
EndFunction

string Function NameOf(Form item)
    if item
        return item.GetName()
    endif
    return "-"
EndFunction

; SI rewrites its settings.json from its menu (presets re-apply module switches),
; so read the file as it is now. Returns 1 on, 0 off, -1 unreadable.
int Function SISetting(string path)
    JsonUtil.Unload(SISettingsFile, false)
    if !JsonUtil.JsonExists(SISettingsFile)
        return -1
    endif
    int value = JsonUtil.GetPathIntValue(SISettingsFile, path, -1)
    JsonUtil.Unload(SISettingsFile, false)
    if value > 0
        return 1
    endif
    return value
EndFunction

Function Offer(int eventID, int keyCode, string label)
    string text = label
    if eventID == EventBathe || eventID == EventShower
        text = label + " (" + Math.Floor(bis.DirtinessPercentage.GetValue() * 100.0) + "%)"
    endif
    int[] devices = new int[1]
    int[] keyCodes = new int[1]
    devices[0] = 0
    keyCodes[0] = keyCode
    bool sent = SkyPrompt.SendPrompt(clientID, text, eventID, 0, 0, PlayerRef, devices, keyCodes, 0.0)
    Log("offer event=" + eventID + " sent=" + sent)
EndFunction

Event OnSkyPromptEvent(int eventClientID, int eventType, int eventID, int actionID, float dx, float dy, float progress)
    if eventClientID != clientID
        return
    endif
    Log("prompt event type=" + eventType + " event=" + eventID + " action=" + actionID)
    ; 0 = accepted; 5 = shown, 3/4 = expired (observed).
    if eventType != 0
        return
    endif
    SkyPrompt.RemovePrompt(clientID, eventID, 0)
    if eventID == EventBathe || eventID == EventShower
        SkyPrompt.RemovePrompt(clientID, EventBathe, 0)
        SkyPrompt.RemovePrompt(clientID, EventShower, 0)
        bool shower = eventID == EventShower
        bool washed = bis.TryWashActor(PlayerRef, None, shower, true)
        Log("wash shower=" + shower + " result=" + washed)
    elseif eventID == EventUndress
        Undress()
    elseif eventID == EventDress
        Dress()
    endif
EndEvent

Function Log(string msg)
    Debug.Trace("[SI-Extensions] " + msg)
    MiscUtil.WriteToFile(LogPath, msg + StringUtil.AsChar(10), true, true)
EndFunction
