ScriptName CIGAR_PlayerAlias extends ReferenceAlias
{CIGAR: re-runs the owning module's startup on every save load.}

Event OnPlayerLoadGame()
    (GetOwningQuest() as CIGAR_ModuleBase).Startup("load")
EndEvent
