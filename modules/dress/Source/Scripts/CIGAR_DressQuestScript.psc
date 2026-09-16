ScriptName CIGAR_DressQuestScript extends CIGAR_ModuleBase
{CIGAR Dress. Offers undressing in water, at a bed or at a wardrobe/dresser, and
dressing again once the player has left that place. Undress skips items that
must stay worn (see CIGAR_Util.IsStrippable). Replaces Streamlined Interactions'
Water, Bed and Wardrobe Undress, which strip the Softbody SMP collision carrier.}

int Property EventUndress = 0 AutoReadOnly
int Property EventDress = 1 AutoReadOnly
int Property KeyPrimary = 0x02 AutoReadOnly
; How close the player must stay to the bed or wardrobe last aimed at.
float Property PlaceRange = 250.0 AutoReadOnly
; po3 GetFurnitureType: 0 perch, 1 lean, 2 sit, 3 sleep.
int Property FurnitureSleep = 3 AutoReadOnly

; Items removed by undress, kept across saves until dress restores them.
Form[] removed
int removedCount = 0

bool undressOffered = false
bool dressOffered = false
ObjectReference placeRef = None
string placeKind = ""
string lastContext = ""
ObjectReference lastLoggedFurniture = None

string Function ModuleName()
    return "Dress"
EndFunction

Function ModuleReset()
    undressOffered = false
    dressOffered = false
    placeRef = None
    placeKind = ""
    lastContext = ""
    lastLoggedFurniture = None
EndFunction

string Function ModuleStartup()
    if !removed
        removed = new Form[32]
    endif
    RegisterForCrosshairRef()
    return ""
EndFunction

string Function ModuleStatus()
    return " SIWater=" + SISetting(".MCP.modules.DressActions.enabled_water") + " SIBed=" + SISetting(".MCP.modules.DressActions.enabled_bed") + " SIWardrobe=" + SISetting(".MCP.modules.DressActions.enabled_wardrobe") + " pendingDress=" + removedCount
EndFunction

; ---------------------------------------------------------------------------
; Place detection

Event OnCrosshairRefChange(ObjectReference ref)
    if !IsReady() || !ref
        return
    endif
    string kind = PlaceKind(ref)
    if kind != "" && ref != placeRef
        placeRef = ref
        placeKind = kind
        Log("aimed at " + kind + ": " + CIGAR_Util.NameOf(ref.GetBaseObject()))
    endif
EndEvent

string Function PlaceKind(ObjectReference ref)
    Form base = ref.GetBaseObject()
    Furniture furn = base as Furniture
    if furn
        int furnType = PO3_SKSEFunctions.GetFurnitureType(furn)
        if ref != lastLoggedFurniture
            lastLoggedFurniture = ref
            Log("furniture " + CIGAR_Util.NameOf(base) + " type=" + furnType)
        endif
        if furnType == FurnitureSleep
            return "bed"
        endif
        return ""
    endif
    if base as Container
        string id = PO3_SKSEFunctions.GetFormEditorID(base)
        string model = base.GetWorldModelPath()
        if HasWord(id) || HasWord(model)
            return "wardrobe"
        endif
    endif
    return ""
EndFunction

bool Function HasWord(string s)
    return StringUtil.Find(s, "Wardrobe") >= 0 || StringUtil.Find(s, "wardrobe") >= 0 || StringUtil.Find(s, "Dresser") >= 0 || StringUtil.Find(s, "dresser") >= 0
EndFunction

; "water", "bed", "wardrobe" or "".
string Function CurrentContext()
    if PO3_SKSEFunctions.IsActorInWater(PlayerRef)
        return "water"
    endif
    if placeRef
        if placeRef.GetParentCell() == PlayerRef.GetParentCell() && PlayerRef.GetDistance(placeRef) <= PlaceRange
            return placeKind
        endif
        placeRef = None
        placeKind = ""
    endif
    return ""
EndFunction

; ---------------------------------------------------------------------------
; Tick

Function Tick()
    string context = CurrentContext()
    bool busy = CIGAR_Util.IsBusy(PlayerRef)
    int strippable = 0
    if (context != "" || removedCount > 0) && !busy
        strippable = CIGAR_Util.CountStrippable(PlayerRef)
    endif

    ; Dressed again by hand after leaving: the remembered set is stale.
    if context == "" && removedCount > 0 && strippable > 0 && !dressOffered
        Log("player dressed without the prompt; forgetting " + removedCount + " item(s)")
        ClearRemoved()
    endif

    bool canUndress = context != "" && !busy && strippable > 0
    bool canDress = context == "" && !busy && removedCount > 0 && strippable == 0

    LogGate("context=" + context + " strippable=" + strippable + " busy=" + busy + " pendingDress=" + removedCount)
    if context != lastContext
        if context != ""
            OnEnterContext(context)
        endif
        lastContext = context
    endif

    undressOffered = UpdatePrompt(canUndress, undressOffered, EventUndress, KeyPrimary, "@CIGAR:undress@")
    dressOffered = UpdatePrompt(canDress, dressOffered, EventDress, KeyPrimary, "@CIGAR:dress@")
EndFunction

Function OnEnterContext(string context)
    Log("entered " + context + "; worn:" + CIGAR_Util.DescribeWorn(PlayerRef))
    if context == "water"
        WarnIfSIModuleOn("Water Undress", ".MCP.modules.DressActions.enabled_water")
    elseif context == "bed"
        WarnIfSIModuleOn("Bed Undress", ".MCP.modules.DressActions.enabled_bed")
    else
        WarnIfSIModuleOn("Wardrobe Undress", ".MCP.modules.DressActions.enabled_wardrobe")
    endif
EndFunction

; ---------------------------------------------------------------------------
; Actions

Function OnPromptAccepted(int eventID)
    if eventID == EventUndress
        Undress()
    elseif eventID == EventDress
        Dress()
    endif
EndFunction

Function Undress()
    Form[] items = CIGAR_Util.GetStrippable(PlayerRef)
    string names = ""
    int i = 0
    while i < items.Length && items[i]
        Form item = items[i]
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
        i += 1
    endwhile
    ClearRemoved()
    Log("dress equipped" + names)
EndFunction

Function ClearRemoved()
    while removedCount > 0
        removedCount -= 1
        removed[removedCount] = None
    endwhile
EndFunction
