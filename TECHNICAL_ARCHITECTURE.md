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
| `Items/` | Weltobjekt, Item-Definitionen (Data Assets), Item-Instanzen | 1–2 ✓ |
| `Inventory/` | Inventarkomponente, Container | 2 ✓ |
| `Save/` | SaveGame, Versionierung | 2 ✓ (Version 2 seit Phase 3) |
| `Workshop/` | Werkbank, Zerlegeabläufe, Werkzeuge | 3 ✓ |
| `Economy/` | Firmenkonto, Markt/Preise, Händler, Terminal, Aufträge | 4 ✓ |
| `Contracts/` | (Aufträge liegen in `Economy/`, Ausbau in Phase 7) | 7 |
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

## 3. Klassen (Stand Phase 8)

| Klasse | Datei | Aufgabe |
|---|---|---|
| `FWEMoney` | `WECore/Public/WEMoney.h` | Geldbetrag als `int64` Cent. `FromEuros`, `FromCents`, `TryAdd`/`TrySubtract` mit Überlaufprüfung, `ToDisplayString()` ("1.234,56 €"). BlueprintType, replizierbar. |
| `LogWorkshopEmpire`, `LogWENet` | `WECore/Public/WELog.h` | Log-Kategorien |
| `AWEGameMode` | `WorkshopEmpire/Public/Framework/WEGameMode.h` | Serverregeln, setzt das Startkapital. Setzt GameState/PlayerState/PlayerController und `AWECharacter` als Pawn. `MaxCoopPlayers` (Config, Standard 4); `PreLogin` lehnt weitere Spieler ab. `SaveToSlot`/`LoadFromSlot`, Autosave alle `AutosaveIntervalSeconds` (Config, 300 s), wendet den zuletzt geladenen Stand auf später beitretende Spieler an. |
| `AWEGameState` | `.../Framework/WEGameState.h` | Gemeinsamer Zustand aller Spieler: `UWECompanyComponent` (Konto), `UWEMarketComponent` (Nachfrage), `UWEContractComponent` (Kundenaufträge), `UWEProgressionComponent` (Werkstatt-Upgrades, Leitfaden), `UWEAuctionComponent` (Auktionen), `UWEWorkshopLayoutComponent` (Einrichtung). |
| `UWECompanyComponent` | `.../Economy/WECompanyComponent.h` | Firmenkonto in `FWEMoney` (Cent), nur der Server bucht (`Deposit`, `TryWithdraw`, nie unter 0, Überlaufschutz), letzte 12 Buchungen repliziert. `RegisterRequest(FGuid)`: jede Client-Anfrage trägt eine ID, Wiederholungen werden ignoriert (Schutz vor Doppelbuchung). Startkapital 250 € aus `AWEGameMode::StartingCapitalCents` (Config). |
| `UWEMarketComponent` | `.../Economy/WEMarketComponent.h` | Nachfragefaktor je Kategorie (0,8–1,2, alle 5 min leichte Zufallsbewegung mit Zug zur Mitte, repliziert). Preisfunktionen: Marktwert = Schätzwert × Nachfrage; Ankauf durch Händler = 60 % × Händlerfaktor; Verkauf an Spieler = 130 % × Angebotsfaktor (kein Arbitrage-Gewinn möglich). |
| `UWETraderDefinition` | `.../Economy/WETraderDefinition.h` | Data Asset je Händler: was er ankauft (Kategorie + Faktor), was er verkauft (Item, Menge, Zustand, Qualität, Preisfaktor), Nachschub-Intervall. |
| `AWETraderTerminal`, `AWEDeliveryZone` | `.../Economy/WETraderTerminal.h` | Online-Marktplatz in der Garage: [E] öffnet den Handel. Server-Funktionen `SellFromInventory`, `SellHeldItem`, `Buy`, `AcceptContract`, `DeliverContract`, `BuyUpgrade` mit Reichweite, Zugriff, Geld, Vorrat und Anfrage-ID. Verkauft wird an den Händler mit dem besten Preis. Seit Phase 7 wird der Vorrat bei jedem Nachschub als `FWEStockLine`-Liste gewürfelt (Chance je Angebot, Zustands- und Qualitätsstreuung); `BidOnAuction` für Gebote. Reihenfolge innerhalb eines Handels: erst Ware bzw. Geld prüfen, dann buchen, dann übergeben. Kleinteile ins Inventar, sperrige Käufe auf die Lieferpalette. Vorrat repliziert. |
| `UWEContractComponent`, `UWEContractCatalog` | `.../Economy/WEContractComponent.h` | Kundenaufträge aus Vorlagen (Data Asset): immer 3 Angebote, max. 3 angenommene, Frist, Belohnung = Marktwert × Faktor; Abliefern nimmt passende Gegenstände (Zustand mindestens X, nicht teilzerlegt) aus dem Inventar und zahlt aufs Firmenkonto. Seit Phase 6 Art `Repair`: Beim Annehmen erscheint das Kundengerät (Flag `CustomerItem`, Id im Auftrag) an der Lieferpalette; Abliefern verlangt genau dieses Gerät repariert in der Hand; bei Ablauf wird es entfernt. |
| `UWEAuctionComponent`, `UWEAuctionCatalog` | `.../Economy/WEAuctionComponent.h` | Auktionen (Phase 7) am GameState: max. 3 Lose aus Vorlagen (Data Asset `DA_AuctionCatalog`), Startpreis = Marktwert × Faktor, ein Mitbieter je Los mit geheimem Limit (nur auf dem Server, `TMap`), Gegengebot sofort, Mindestschritt 5 % (≥ 1 €, auf 50 ct gerundet), Verlängerung auf 20 s bei späten Geboten. `FinishLot` zieht das Geld erst beim Zuschlag ab (`TryWithdraw`) und legt die Ware an die Lieferpalette; reicht das Geld nicht, verfällt der Zuschlag. Lose und letzte Ergebnisse repliziert. |
| `UWEWorkshopLayoutComponent`, `UWEFurnitureCatalog` | `.../Workshop/WEWorkshopLayoutComponent.h` | Bau- und Platzierungssystem (Phase 8) am GameState: Möbelkatalog (`DA_FurnitureCatalog`), Regel `CanPlaceAt` (Grundriss komplett in einem `AWEBuildArea`, Box-Überlappung gegen Wände, Möbel, Gegenstände, Spieler; läuft im Client für die Vorschau und auf dem Server), `PlaceFurniture`/`MoveFurniture`/`RemoveFurniture` (Server, mit Anfrage-ID, Geld, Raster 10 cm / 15°, Abbauen nur leer mit 50 % Erstattung), `SpawnFurniture` mit stabilem Namen (Lagerinhalt im Spielstand wird über den Namen zugeordnet; Name eines gerade abgebauten Möbels wird freigegeben). |
| `UWEBuildComponent`, `AWEBuildGhost` | `.../Workshop/WEBuildComponent.h` | Baumodus des lokalen Spielers: Auswahl (Mausrad), Vorschau-Actor nur lokal (nicht repliziert, grün/rot), Zielstrahl 30 Hz per Timer, Drehen, Bestätigen/Verschieben/Abbauen über Server-RPCs des PlayerControllers. |
| `AWEBuildArea` | `.../World/WEBuildArea.h` | Box, in der gebaut werden darf (Garage; später neue Räume). |
| `UWELootTable` | `.../Items/WELootTable.h` | Gewichtete Fundtabelle (Data Asset `LT_*`): Gegenstand, Gewicht, Menge, Zustands- und Qualitätsbereich; `RollOne`/`Roll` mit `FRandomStream`. Für Retourenpaletten, Schrotthaufen. |
| `AWEScrapPile` | `.../World/WEScrapPile.h` | Schrotthaufen (Phase 7): Server füllt per Timer auf `MaxItems` freie Weltgegenstände im Umkreis auf (zählt, was dort liegt – funktioniert auch nach dem Laden ohne Referenzen). Nur die nächste Nachschubzeit wird repliziert (Hinweis im HUD). |
| `UWETradeWidget`, `UWETradeRowWidget` | `.../UI/WETradeWidget.h` | Handelsbildschirm (C++-UMG) mit zwei Reitern (`UWidgetSwitcher`): Marktplatz (Verkaufen/Kaufen/Aufträge/Werkstatt verbessern) und Auktionen & Beschaffung; letzte Buchungen. Zeilentexte mit fester Umbruchbreite (kein Ein-Bild-Flackern nach Neuaufbau). Baut Listen nur bei Änderungen neu. |
| `AWEPlayerState` | `.../Framework/WEPlayerState.h` | Trägt das Spielerinventar (`UWEInventoryComponent`, 20 Plätze, 25 kg), damit es Respawns übersteht und pro Spieler gespeichert werden kann. |
| `AWEPlayerController` | `.../Framework/WEPlayerController.h` | HUD und Inventarfenster (lokal). Server-RPCs für Inventar: `ServerMoveItem`, `ServerQuickTransfer`, `ServerDropFromInventory`, `ServerSortInventory`; Server prüft Zugriff (`CanUseInventory`). `ClientOpenContainer`, `ClientShowMessage`. Speichern/Laden nur für den Host (F5/F9, Konsole `WESave <Slot>`/`WELoad <Slot>`). |
| `AWECharacter` | `.../Player/WECharacter.h` | First-Person-Charakter: Kamera (90° FOV, weiches Ducken), Gehen 300 / Sprinten 550 / Ducken 150 cm/s, Springen. Tragen: `HeldItem` (repliziert), `TryPickUpItem`/`DropHeldItem` nur auf dem Server, max. 30 kg. Ablegen dort, wo man hinschaut (bis 250 cm), sonst 110 cm vor dem Spieler. Lässt Gegenstände beim Verlassen (EndPlay) fallen. Platzhalter-Körper (Zylinder) für andere Spieler. |
| `UWEInputConfig` | `.../Player/WEInputConfig.h` | Enhanced-Input-Aktionen + Standardbelegung (WASD, Maus, Leertaste, Shift, Strg/C, E, F, R, Q, B, X, Mausrad, Tab/I, F5, F9; 20 Belegungen). Im Baumodus leitet `AWECharacter` E/R/Q/X/Mausrad an die `UWEBuildComponent` weiter. Zur Laufzeit erzeugt, kann später als Data Asset (Tastenbelegung) ersetzt werden. CVars `we.MouseSensitivity`, `we.InvertY`. |
| `IWEInteractable` | `.../Interaction/WEInteractable.h` | Schnittstelle für alles Benutzbare: `GetInteractionText`, `CanInteract` (mit Begründung), `Interact` (nur Server), `GetExamineInfo`, optional `GetSecondaryInteraction`/`InteractSecondary` ([R]) und `GetAlternativeInteraction`/`InteractAlternative` ([Q], z. B. Reparieren). |
| `UWEInteractionComponent` | `.../Interaction/WEInteractionComponent.h` | Lokaler Fokus-Trace mit 20 Hz per Timer (kein Tick). [E] → `ServerInteract`/`ServerDropHeldItem`, [R] → `ServerInteractSecondary`, [Q] → `ServerInteractAlternative` (RPCs). Server prüft Reichweite (250 + 75 cm Toleranz, Abstand zur Bounding Box) und `CanInteract`. Baut die Hinweiszeilen für das HUD; [F] Untersuchen ist rein lokal. |
| `AWEPickupItem` | `.../Items/WEPickupItem.h` | Weltdarstellung einer `FWEItemInstance` (Mesh/Größe/Farbe/Masse aus der Definition). [E]: kleine Gegenstände ins Inventar (`TryStoreWorldItem`, auch teilweise bei Gewichtsgrenze), sperrige werden getragen. `Item` und `Holder` repliziert. `SpawnWorldItem` für Ablegen aus dem Inventar und Laden. |
| `UWEItemDefinition` | `.../Items/WEItemDefinition.h` | Primary Data Asset (Typ `WEItem`) je Gegenstandsart: `ItemId`, Name, Beschreibung, Kategorie, Seltenheit, Gewicht, `BaseValue` (FWEMoney), Stapelbarkeit, passt ins Inventar, reparierbar, `Disassembly` (Zerlegeablauf), `Repair` (Reparaturablauf), `BrokenValueMultiplier` (Wertabschlag, solange kaputt), Werkzeugtyp + Tempo (z. B. Akkuschrauber = Schraubendreher ×2,5), Materialzusammensetzung, Mesh, Größe, Farbe, Icon. `FindById`. |
| `FWEItemInstance` | `.../Items/WEItemInstance.h` | Definition + `InstanceId` (GUID) + Menge + Zustand (Neu … Schrott) + Qualität (0–100) + `DisassemblyStep` (Zerlegefortschritt, bleibt beim Herunternehmen erhalten) + `FWERepairState` (Bitmasken je Fehler: kaputt / ausgebaut / ersetzt, Flags: Fehler ausgewürfelt, diagnostiziert, aufbereitet, Kundengerät). `CanStackWith`, Gewicht, `GetEstimatedValue`, `SplitOff`. |
| `WEItems::*`, Enums | `.../Items/WEItemTypes.h` | Kategorie, Zustand, Seltenheit, Rohstoffart; Texte, Wertfaktor und Farbe je Zustand. |
| `UWEInventoryComponent` | `.../Inventory/WEInventoryComponent.h` | Slot-Inventar mit `FFastArraySerializer` (nur Änderungen werden übertragen). Plätze + Gewichtsgrenze, Stapeln nur bei gleicher Definition und gleichem Zustand, `TryAddItem`, `RemoveFromSlot`, `MoveItem` (verschieben, teilen, zusammenführen, tauschen, auch zwischen Inventaren), `SortItems`, `SetAllSlots`, `TransferAllTo` ("Alles nehmen": verschiebt, was passt, Rest bleibt). Alle Änderungen nur auf dem Server. `CanBeAccessedBy`: eigenes Inventar oder Lager in Reichweite (325 cm); fremde Spielerinventare sind privat. `OnInventoryChanged` für die UI. |
| `UWESaveGame`, `FWESaveSystem` | `.../Save/` | Siehe Abschnitt 5. |
| `UWEDisassemblyDefinition` | `.../Workshop/WEDisassemblyDefinition.h` | Data Asset mit Zerlegeschritten (`FWEDisassemblyStep`: Aktionstext, benötigter Werkzeugtyp, Grunddauer, Ergebnisse `FWEDisassemblyOutput` mit Menge, Chance und ob der Zustand vererbt wird). Mehrere Gegenstände können denselben Ablauf nutzen; Detailtiefe je Gegenstand frei (1 bis n Schritte). |
| `UWERepairDefinition` | `.../Workshop/WERepairDefinition.h` | Data Asset je Gerätetyp (`REP_*`): mögliche Fehler (`FWERepairFault`: Name, Aus-/Einbautext, Ersatzteil, Werkzeug, Dauer, Gewichtung, ob das kaputte Teil zurückkommt), Diagnose-Werkzeug/-Dauer, Testdauer, Chance auf zweiten Fehler, Versagensquote beschädigter Ersatzteile, Qualitätsgewinne, Material zum Aufbereiten. Max. 8 Fehler (Bitmaske). |
| `AWEWorkbench` | `.../Workshop/WEWorkbench.h` | Werkbank (erbt Lager und Optik von `AWEPlaceholderProp`; eigenes Arbeitstempo `BenchSpeedMultiplier`, z. B. große Werkbank ×1,2). Gegenstand ablegen (Arbeitsplatz links), Schritte nacheinander ausführen, Werkzeugprüfung (bestes passendes Werkzeug im Spielerinventar bestimmt das Tempo), Abbruch wenn der Arbeiter weggeht, Ergebnisse ins Werkbanklager bzw. vor die Werkbank, Gegenstand ist nach dem letzten Schritt verbraucht. Repliziert: `PlacedItem` und `FWEWorkbenchWork` (Schritt, Dauer, Arbeiter, Ereigniszähler). Fortschritt, Leiste an der Vorderkante und Sounds werden auf jedem Rechner lokal berechnet. Seit Phase 6 auch Reparatur: `GetNextRepairStep` (Diagnose → Ausbau → Einbau je Fehler → Funktionstest → Aufbereiten), `TryStartRepairStep` ([Q]), `FindSparePart` (Inventar des Arbeiters, dann Werkbanklager; bester Zustand zuerst). |
| `UWEInventoryWidget`, `UWEInventorySlotWidget`, `UWEItemDragOperation` | `.../UI/` | Inventarfenster (C++-UMG): Spielerinventar, optional neben einem Lager; Drag & Drop, Shift = halber Stapel, Doppelklick = umlagern, Rechtsklick = ablegen, aus dem Fenster ziehen = ablegen, Sortieren, Tooltip mit allen Werten. |
| `AWEGarageDoor` | `.../World/WEGarageDoor.h` | Sektionaltor, [E] öffnet/schließt. Nur `bIsOpen` repliziert, Animation lokal per Timer. |
| `AWEGarageBuilding`, `AWEPlaceholderProp` | `.../World/` | Parametrische Block-out-Garage bzw. Werkbank/Regal/Kiste aus Würfel-Instanzen (ISM, beweglich). Gekaufte Möbel tragen eine replizierte `FurnitureId`; Clients bauen daraus Geometrie und Lagergröße selbst (`ApplyFurniture`), Bewegung wird repliziert. `AWEPlaceholderProp` hat optional ein Lager (Werkbank 8 Plätze/60 kg, Regal 15/150 kg); [E] öffnet es (Server prüft und schickt `ClientOpenContainer`). |
| `WEPlaceholder::ApplyTint` | `.../World/WEPlaceholderVisuals.h` | Färbt Platzhalter über `M_WEPlaceholder` (transiente MID, nie in der Map gespeichert). |
| `UWEUpgradeCatalog`, `FWEUpgradeDefinition` | `.../Progression/WEUpgradeDefinition.h` | Data Asset `DA_UpgradeCatalog` mit allen Werkstatt-Upgrades: Id, Name, Beschreibung, Preis, Voraussetzung, Wirkungen (Werkbankplätze, Inventar-kg, Arbeitstempo, Lichtfaktor, schaltet Möbel frei). Neue Upgrades = neuer Eintrag, kein C++. |
| `UWEProgressionComponent` | `.../Progression/WEProgressionComponent.h` | Am GameState: gekaufte Upgrades und Leitfaden-Schritt (repliziert). Nur der Server vergibt Upgrades (`GrantUpgrade`, prüft Voraussetzung und Doppelkauf). `ApplyEffects` läuft auf **jedem** Rechner aus dem replizierten Zustand: Möbel mit `RequiredUpgrade` ein-/ausblenden, Werkbanklager vergrößern, Tragkraft aller Spieler, Lampen mit Tag `WE_WorkshopLight` (Grundhelligkeit wird gemerkt, damit nichts doppelt wirkt). Leitfaden: 6 Schritte, rückt nur bei echten Spielereignissen (`NotifyEvent`) in der richtigen Reihenfolge vor; danach zeigt er das günstigste noch kaufbare Upgrade. |
| `UWEHUDWidget` | `.../UI/WEHUDWidget.h` | C++-UMG-HUD ohne Widget-Asset: Fadenkreuz, "[E] Aufheben", "In der Hand: …", Untersuchen-Panel, Kurzmeldungen ("Gespeichert", "Inventar voll"), Firmenkonto oben rechts, Leitfaden oben links (wächst mit dem Text). Liest nur Zustand. |
| `UWEDevAutomationComponent` | `.../Dev/WEDevAutomationComponent.h` | Nur Nicht-Shipping: `-WESmokeRole=A|B` (Tragen), `C|D` (Inventar), `E|F` (Werkbank), `G|H` (Handel), `I|J` (Upgrades im Koop als Listen-Server), `K|L` (Reparaturauftrag mit 2 Spielern), `M|N` (Auktion und Palette mit 2 Spielern), `O|P` (Möbel aufstellen/verschieben/abbauen mit 2 Spielern), `-WEShots=P1…P8` (Screenshots; P5 = kompletter Ablauf inkl. Speichern/Laden, P6 = Reparatur). Dev-RPCs nur außerhalb von Shipping: `ServerDevTeleport`, `ServerDevAddContractOffer`, `ServerDevEndAuctions`. Nutzt dieselben Anfragewege wie echte Eingaben. |

Content: `Content/WorkshopEmpire/Maps/L_Garage` (Start-Map), `Materials/M_WEPlaceholder`, `Materials/MI_WEGround`, `Data/Items/DA_Item_*` (18 Gegenstände), `Data/Disassembly/DIS_*` (3 Zerlegeabläufe), `Data/Economy/` (3 Händler, Auftragskatalog), `Data/Progression/DA_UpgradeCatalog` (6 Upgrades), `Data/Repair/REP_*` (2 Reparaturabläufe), `Data/Loot/LT_*` (3 Beutetabellen), `Data/Economy/DA_AuctionCatalog` (7 Auktionsvorlagen), `Audio/SFX_*` (5 synthetisch erzeugte Platzhalter-Sounds). Alles wird reproduzierbar von `Tools/Maps/build_placeholder_material.py`, `build_audio.py`, `build_item_definitions.py`, `build_procurement.py` (Beutetabellen), `build_economy.py`, `build_progression.py`, `build_workshop.py` (Möbelkatalog) und `build_garage_map.py` erzeugt (in dieser Reihenfolge; mit `-RenderOffscreen`, nicht `-nullrhi`, weil die Engine beim Platzieren von Lichtern mit NullRHI abstürzt).

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
| Firmenkonto, Marktnachfrage, Kundenaufträge, Werkstatt-Upgrades und Leitfaden (später Unternehmensstufe) | Komponenten an `AWEGameState` | alle |
| Spielername, persönliche Statistik | `AWEPlayerState` | alle |
| Spielerinventar | `UWEInventoryComponent` am `AWEPlayerState` | alle (Fast Array, nur Änderungen). Zugriff (verschieben/entnehmen) nur für den Besitzer, serverseitig geprüft. Eine Einschränkung der Sichtbarkeit auf den Besitzer ist für später vorgemerkt (bei 4 Spielern unkritisch). |
| Weltgegenstände | Item-Actor, `bReplicates`, Dormancy wenn ruhend | relevante Clients |
| Lager (Werkbank, Regal), später Maschinen | Actor + `UWEInventoryComponent` | relevante Clients; Zugriff nur in Reichweite |
| Sessionregeln | `AWEGameMode` | nur Server |

**Inventare:** `FFastArraySerializer`, damit nur geänderte Einträge übertragen werden. Alle Inventar-Anfragen laufen über den eigenen PlayerController (Clients dürfen keine RPCs auf fremden Actors aufrufen), der Server prüft Zugriff und Regeln. Getestet: Mengen bleiben über Spieler, Lager und Welt hinweg erhalten (keine Duplikate).

**Konflikte (Spec §17.6):** Server verarbeitet RPCs sequenziell im Game-Thread; jede Aktion prüft vor Ausführung erneut den aktuellen Zustand (z. B. "liegt der Gegenstand noch da / gehört er niemandem"). Getragene Gegenstände sind über `Holder` serverseitig reserviert und werden beim Verlassen freigegeben. Ein eingesteckter Gegenstand verschwindet im selben Server-Frame aus der Welt, eine zweite Anfrage findet ihn nicht mehr.

**Geld:** nur über `FWEMoney`, nur auf dem Server verändert, keine Float-Beträge (Preisfaktoren werden einmal pro Rechnung auf ganze Cent gerundet). Jede Kauf-/Verkaufs-/Auftragsanfrage trägt eine GUID; der Server merkt sich die letzten 512 und ignoriert Wiederholungen (im Netzwerktest geprüft).

**Performance:** keine Ticks für Maschinen/Items; stattdessen Timer und ein zentraler Simulations-Manager (Subsystem) mit festem Intervall. Net-Dormancy für ruhende Objekte, `NetUpdateFrequency` pro Klasse.

**Tests:** Ab Phase 1 wird jede Interaktion im PIE mit "Play As Listen Server" und 2 Spielern geprüft; ab Phase 2 gibt es automatisierte Netzwerk-Tests, sofern sie sich headless stabil ausführen lassen.

**Steam (Phase 14):** Online Subsystem Steam bzw. Steam Sockets über das Session-Interface; bis dahin LAN/IP über den Standard-NetDriver.

## 5. Speichersystem (umgesetzt in Phase 2)

- **Wer speichert:** nur der Host/Server (`FWESaveSystem`, aufgerufen von `AWEGameMode`). Clients sehen eine Meldung "Nur der Host kann speichern".
- **Wann:** F5 = Schnellspeichern (Slot `Quicksave`), F9 = Schnellladen, Konsole `WESave <Slot>` / `WELoad <Slot>` für beliebig viele benannte Spielstände, Autosave alle 300 s (Slot `Autosave`).
- **Was:** `UWESaveGame` (Version 7; ältere Stände bleiben lesbar, fehlende Felder = Standardwert, das Konto bleibt dann unverändert): Kartenname, Zeitpunkt, Spieler (Schlüssel "Host" bzw. Unique Net Id, Position, Blickrichtung, Inventar), alle Weltgegenstände (Item inkl. Zerlegefortschritt + Transform; getragene an der Ablegeposition; auf einer Werkbank liegende werden dort wieder abgelegt), alle Lager (über den stabilen Actor-Namen in der Map), Torzustände, Firmenkonto, Marktnachfrage, Kundenaufträge (Restzeit), Händlervorrat, gekaufte Upgrades und Leitfaden-Schritt (werden vor den Lagern angewendet, weil Upgrades Lagergrößen ändern), Reparaturstand jedes Gegenstands, Art und Kundengerät-Id von Aufträgen, gewürfelter Händlervorrat (Zustand/Qualität je Angebot), laufende Auktionen mit Gebot, Höchstbietendem und geheimem Limit (Spielstand gehört dem Host), Mindestqualität von Aufträgen, alle Möbel (Name, Katalog-Id, Position) – beim Laden zuerst wiederhergestellt, weil Lager und Gegenstände auf Werkbänken über Namen zugeordnet werden. Gegenstände werden über die stabile `ItemId` gespeichert, nicht über den Asset-Pfad; unbekannte Ids werden beim Laden übersprungen und geloggt.
- **Dateiformat:** `Saved/SaveGames/WorkshopEmpire/<Slot>.wesave` = Kopf (`WESV`, Formatversion, Größe, CRC32) + UE-SaveGame-Daten.
- **Schutz vor beschädigten Spielständen:** Schreiben in `<Slot>.tmp`, alter Stand wird zu `<Slot>.bak`, dann Umbenennen. Beim Laden werden Kopf, Größe und CRC geprüft, bevor deserialisiert wird; ist die Hauptdatei kaputt, wird die `.bak` geladen. Neuere Spielstandversionen und andere Karten werden mit Meldung abgelehnt. Ungültige Slot-Namen (z. B. `../`) werden abgelehnt.
- **Laden:** ersetzt alle Weltgegenstände, setzt Lager und Tore, stellt verbundene Spieler wieder her; später beitretende Spieler bekommen ihren Stand beim Login.
- **Migration:** `SaveVersion` wird geprüft; ein Migrationspfad für ältere Versionen kommt mit der ersten Formatänderung.

## 6. Datengetriebene Inhalte (umgesetzt in Phase 2)

- Statische Itemdefinitionen: `UWEItemDefinition` (Primary Data Asset `WEItem`, Asset-Manager-Verzeichnis `/Game/WorkshopEmpire/Data/Items`). Neue Gegenstände = neues Data Asset, kein C++.
- Iteminstanzen: `FWEItemInstance` (Definition + GUID + Menge + Zustand + Qualität), getrennt von der Definition gespeichert und repliziert.
- Inventargröße: derzeit 1 Platz pro Stapel; Rastergrößen pro Gegenstand sind nicht geplant, solange die Plätze-plus-Gewicht-Regel ausreicht.
- Vorschaubilder: Feld `Icon` vorhanden; bis es Icons gibt, zeigt die UI farbige Kacheln mit Namen und Zustandsstreifen.
- Zerlegeabläufe als Daten (`UWEDisassemblyDefinition`), seit Phase 3. Reparaturabläufe als Daten (`UWERepairDefinition`, `/Game/WorkshopEmpire/Data/Repair`), seit Phase 6. Ein laufender Arbeits- oder Reparaturschritt wird nicht gespeichert (nur abgeschlossene Schritte).
- Reparatur (Phase 6): Die Fehler eines kaputten Geräts werden einmal auf dem Server ausgewürfelt (bei Diagnose oder erstem Zerlegeschritt) und im Gegenstand gespeichert; der Client sieht sie über die normale Replikation des Gegenstands, das HUD zeigt sie erst nach der Diagnose. Ein Reparaturschritt läuft wie ein Zerlegeschritt über `FWEWorkbenchWork` (`RepairKind`, `RepairFault`) mit Server-Timer; Ersatzteile werden erst beim Abschluss des Schritts entnommen und dann erneut geprüft.
## 7. Rendering (Entscheidung Phase 0)

Engine-Standard von UE 5.7 bleibt aktiv: DX12, SM6, Lumen (GI + Reflexionen), Virtual Shadow Maps, Nanite-fähig, keine statische Beleuchtung. Grund: Innenräume (Garage, Hallen) profitieren stark von Lumen; Platzhaltergrafik bis Phase 15 (Spec §19). Bewertung mit Messungen erfolgt, sobald es eine Referenzszene gibt.

## 8. Build und Tests

```
F:\UE_5.7\Engine\Build\BatchFiles\Build.bat WorkshopEmpireEditor Win64 Development -Project="<Pfad>\WorkshopEmpire.uproject" -WaitMutex
F:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "<Pfad>\WorkshopEmpire.uproject" -ExecCmds="Automation RunTests WorkshopEmpire;Quit" -unattended -nopause -NullRHI -testexit="Automation Test Queue Empty" -ReportExportPath="<Pfad>\Saved\TestReports\<Name>"
```

Teststrategie siehe `TEST_REPORT.md`.
