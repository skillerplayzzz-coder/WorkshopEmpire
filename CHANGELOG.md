# Changelog

## [0.2.0] – 2026-10-12 – Phase 2: Items und Inventar

### Neu
- Datengetriebenes Itemsystem: `UWEItemDefinition` (Data Asset) mit Kategorie, Seltenheit, Gewicht, Grundwert, Stapelbarkeit, Inventartauglichkeit, Reparier-/Zerlegbarkeit, Materialzusammensetzung, Mesh, Farbe und Icon-Feld.
- 9 Gegenstände als Data Assets: Alter Computer, Röhrenmonitor, Schraubendreher, Werkzeugkiste, Alter Elektromotor, Karton mit Kabeln, Blechschrauben, Kupferkabel, Arbeitsspeicher.
- Item-Instanzen `FWEItemInstance`: eindeutige ID, Menge, Zustand (Neu … Schrott), Qualität, Schätzwert.
- Inventarsystem `UWEInventoryComponent`: Plätze, Gewichtsgrenze, Stapeln, Teilen, Zusammenführen, Tauschen, Sortieren, Transfer zwischen Inventaren.
- Spielerinventar (20 Plätze, 25 kg), Werkbanklager (8/60 kg), Regallager (15/150 kg); [E] öffnet Werkbank/Regal.
- [E] steckt Kleinteile ein (auch teilweise, wenn das Gewicht nicht für alles reicht); sperrige Gegenstände werden weiter getragen.
- Inventarfenster (Tab/I) mit Drag & Drop, Shift = halber Stapel, Doppelklick = umlagern, Rechtsklick = ablegen, Tooltips mit allen Werten.
- Untersuchen zeigt jetzt Kategorie, Zustand, Qualität, Gewicht und Schätzwert.
- Speichern und Laden: F5/F9, benannte Spielstände (`WESave`/`WELoad`), Autosave alle 5 Minuten, Kurzmeldungen im HUD.

### Multiplayer
- Inventare per Fast-Array-Replikation (nur Änderungen werden übertragen).
- Alle Inventaraktionen laufen über Server-RPCs; der Server prüft Besitz und Reichweite. Fremde Spielerinventare sind gesperrt.
- Getestet: keine Duplikate, Mengen bleiben über Spieler, Lager und Welt erhalten.
- Speichern/Laden nur durch den Host; später beitretende Spieler bekommen ihren gespeicherten Stand.

### Technisch
- Spielstandformat mit Versionsnummer, CRC-Prüfung, `.tmp`-Schreiben und `.bak`-Sicherung; beschädigte Dateien fallen auf das Backup zurück.
- Gegenstände werden über stabile `ItemId` gespeichert; Asset Manager kennt den Typ `WEItem`.
- Neues Skript `Tools/Maps/build_item_definitions.py`; Map-Skript nutzt die Definitionen und richtet die Lager ein.
- `AWEPickupItem` stellt jetzt eine Item-Instanz dar (vorher eigene Name/Gewicht-Felder).
- 12 neue Automation-Tests (insgesamt 26), Netzwerktest-Rollen C/D, Screenshot-Lauf P2 mit Speichern/Laden.

### Fehlerbehebungen (während der Phase)
- Fehlende Modulabhängigkeit `CoreOnline`.
- Asset-Manager-Fehler durch abweichende Primary Asset Ids.
- Lange Namen liefen in Nachbarkacheln; Welt-Hinweise schienen durch das geöffnete Inventar.

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
