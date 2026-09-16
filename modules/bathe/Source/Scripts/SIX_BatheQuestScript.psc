ScriptName SIX_BatheQuestScript extends Quest
{SI-Extensions / Bathe. Replaces Streamlined Interactions' animation-only Bathe
prompt with a SkyPrompt prompt that runs Bathing in Skyrim - Renewed's real wash
(dirt reset, soap, followers, BiS animation selection and restrictions).
Player-facing text is ASCII placeholders (see strings.ko.json) patched to UTF-8 Korean
after compiling, because the CK compiler reads sources in the system code page.
A prompt is offered once per water entry; leaving the water, dressing, combat or
mounting re-arms it.}

Actor Property PlayerRef Auto

string Property LogPath = "Data/SKSE/Plugins/SI-Extensions/SIX_Bathe.log" AutoReadOnly
float Property TickSeconds = 1.0 AutoReadOnly
float Property RetrySeconds = 10.0 AutoReadOnly
int Property BodySlotMask = 0x00000004 AutoReadOnly

int Property EventBathe = 0 AutoReadOnly
int Property EventShower = 1 AutoReadOnly
int Property KeyBathe = 0x02 AutoReadOnly
int Property KeyShower = 0x03 AutoReadOnly

int clientID = 0
mzinBatheQuest bis = None
bool ready = false
bool batheOffered = false
bool showerOffered = false
int startupFailures = 0

Event OnInit()
    Startup("init")
EndEvent

; Called by SIX_BathePlayerAlias on every save load: SkyPrompt client IDs are
; native session state and do not survive a load.
Function Startup(string reason)
    ready = false
    batheOffered = false
    showerOffered = false
    UnregisterForUpdate()

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
    Log("READY (" + reason + ") client=" + clientID + " BiS=" + mzinAPI.GetModVersion() + " BiSEnabled=" + mzinAPI.GetModState())
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

    bool canBathe = false
    bool canShower = false
    if mzinAPI.GetModState() > 0.0 && !PlayerRef.IsInCombat() && !PlayerRef.IsOnMount() && !PlayerRef.GetWornForm(BodySlotMask)
        canBathe = PO3_SKSEFunctions.IsActorInWater(PlayerRef)
        ; With BiS's water restriction off, IsUnderWaterfall() is true everywhere,
        ; so only a real waterfall check is worth a separate prompt.
        if bis.WaterRestrictionEnabled.GetValue() != 0.0
            canShower = bis.IsUnderWaterfall(PlayerRef)
        endif
    endif

    if canBathe && !batheOffered
        batheOffered = true
        Offer(EventBathe, KeyBathe, "@SIX:bathe@")
    elseif !canBathe && batheOffered
        batheOffered = false
        SkyPrompt.RemovePrompt(clientID, EventBathe, 0)
    endif

    if canShower && !showerOffered
        showerOffered = true
        Offer(EventShower, KeyShower, "@SIX:shower@")
    elseif !canShower && showerOffered
        showerOffered = false
        SkyPrompt.RemovePrompt(clientID, EventShower, 0)
    endif
EndFunction

Function Offer(int eventID, int keyCode, string label)
    int dirt = Math.Floor(bis.DirtinessPercentage.GetValue() * 100.0)
    int[] devices = new int[1]
    int[] keyCodes = new int[1]
    devices[0] = 0
    keyCodes[0] = keyCode
    bool sent = SkyPrompt.SendPrompt(clientID, label + " (" + dirt + "%)", eventID, 0, 0, PlayerRef, devices, keyCodes, 0.0)
    Log("offer event=" + eventID + " dirt=" + dirt + " sent=" + sent)
EndFunction

Event OnSkyPromptEvent(int eventClientID, int eventType, int eventID, int actionID, float dx, float dy, float progress)
    if eventClientID != clientID
        return
    endif
    Log("prompt event type=" + eventType + " event=" + eventID + " action=" + actionID)
    if eventType != 0
        return
    endif
    if eventID == EventBathe || eventID == EventShower
        SkyPrompt.RemovePrompt(clientID, EventBathe, 0)
        SkyPrompt.RemovePrompt(clientID, EventShower, 0)
        bool shower = eventID == EventShower
        bool washed = bis.TryWashActor(PlayerRef, None, shower, true)
        Log("wash shower=" + shower + " result=" + washed)
    endif
EndEvent

Function Log(string msg)
    Debug.Trace("[SI-Extensions] " + msg)
    MiscUtil.WriteToFile(LogPath, msg + StringUtil.AsChar(10), true, true)
EndFunction
