ScriptName CIGAR_Util Hidden
{CIGAR shared helpers.}

; Hair, tail, ears and decapitation slots belong to the body. Non-playable items
; (such as the Softbody SMP collision carrier HDTSMPObjectBase in slot 60),
; no-strip items and locked devices must stay on.
bool Function IsStrippable(Form item, int slot) Global
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

; Worn items undress may remove, each once, padded with None.
Form[] Function GetStrippable(Actor akActor) Global
    Form[] found = new Form[32]
    int count = 0
    int slot = 30
    while slot < 62
        Form item = akActor.GetWornForm(Armor.GetMaskForSlot(slot))
        if item && IsStrippable(item, slot) && found.Find(item) < 0
            found[count] = item
            count += 1
        endif
        slot += 1
    endwhile
    return found
EndFunction

int Function CountStrippable(Actor akActor) Global
    Form[] found = GetStrippable(akActor)
    int count = found.Find(None)
    if count < 0
        return found.Length
    endif
    return count
EndFunction

; "slot:name" for every worn slot, "(kept)" marking what undress leaves on,
; followed by what each hand holds.
string Function DescribeWorn(Actor akActor) Global
    string worn = ""
    int slot = 30
    while slot < 62
        Form item = akActor.GetWornForm(Armor.GetMaskForSlot(slot))
        if item
            worn += " " + slot + ":" + item.GetName()
            if !IsStrippable(item, slot)
                worn += "(kept)"
            endif
        endif
        slot += 1
    endwhile
    return worn + " | left=" + NameOf(akActor.GetEquippedObject(0)) + " right=" + NameOf(akActor.GetEquippedObject(1))
EndFunction

string Function NameOf(Form item) Global
    if item
        return item.GetName()
    endif
    return "-"
EndFunction

bool Function IsBusy(Actor akActor) Global
    return akActor.IsInCombat() || akActor.IsOnMount()
EndFunction
