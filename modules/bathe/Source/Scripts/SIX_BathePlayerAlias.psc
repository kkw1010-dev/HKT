ScriptName SIX_BathePlayerAlias extends ReferenceAlias
{SI-Extensions / Bathe. Re-runs the quest startup on every save load.}

Event OnPlayerLoadGame()
    (GetOwningQuest() as SIX_BatheQuestScript).Startup("load")
EndEvent
