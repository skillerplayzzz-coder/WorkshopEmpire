# WORKSHOP EMPIRE – Testbericht

## Teststrategie

| Ebene | Werkzeug | Ab Phase | Ausführung |
|---|---|---|---|
| Build | UnrealBuildTool (Editor- und Game-Target, Win64 Development) | 0 | nach jeder Änderung |
| Unit-Tests | UE Automation (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`), Pfad `WorkshopEmpire.*` | 0 | headless über `UnrealEditor-Cmd -NullRHI` |
| Funktionstests | Automation-Tests mit Test-World bzw. Functional Test Actors in Test-Maps | 1–2 | headless, soweit stabil |
| Multiplayer | PIE "Play As Listen Server", 2 Spieler (manuell); automatisierte Netzwerk-Tests, wenn headless stabil | 1 (manuell), 2+ (automatisiert) | pro Phase |
| Speichern/Laden | Automation: speichern → laden → vergleichen; beschädigte Datei → sauberer Fehler | 2 | pro Phase |
| Regression | gesamte Suite `WorkshopEmpire` | ab 1 | vor jeder Übergabe |
| Performance | Unreal Insights / `stat unit` in Referenzszene | ab 9 | Messung statt Schätzung |
| Praxistest | Mike, nach Anleitung im Development Report | jede Phase | manuell |

Grundsatz: Ein erfolgreicher Build heißt nicht, dass das Gameplay funktioniert. Nur tatsächlich ausgeführte Tests werden als bestanden aufgeführt.

---

## Phase 0 – Lauf vom 2026-10-12

Umgebung: UE 5.7.4, MSVC (VS 2026 18.10.3), Windows 11 Pro, Ryzen 9 5900X, RTX 3060.

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build `WorkshopEmpireEditor Win64 Development` | ✅ erfolgreich, 0 Warnungen/Fehler im Projektcode (Engine-Header-Deprecations aus Chaos/UnrealEd sind Engine-intern) |
| Build `WorkshopEmpire Win64 Development` (Game) | ✅ erfolgreich, `Binaries/Win64/WorkshopEmpire.exe` gelinkt |
| Editor-Start headless (`UnrealEditor-Cmd -NullRHI`) | ✅ Projekt lädt, `WECore module started.` im Log, keine Fehler aus Projektcode |
| `WorkshopEmpire.Core.Money.Construction` | ✅ bestanden |
| `WorkshopEmpire.Core.Money.Arithmetic` (inkl. 1000 × 0,10 € = exakt 100,00 €) | ✅ bestanden |
| `WorkshopEmpire.Core.Money.Overflow` | ✅ bestanden |
| `WorkshopEmpire.Core.Money.DisplayString` | ✅ bestanden |
| `WorkshopEmpire.Framework.GameModeDefaults` | ✅ bestanden |
| Netzwerk-Rauchtest: headless Dedicated Server (Port 7790, `MaxCoopPlayers=1` per `-ini`-Override) + 2 headless Clients über 127.0.0.1 | ✅ Client 1: `Welcomed by server … Game: /Script/WorkshopEmpire.WEGameMode`, Server: `Player joined (1/1)`, Client-PlayerController `NetMode=Client`. Client 2: Server lehnt mit `Session is full (1/1 players)` ab, Client erhält `PendingConnectionFailure` mit dieser Meldung. |

Summe Automation: **5 bestanden, 0 fehlgeschlagen, 0 nicht ausgeführt.** Bericht: `Saved/TestReports/Phase0/index.json` (nicht versioniert). Netzwerk-Logs: `Saved/Logs/NetTest*.log` (nicht versioniert).

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Editor mit Grafik öffnen, PIE | Erfordert Bildschirm/Bedienung; Teil von Mikes Praxistest |
| PIE mit 2 Spielern als Listen Server (mit Grafik) | Erfordert Bildschirm; Teil von Mikes Praxistest. Die Netzwerklogik selbst ist durch den headless Server/Client-Test oben abgedeckt. |
| Limit bei Standardwert 4 (5. Spieler) | Nicht mit 5 Clients ausgeführt; dieselbe Logik wurde mit Limit 1 geprüft |
| Gestartete Game-EXE | Ohne gecookte Inhalte nicht lauffähig; Packaging folgt später |
| Shipping-Build | in Phase 0 nicht nötig |

### Offene Probleme

Keine.

### Manueller Test für Mike (Phase 0)

1. `F:\Spiele\WORKSHOP EMPIRE\WorkshopEmpire\WorkshopEmpire.uproject` doppelklicken. Falls gefragt wird, ob Module neu gebaut werden sollen: "Ja".
2. Der Editor öffnet eine leere Standard-Map (Himmel + Boden).
3. *Window → Output Log* öffnen, in das Filterfeld `LogWorkshopEmpire` eingeben: Zeile `WECore module started.` muss da sein.
4. *Edit → Project Settings → Maps & Modes*: Default GameMode = `WEGameMode`.
5. Neben dem Play-Button auf ⋮ klicken: *Number of Players* = 2, *Net Mode* = *Play As Listen Server*, dann Play. Zwei Fenster erscheinen, man fliegt mit WASD + Maus (Platzhalter-Pawn).
6. Im Output-Log nach `LogWENet` filtern: zwei Zeilen `Player joined: … (1/4)` und `(2/4)` sowie `BeginPlay (NetMode=ListenServer …)` bzw. `NetMode=Client`.
7. Optional: *Tools → Session Frontend → Automation*, nach `WorkshopEmpire` filtern, alle anhaken, *Start Tests*: 5 grüne Tests.
