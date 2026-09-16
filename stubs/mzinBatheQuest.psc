ScriptName mzinBatheQuest extends Quest
{COMPILE-TIME STUB. Declares only the Bathing in Skyrim - Renewed 2.7.8 members
CIGAR calls; the real script ships with BiS. Never deploy this file.
Signatures were read from the decompiled mzinBatheQuest.pex.}

GlobalVariable Property WaterRestrictionEnabled Auto
GlobalVariable Property DirtinessPercentage Auto

bool Function TryWashActor(Actor DirtyActor, MiscObject WashProp, bool Shower, bool PlayerTeammates)
    return false
EndFunction

bool Function IsUnderWaterfall(Actor DirtyActor)
    return false
EndFunction

bool Function IsActorAnimating(Actor DirtyActor)
    return false
EndFunction
