ScriptName CIGAR_ModuleBase extends Quest
{CIGAR (Contextual Interaction, Gameplay Acceleration and Rhythm) module base.
Each module is its own quest whose script extends this one. The base owns the
SkyPrompt client, the once-a-second tick, prompt bookkeeping, logging and the
Streamlined Interactions settings check. Player-facing text is ASCII
placeholders patched to UTF-8 Korean after compiling (see strings.ko.json),
because the CK compiler reads sources in the system code page.}

Actor Property PlayerRef Auto

float Property TickSeconds = 1.0 AutoReadOnly
float Property RetrySeconds = 10.0 AutoReadOnly
string Property LogPath = "Data/SKSE/Plugins/CIGAR/CIGAR.log" AutoReadOnly
; JsonUtil paths are relative to Data/SKSE/Plugins/StorageUtilData.
string Property SISettingsFile = "../StreamlinedInteractions/settings.json" AutoReadOnly

int clientID = 0
bool ready = false
int startupFailures = 0
string lastGate = ""
bool warnedSIModule = false

; ---------------------------------------------------------------------------
; Module hooks, overridden by each module script.

string Function ModuleName()
    return "Module"
EndFunction

; Returns "" when the module can run, otherwise the reason it cannot.
string Function ModuleStartup()
    return ""
EndFunction

; Extra READY-line details.
string Function ModuleStatus()
    return ""
EndFunction

; Clears per-session state before every startup.
Function ModuleReset()
EndFunction

Function Tick()
EndFunction

Function OnPromptAccepted(int eventID)
EndFunction

; ---------------------------------------------------------------------------
; Lifecycle

Event OnInit()
    Startup("init")
EndEvent

; Also called by CIGAR_PlayerAlias on every save load: SkyPrompt client IDs are
; native session state and do not survive a load.
Function Startup(string reason)
    ready = false
    lastGate = ""
    warnedSIModule = false
    UnregisterForUpdate()
    ModuleReset()

    string why = ModuleStartup()
    if why != ""
        Fail(reason, why)
        return
    endif

    clientID = SkyPrompt.RegisterForSkyPromptEvent(self, 2, 0)
    if clientID == 0
        Fail(reason, "SkyPrompt registration returned client 0 (SkyPrompt missing or API mismatch)")
        return
    endif

    ready = true
    startupFailures = 0
    Log("READY (" + reason + ") client=" + clientID + ModuleStatus())
    if reason == "init"
        Debug.Notification("@CIGAR:ready@" + ModuleName())
    endif
    RegisterForSingleUpdate(TickSeconds)
EndFunction

Function Fail(string reason, string why)
    startupFailures += 1
    Log("FAILED (" + reason + ", attempt " + startupFailures + "): " + why)
    if startupFailures == 1
        Debug.Notification("@CIGAR:fail@" + ModuleName() + " - " + why)
    endif
    if startupFailures < 6
        RegisterForSingleUpdate(RetrySeconds)
    endif
EndFunction

bool Function IsReady()
    return ready
EndFunction

Event OnUpdate()
    if !ready
        Startup("retry")
        return
    endif
    Tick()
    RegisterForSingleUpdate(TickSeconds)
EndEvent

; ---------------------------------------------------------------------------
; Prompts

; Offers the prompt when its condition starts holding and withdraws it when the
; condition ends. text is only read when the prompt is offered. Returns the new
; offered state.
bool Function UpdatePrompt(bool can, bool offered, int eventID, int keyCode, string text)
    if can && !offered
        Offer(eventID, keyCode, text)
        return true
    elseif !can && offered
        Withdraw(eventID)
        return false
    endif
    return offered
EndFunction

Function Offer(int eventID, int keyCode, string text)
    int[] devices = new int[1]
    int[] keyCodes = new int[1]
    devices[0] = 0
    keyCodes[0] = keyCode
    bool sent = SkyPrompt.SendPrompt(clientID, text, eventID, 0, 0, PlayerRef, devices, keyCodes, 0.0)
    Log("offer event=" + eventID + " sent=" + sent)
EndFunction

Function Withdraw(int eventID)
    SkyPrompt.RemovePrompt(clientID, eventID, 0)
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
    Withdraw(eventID)
    OnPromptAccepted(eventID)
EndEvent

; ---------------------------------------------------------------------------
; Diagnostics

; Logs the gate inputs whenever they change, so a missing prompt is explained by the log.
Function LogGate(string gate)
    if gate != lastGate
        lastGate = gate
        Log("gate " + gate)
    endif
EndFunction

Function Log(string msg)
    string line = "[" + ModuleName() + "] " + msg
    Debug.Trace("[CIGAR]" + line)
    MiscUtil.WriteToFile(LogPath, line + StringUtil.AsChar(10), true, true)
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

; Warns once per session when an SI module that CIGAR replaces is switched on again.
Function WarnIfSIModuleOn(string label, string path)
    int value = SISetting(path)
    if value == 1 && !warnedSIModule
        warnedSIModule = true
        Log("WARN Streamlined Interactions " + label + " is on; its prompt will duplicate CIGAR's")
        Debug.Notification("@CIGAR:sion@" + label)
    elseif value < 0
        Log("WARN could not read " + path + " from Streamlined Interactions settings.json")
    endif
EndFunction
