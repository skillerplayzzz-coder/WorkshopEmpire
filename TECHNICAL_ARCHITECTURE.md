# WORKSHOP EMPIRE – Technische Architektur

Stand: Phase 0 (2026-10-12)

## 1. Entwicklungsumgebung (geprüft)

| Komponente | Version / Pfad |
|---|---|
| Unreal Engine | **5.7.4** (Launcher-Build, CL 51494982), `F:\UE_5.7`, `EngineAssociation: "5.7"` |
| IDE / Compiler | Visual Studio Community 2026 (18.10.3), MSVC 14.50.35717 und 14.51.36231, LLVM/Clang-Komponente vorhanden |
| Windows SDK | 10.0.22621.0 (von UE 5.7 bevorzugt), 10.0.26100.0 |
| .NET SDK | 9.0.318, 10.0.401 |
| Git | Git aus Visual Studio (`...\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd\git.exe`), kein systemweites Git im PATH |
| Hardware (Entwicklungs-PC) | AMD Ryzen 9 5900X (12 Kerne), 32 GB RAM, NVIDIA GeForce RTX 3060 |

Die Referenzhardware für Performance-Ziele (Spec §23) ist noch nicht festgelegt. Vorschlag: der Entwicklungs-PC oben als "empfohlen", eine schwächere Konfiguration als "minimal" wird später definiert.

## 2. Projektstruktur

```
WORKSHOP EMPIRE/
└── WorkshopEmpire/                     ← Git-Repository, UE-Projektwurzel
    ├── WorkshopEmpire.uproject
    ├── Config/                         ← DefaultEngine/Game/Input/Editor.ini
    ├── Content/WorkshopEmpire/         ← alle eigenen Assets (siehe 2.2)
    ├── Docs/                           ← Master-Prompt (Spezifikation) und weitere Doku
    ├── Source/
    │   ├── WorkshopEmpire.Target.cs        (Game)
    │   ├── WorkshopEmpireEditor.Target.cs  (Editor)
    │   ├── WECore/                     ← Runtime-Modul: Basistypen
    │   ├── WorkshopEmpire/             ← Runtime-Modul: Spiel (Primary Game Module)
    │   └── WorkshopEmpireTests/        ← Editor-Modul: Automation-Tests
    ├── PROJECT_PROGRESS.md, GAME_DESIGN.md, TECHNICAL_ARCHITECTURE.md
    └── CHANGELOG.md, TEST_REPORT.md
```

### 2.1 C++-Module

| Modul | Typ | Abhängigkeiten | Zweck |
|---|---|---|---|
| `WECore` | Runtime, PreDefault | Core, CoreUObject, Engine | Systemübergreifende Basistypen ohne Gameplay-Abhängigkeit: `FWEMoney`, Log-Kategorien. Später: gemeinsame Enums, Gameplay Tags, Interfaces. |
| `WorkshopEmpire` | Runtime, Default (Primary) | WECore, EnhancedInput, NetCore, InputCore; privat UMG, Slate, SlateCore | Game Framework (GameMode, GameState, PlayerState, PlayerController) und ab Phase 1 die Gameplay-Systeme. |
| `WorkshopEmpireTests` | Editor | WECore, WorkshopEmpire | Automation-Tests (`WorkshopEmpire.*`). Wird nicht in Spiel-Builds ausgeliefert. |

**Modulstrategie:** Systeme starten als Unterordner im Modul `WorkshopEmpire` (`Public/<System>/`, `Private/<System>/`). Ein System wird erst in ein eigenes Modul ausgelagert, wenn es eine klare, stabile Schnittstelle hat und von mehreren anderen Systemen genutzt wird (Kandidaten: `WEItems`, `WEInventory`, `WEEconomy`, `WEMachines`). Grund: Jedes Modul kostet Build-Konfiguration und erzwingt Exporte (`*_API`); zu frühe Aufteilung erzeugt Zirkelabhängigkeiten, bevor die Schnittstellen bekannt sind.

**Abhängigkeitsregel:** Abhängigkeiten zeigen nur nach unten: `Tests → Gameplay-Module → WECore → Engine`. UI hängt von Gameplay ab, nie umgekehrt (Gameplay meldet Änderungen über Delegates/Events).

### 2.2 Geplante Ordnerstruktur (Source und Content)

Source (`Source/WorkshopEmpire/{Public,Private}/…`):

| Ordner | System (Spec §24) | Phase |
|---|---|---|
| `Framework/` | Core Game Framework | 0 ✓ |
| `Player/` | Player Character, Kamera, Bewegung, Eingabe | 1 ✓ |
| `Interaction/` | Interaction System (Interface + Komponente, Trace, Prompt) | 1 ✓ |
| `World/` | Garage, Tor, Platzhalter-Einrichtung | 1 ✓ |
| `Dev/` | Test-Automatisierung (nicht in Shipping) | 1 ✓ |
| `Items/` | Weltobjekt `AWEPickupItem` (1 ✓); Item-Definitionen (Data Assets), Item-Instanzen | 1–2 |
| `Inventory/` | Inventarkomponente, Container | 2 |
| `Save/` | SaveGame, Versionierung | 2 |
| `Workshop/` | Werkbank, Zerlegeabläufe, Werkzeuge | 3 |
| `Economy/` | Firmenkonto, Händler, Preise, Transaktionen | 4 |
| `Contracts/` | Aufträge | 4–7 |
| `Repair/` | Reparatursystem | 6 |
| `Building/` | Platzierungssystem | 8 |
| `Machines/`, `Production/` | Maschinen, Rezepte, Förderbänder | 9–10 |
| `Employees/`, `Vehicles/` | Mitarbeiter, Fahrzeuge | 11 |
| `Progression/` | Freischaltungen, Unternehmensstufen | 5+ |
| `UI/` | C++-Basisklassen für UMG-Widgets, HUD | 1 ✓ (HUD), weitere ab 2 |

Content (`Content/WorkshopEmpire/…`): `Maps/`, `Blueprints/`, `Data/` (Data Assets, Data Tables), `Items/`, `Machines/`, `Workshop/`, `UI/`, `Input/`, `Materials/`, `Meshes/`, `Audio/`, `VFX/`. Ordner werden angelegt, sobald die ersten Assets entstehen (Git versioniert keine leeren Ordner).

### 2.3 Namenskonventionen

- C++-Präfix `WE` für alle Typen: `AWEGameMode`, `UWEInventoryComponent`, `FWEMoney`, `EWEItemCondition`.
- Assets nach Epic-Standard: `BP_`, `WBP_`, `DA_`, `DT_`, `M_`, `MI_`, `SM_`, `SK_`, `T_`, `IA_`, `IMC_`, `SFX_`.
- Log-Kategorien: `LogWorkshopEmpire` (allgemein), `LogWENet` (Netzwerk/Sessions); weitere pro System bei Bedarf.
- Automation-Tests: `WorkshopEmpire.<Bereich>.<System>.<Fall>`.

## 3. Klassen (Stand Phase 1)

| Klasse | Datei | Aufgabe |
|---|---|---|
| `FWEMoney` | `WECore/Public/WEMoney.h` | Geldbetrag als `int64` Cent. `FromEuros`, `FromCents`, `TryAdd`/`TrySubtract` mit Überlaufprüfung, `ToDisplayString()` ("1.234,56 €"). BlueprintType, replizierbar. |
| `LogWorkshopEmpire`, `LogWENet` | `WECore/Public/WELog.h` | Log-Kategorien |
| `AWEGameMode` | `WorkshopEmpire/Public/Framework/WEGameMode.h` | Serverregeln. Setzt GameState/PlayerState/PlayerController und `AWECharacter` als Pawn. `MaxCoopPlayers` (Config, Standard 4); `PreLogin` lehnt weitere Spieler ab. |
| `AWEGameState` | `.../Framework/WEGameState.h` | Platzhalter für den replizierten Unternehmenszustand (Konto, Freischaltungen). |
| `AWEPlayerState` | `.../Framework/WEPlayerState.h` | Platzhalter für replizierte Spielerdaten. |
| `AWEPlayerController` | `.../Framework/WEPlayerController.h` | Erzeugt lokal das HUD, setzt Eingabemodus, hängt bei Testparametern `UWEDevAutomationComponent` an. |
| `AWECharacter` | `.../Player/WECharacter.h` | First-Person-Charakter: Kamera (90° FOV, weiches Ducken), Gehen 300 / Sprinten 550 / Ducken 150 cm/s, Springen. Tragen: `HeldItem` (repliziert), `TryPickUpItem`/`DropHeldItem` nur auf dem Server, max. 30 kg. Ablegen dort, wo man hinschaut (bis 250 cm), sonst 110 cm vor dem Spieler. Lässt Gegenstände beim Verlassen (EndPlay) fallen. Platzhalter-Körper (Zylinder) für andere Spieler. |
| `UWEInputConfig` | `.../Player/WEInputConfig.h` | Enhanced-Input-Aktionen + Standardbelegung (WASD, Maus, Leertaste, Shift, Strg/C, E, F). Zur Laufzeit erzeugt, kann später als Data Asset (Tastenbelegung) ersetzt werden. CVars `we.MouseSensitivity`, `we.InvertY`. |
| `IWEInteractable` | `.../Interaction/WEInteractable.h` | Schnittstelle für alles Benutzbare: `GetInteractionText`, `CanInteract` (mit Begründung), `Interact` (nur Server), `GetExamineInfo`. |
| `UWEInteractionComponent` | `.../Interaction/WEInteractionComponent.h` | Lokaler Fokus-Trace mit 20 Hz per Timer (kein Tick). [E] → `ServerInteract`/`ServerDropHeldItem` (RPC). Server prüft Reichweite (250 + 75 cm Toleranz, Abstand zur Bounding Box) und `CanInteract`. Baut die Hinweiszeilen für das HUD; [F] Untersuchen ist rein lokal. |
| `AWEPickupItem` | `.../Items/WEPickupItem.h` | Physischer Gegenstand (Name, Beschreibung, Gewicht, Mesh, Farbe). `Holder` repliziert; gehalten: an HoldPoint angehängt, Physik/Kollision aus; abgelegt: Physik auf dem Server, Bewegung repliziert. Ab Phase 2 Weltdarstellung einer Item-Instanz. |
| `AWEGarageDoor` | `.../World/WEGarageDoor.h` | Sektionaltor, [E] öffnet/schließt. Nur `bIsOpen` repliziert, Animation lokal per Timer. |
| `AWEGarageBuilding`, `AWEPlaceholderProp` | `.../World/` | Parametrische Block-out-Garage bzw. Werkbank/Regal aus Würfel-Instanzen (ISM). |
| `WEPlaceholder::ApplyTint` | `.../World/WEPlaceholderVisuals.h` | Färbt Platzhalter über `M_WEPlaceholder` (transiente MID, nie in der Map gespeichert). |
| `UWEHUDWidget` | `.../UI/WEHUDWidget.h` | C++-UMG-HUD ohne Widget-Asset: Fadenkreuz, "[E] Aufheben", "In der Hand: …", Untersuchen-Panel. Liest nur Zustand. |
| `UWEDevAutomationComponent` | `.../Dev/WEDevAutomationComponent.h` | Nur Nicht-Shipping: `-WESmokeRole=A|B` (Netzwerktest), `-WEShots=<Name>` (Screenshots). Nutzt dieselben Anfragewege wie echte Eingaben. |

Content: `Content/WorkshopEmpire/Maps/L_Garage` (Start-Map), `Materials/M_WEPlaceholder`, `Materials/MI_WEGround`. Beides wird reproduzierbar von `Tools/Maps/build_placeholder_material.py` und `Tools/Maps/build_garage_map.py` erzeugt (Reihenfolge: Material, dann Map; mit `-RenderOffscreen`, nicht `-nullrhi`, weil die Engine beim Platzieren von Lichtern mit NullRHI abstürzt).

**Tragen – Entscheidung:** Statt Physics Handle werden getragene Gegenstände kinematisch an einen Haltepunkt vor der Kamera angehängt. Grund: Physics Handles ruckeln im Multiplayer (Client-Vorhersage gegen Server-Physik) und können Gegenstände durch Wände drücken. Beim Ablegen übernimmt wieder die Physik. Ein Physics Handle für schwere Objekte (z. B. Motor schieben) kann später ergänzt werden.

**Bekannte Vereinfachung Sprint:** Der Client setzt die Sprintgeschwindigkeit sofort und meldet sie per RPC; bis das RPC ankommt, kann es sehr kurze Positionskorrekturen geben. Ausreichend für Koop; eine saubere Lösung (Custom-Flag im `FSavedMove`) ist vorgemerkt, falls es sich im Test störend anfühlt.

## 4. Netzwerkarchitektur (Multiplayer von Anfang an)

**Modell:** Listen-Server (ein Spieler hostet, bis zu 3 treten bei). Dedicated Server ist nicht geplant, die Architektur schließt ihn aber nicht aus, weil keine Logik vom lokalen Spieler des Hosts abhängen darf.

**Server-Autorität (Spec §17.4):**

1. Der Client führt keine verbindlichen Zustandsänderungen aus. Er sendet eine **Anfrage** (Server-RPC), z. B. `ServerRequestPickup(Item)`, `ServerRequestSell(Item, Trader)`.
2. Der Server **validiert** (Reichweite, Besitz, Geld, Zustand) und **führt aus**.
3. Das Ergebnis erreicht alle Clients über **replizierte Properties** (mit `OnRep_` für UI/Effekte). Multicast-RPCs nur für reine Kosmetik (Sound, Partikel).

**Wo liegt welcher Zustand:**

| Zustand | Ort | Repliziert an |
|---|---|---|
| Firmenkonto, Unternehmensstufe, Freischaltungen | `AWEGameState` (bzw. ein `UWECompanyComponent` daran) | alle |
| Spielername, persönliche Statistik | `AWEPlayerState` | alle |
| Spielerinventar | `UWEInventoryComponent` am Pawn/PlayerState | nur Besitzer (`COND_OwnerOnly`), andere sehen nur Getragenes |
| Weltgegenstände | Item-Actor, `bReplicates`, Dormancy wenn ruhend | relevante Clients |
| Maschinen, Lager | Maschinen-Actor + Inventarkomponente | relevante Clients |
| Sessionregeln | `AWEGameMode` | nur Server |

**Inventare:** `FFastArraySerializer`, damit nur geänderte Einträge übertragen werden.

**Konflikte (Spec §17.6):** Server verarbeitet RPCs sequenziell im Game-Thread; jede Aktion prüft vor Ausführung erneut den aktuellen Zustand (z. B. "liegt der Gegenstand noch da / gehört er niemandem"). Gegenstände in Benutzung erhalten einen serverseitigen "Reserviert von"-Eintrag (Lock), der beim Verlassen/Disconnect freigegeben wird.

**Geld:** nur über `FWEMoney`, nur auf dem Server verändert, keine Float-Beträge.

**Performance:** keine Ticks für Maschinen/Items; stattdessen Timer und ein zentraler Simulations-Manager (Subsystem) mit festem Intervall. Net-Dormancy für ruhende Objekte, `NetUpdateFrequency` pro Klasse.

**Tests:** Ab Phase 1 wird jede Interaktion im PIE mit "Play As Listen Server" und 2 Spielern geprüft; ab Phase 2 gibt es automatisierte Netzwerk-Tests, sofern sie sich headless stabil ausführen lassen.

**Steam (Phase 14):** Online Subsystem Steam bzw. Steam Sockets über das Session-Interface; bis dahin LAN/IP über den Standard-NetDriver.

## 5. Speichersystem (Plan)

- `UWESaveGame` (USaveGame) mit `SaveVersion` (int32) und Migrationspfad pro Version.
- Gespeichert wird nur auf dem Server/Host. Clients speichern keinen Unternehmensstand.
- Weltobjekte mit stabiler `FGuid`, damit Referenzen (Gegenstand in Regal X) beim Laden wiederhergestellt werden.
- Schreiben in eine temporäre Datei, danach Umbenennen; vorheriger Stand bleibt als `.bak` erhalten (Schutz vor beschädigten Spielständen).
- Autosave über Timer; mehrere Slots.
- Erste Umsetzung in Phase 2 (Spec §18: "früh einbauen").

## 6. Datengetriebene Inhalte (Plan)

- Statische Itemdefinitionen: `UWEItemDefinition : UPrimaryDataAsset` (ID, Name, Kategorie, Gewicht, Preise, Materialien, Zerlegeschritte). Neue Gegenstände ohne C++-Änderung.
- Iteminstanzen: `FWEItemInstance` (Definition + Zustand + Qualität + Instanz-GUID), separat repliziert/gespeichert.
- Zerlegeabläufe und Rezepte als Daten (Schrittliste mit Werkzeuganforderung), nicht als Code pro Gegenstand.
- Laden über Asset Manager (Primary Asset Types).

## 7. Rendering (Entscheidung Phase 0)

Engine-Standard von UE 5.7 bleibt aktiv: DX12, SM6, Lumen (GI + Reflexionen), Virtual Shadow Maps, Nanite-fähig, keine statische Beleuchtung. Grund: Innenräume (Garage, Hallen) profitieren stark von Lumen; Platzhaltergrafik bis Phase 15 (Spec §19). Bewertung mit Messungen erfolgt, sobald es eine Referenzszene gibt.

## 8. Build und Tests

```
F:\UE_5.7\Engine\Build\BatchFiles\Build.bat WorkshopEmpireEditor Win64 Development -Project="<Pfad>\WorkshopEmpire.uproject" -WaitMutex
F:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "<Pfad>\WorkshopEmpire.uproject" -ExecCmds="Automation RunTests WorkshopEmpire;Quit" -unattended -nopause -NullRHI -testexit="Automation Test Queue Empty" -ReportExportPath="<Pfad>\Saved\TestReports\<Name>"
```

Teststrategie siehe `TEST_REPORT.md`.
