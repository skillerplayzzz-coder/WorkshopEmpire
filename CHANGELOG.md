# Changelog

## [0.1.0] – 2026-10-12 – Phase 1: Spielbarer First-Person-Prototyp

### Neu
- First-Person-Charakter `AWECharacter`: Gehen, Sprinten (Shift), Ducken (Strg/C) mit weicher Kamera, Springen, Mausblick; Empfindlichkeit über `we.MouseSensitivity`, Invertieren über `we.InvertY`.
- Eingabe über Enhanced Input (`UWEInputConfig`), Standardbelegung WASD/Maus/Leertaste/Shift/Strg/C/E/F.
- Interaktionssystem: `IWEInteractable` + `UWEInteractionComponent` mit Hinweisen "[E] Aufheben", "[E] Ablegen", "[E] Tor öffnen", "[F] Untersuchen".
- Gegenstände `AWEPickupItem`: aufnehmen, tragen (max. 30 kg), dort ablegen, wo man hinschaut, untersuchen (Name, Beschreibung, Gewicht).
- Garagentor `AWEGarageDoor` zum Öffnen/Schließen.
- HUD `UWEHUDWidget`: Fadenkreuz, Hinweise, "In der Hand", Untersuchen-Panel.
- Platzhalter-Garage `L_Garage` (neue Startmap) mit Werkbank, Regal und 6 Gegenständen (Computer, Röhrenmonitor, Schraubendreher, Werkzeugkiste, Karton mit Kabeln, zu schwerer Elektromotor).

### Multiplayer
- Alle Interaktionen laufen als Server-RPC; Server prüft Reichweite und Regeln.
- Zwei Spieler können nicht denselben Gegenstand nehmen; verlässt ein Spieler das Spiel, fällt sein Gegenstand in die Welt.
- Getragene Gegenstände und Torzustand werden repliziert.

### Technisch
- Neue Ordner `Player/`, `Interaction/`, `Items/`, `World/`, `UI/`, `Dev/` im Spielmodul; UMG/Slate als Abhängigkeit.
- Platzhalter-Material `M_WEPlaceholder` (Farbparameter, für Instanced Meshes freigegeben) und `MI_WEGround`.
- Reproduzierbare Erzeugung von Material und Map: `Tools/Maps/build_placeholder_material.py`, `Tools/Maps/build_garage_map.py`.
- Test-Automatisierung `UWEDevAutomationComponent` (nur Nicht-Shipping): `-WESmokeRole=A|B`, `-WEShots=<Name>`.
- 9 neue Automation-Tests (insgesamt 14), Test-Welt-Harness `FWETestWorld`.
- Plugin Python Editor Script (nur Editor).

### Fehlerbehebungen (während der Phase)
- Test-Welt startete kein BeginPlay/EndPlay, dadurch war "Ablegen beim Verlassen" ungetestet.
- Platzhalter-Farben wurden im Spiel nicht angezeigt (fehlender Materialparameter bzw. Instanced-Mesh-Flag).
- Überbelichtete Garage; große getragene Gegenstände verdeckten die Sicht.

## [0.0.1] – 2026-10-12 – Phase 0: Projektanalyse und Setup

### Neu
- UE 5.7 C++-Projekt `WorkshopEmpire` mit Editor- und Game-Target.
- Modul `WECore`: `FWEMoney` (Geld in int64 Cent, `TryAdd`/`TrySubtract` mit Überlaufprüfung, Anzeige "1.234,56 €"), Log-Kategorien `LogWorkshopEmpire` und `LogWENet`.
- Modul `WorkshopEmpire`: `AWEGameMode` (Koop-Limit 4 Spieler per Config, Ablehnung weiterer Logins, Join/Leave-Log), `AWEGameState`, `AWEPlayerState`, `AWEPlayerController` (NetMode-Log).
- Modul `WorkshopEmpireTests`: 5 Automation-Tests (`WorkshopEmpire.Core.Money.*`, `WorkshopEmpire.Framework.GameModeDefaults`).
- Projektdokumentation: PROJECT_PROGRESS, GAME_DESIGN, TECHNICAL_ARCHITECTURE, CHANGELOG, TEST_REPORT; Spezifikation unter `Docs/MASTER_PROMPT_V1.0.md`.

### Technisch
- `AWEGameMode` als globaler Standard-GameMode, Platzhalter-Map `/Engine/Maps/Templates/Template_Default`.
- DX12, SM6, Lumen und Virtual Shadow Maps (Engine-Standard), keine statische Beleuchtung.
- Plugins: Enhanced Input, Modeling Tools Editor Mode (nur Editor, für Platzhalter-Geometrie).
- Git-Repository mit Unreal-`.gitignore` und `.gitattributes`.
