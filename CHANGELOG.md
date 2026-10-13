# Changelog

## [0.6.0] – 2026-10-13 – Phase 6: Reparatursystem

### Neu
- Reparieren an der Werkbank mit der neuen Taste [Q]: Fehler suchen (Multimeter) → defektes Teil ausbauen → Ersatzteil einbauen → Funktionstest → optional reinigen und aufbereiten.
- Verborgene Fehler je Gerät (`UWERepairDefinition`, Data Assets `REP_OldComputer`, `REP_CrtMonitor`): beschädigte Geräte haben einen, defekte einen oder zwei; erst die Diagnose zeigt sie.
- Ersatzteile aus dem Inventar oder dem Werkbanklager – gekauft oder aus einem anderen Gerät. Beschädigte Ersatzteile können den Funktionstest nicht bestehen.
- Qualitätsverbesserung: Reinigungsset hebt die Qualität um 30 %, ab 80 % wird ein Gerät "Sehr gut".
- Neue Gegenstände: Multimeter, Lötkolben, Kondensatoren-Set, Reinigungsset. Neuer Händler "Ersatzteil-Express" (gebrauchte Netzteile, RAM, Grafikkarte, Mainboard, Kondensatoren, Reinigungssets).
- Reparaturaufträge: Kunde bringt sein Gerät (erscheint an der Lieferpalette), Rückgabe repariert in der Hand; Kundengeräte können nicht verkauft oder zerlegt werden.
- Untersuchen [F] und Werkbank zeigen den Reparaturstand ("Netzteil defekt (ausgebaut)" …).

### Balancing
- Alter Computer 75 € Grundwert, kaputt ×0,42 (defekt beim Händler 11,36 €); Röhrenmonitor 30 €, kaputt ×0,5.
- Gemessen pro PC: Zerlegen +5,81 €, Reparieren +8,20 €, Reparieren und Aufbereiten +17,70 €.
- Beim Zerlegen kommen bekannte kaputte Teile als "Defekt" heraus; ein für die Reparatur ausgebautes Teil fehlt beim Zerlegen (keine Verdopplung).

### Behoben
- Marktplatz zeigte direkt nach einem Kauf für ein Bild übereinanderliegende Zeilen.

### Technisch
- Speicherformat Version 5 (Reparaturstand jedes Gegenstands, Reparaturaufträge).
- Interaktions-Schnittstelle um eine dritte Aktion ([Q]) erweitert; 17 Tastenbelegungen.
- 7 neue Automation-Tests (insgesamt 53), darunter ein Balance-Test für Reparatur mit echten Spieldaten; Netzwerktest-Rollen K/L (Reparaturauftrag mit 2 Spielern); Screenshot-Lauf P6.
- Nur in Entwicklungs-Builds: `ServerDevAddContractOffer` für die Testautomatisierung.

## [0.5.0] – 2026-10-13 – Phase 5: Erster vollständiger Gameplay-Loop (Vertical Slice)

### Neu
- Werkstatt-Upgrades als Data Asset (`DA_UpgradeCatalog`, 6 Stück): LED-Beleuchtung, Werkbank-Unterschrank, zweites Regal, Werkzeuggürtel, Arbeitsmatte, Profi-Werkbank – teils mit Voraussetzung.
- Wirkungen: mehr Werkbankplätze, mehr Tragkraft für alle Spieler, schnelleres Zerlegen, hellere Lampen, das zweite Regal erscheint erst nach dem Kauf.
- Marktplatz: neue Spalte "Werkstatt verbessern" (gekauft / Voraussetzung / Kaufen).
- Leitfaden oben links: "Erste Schritte (1/6)" führt durch den ganzen Ablauf, danach "Nächstes Ziel: …".
- Knopf "Alles nehmen" in Lagern (z. B. alle Teile aus der Werkbank ins Inventar).

### Multiplayer und Sicherheit
- Upgrades kauft nur der Server (Reichweite, Voraussetzung, Doppelkauf, Geld, Anfrage-ID); sie gelten für das ganze Team und werden an alle repliziert. Im Koop als Listen-Server getestet.

### Behoben
- Gekaufte Werkzeuge zählten nicht für den ersten Leitfaden-Schritt.

### Technisch
- Speicherformat Version 4 (Upgrades, Leitfaden); ältere Spielstände bleiben ladbar.
- Neues Skript `Tools/Maps/build_progression.py`; Map um zweites Regal und markierte Werkstattlampen ergänzt.
- 5 neue Automation-Tests (insgesamt 46), darunter der komplette Ablauf mit echten Spieldaten; Netzwerktest-Rollen I/J (Listen-Server); Screenshot-Lauf P5 mit dem ganzen Ablauf.

## [0.4.0] – 2026-10-12 – Phase 4: Wirtschaft und Handel

### Neu
- Firmenkonto (`UWECompanyComponent`): 250 € Startkapital (Config), Cent-genau, nie negativ, letzte Buchungen; Anzeige oben rechts im HUD.
- Markt (`UWEMarketComponent`): Nachfrage je Kategorie (0,8–1,2) mit langsamer Schwankung; Preise aus Wert, Zustand, Qualität, Nachfrage und Händlerfaktor.
- 3 Händler als Data Assets: Schrotthandel Kowalski, Gebrauchtbörse 24, Werkzeug-Versand Hammerl – mit Ankaufskategorien, Angebot und Nachschub.
- Online-Marktplatz-Terminal in der Garage ([E]): verkaufen aus dem Inventar oder den getragenen Gegenstand (an den besten Bieter), kaufen (Kleinteile ins Inventar, Großes auf die neue Lieferpalette am Tor).
- Kundenaufträge (`UWEContractComponent`, Auftragskatalog mit 7 Vorlagen): 3 Angebote, annehmen, abliefern mit Mindestzustand, Fristen, Belohnung aufs Firmenkonto.
- Handelsbildschirm mit Spalten Verkaufen / Kaufen / Kundenaufträge und letzten Buchungen.

### Multiplayer und Sicherheit
- Alle Käufe, Verkäufe und Auftragsaktionen laufen über Server-RPCs mit Reichweiten-, Zugriffs-, Geld- und Vorratsprüfung.
- Schutz vor Doppelbuchungen: jede Anfrage trägt eine eindeutige ID, Wiederholungen werden ignoriert (im Netzwerk getestet).
- Konto, Nachfrage, Aufträge und Vorrat werden an alle Spieler repliziert.

### Balancing
- Zustandsfaktoren angepasst; Teile aus defekten Geräten kommen gleich gut oder besser heraus. Gemessen: defekten PC kaufen, zerlegen, verkaufen ≈ +6 € Gewinn.

### Technisch
- Speicherformat Version 3 (Konto, Markt, Aufträge, Händlervorrat).
- Neues Skript `Tools/Maps/build_economy.py`; Map um Terminal und Lieferpalette ergänzt.
- 8 neue Automation-Tests (insgesamt 41), darunter ein Balance-Test mit echten Spieldaten; Netzwerktest-Rollen G/H; Screenshot-Lauf P4.
- Nur in Entwicklungs-Builds: Teleport-RPC und Doppelsende-Hilfe für die Testautomatisierung.

## [0.3.0] – 2026-10-12 – Phase 3: Werkbank und Zerlegung

### Neu
- Werkbank `AWEWorkbench`: sperrigen Gegenstand mit [E] auf den Arbeitsplatz legen, mit [R] wieder herunternehmen; [E] auf der Werkbank öffnet weiterhin das Werkbanklager (jetzt 12 Plätze).
- Schrittweises Zerlegen: [E] auf den Gegenstand startet den nächsten Schritt; Hinweis zeigt Aktion und Werkzeug, z. B. "Gehäuseschrauben lösen (Schraubendreher)".
- Wiederverwendbare Zerlegeabläufe als Data Assets (`UWEDisassemblyDefinition`): Alter Computer (7 Schritte), Röhrenmonitor (4), Karton mit Kabeln (1).
- Werkzeuge mit Typ und Tempo: Schraubendreher, Seitenschneider (neu in der Garage), Akkuschrauber (×2,5, definiert für später).
- 9 neue Gegenstände: Grafikkarte, PC-Netzteil, Mainboard, Stahlschrott, Kunststoffteile, Glasbruch, Elektronikschrott, Seitenschneider, Akkuschrauber.
- Komponenten erben den Zustand der Quelle (±1 Stufe), Rohstoffe stapeln; Ergebnisse landen im Werkbanklager, Überschuss vor der Werkbank.
- Feedback: Fortschrittsbalken im HUD, Fortschrittsleiste an der Werkbank (orange/grün), Meldung mit den gewonnenen Teilen, 5 synthetisch erzeugte Sounds (Ablegen, Schrauben, Teil ab, fertig, Abbruch).
- Untersuchen zeigt Zerlegefortschritt und Werkzeugtempo; teilweise zerlegte Gegenstände heißen "… (teilweise zerlegt)".
- Neue Taste R für Zweitaktionen.

### Multiplayer
- Platzieren, Starten und Abschließen von Schritten laufen auf dem Server; ein Schritt gleichzeitig pro Werkbank; wer sich entfernt, bekommt kein Ergebnis.
- Repliziert werden nur der abgelegte Gegenstand und ein kleiner Arbeitszustand; Fortschritt, Sounds und Anzeige berechnet jeder Rechner selbst.

### Technisch
- Speicherformat Version 2: Zerlegefortschritt und Gegenstand auf der Werkbank; Version-1-Stände bleiben lesbar.
- Neues Skript `Tools/Maps/build_audio.py`; Item-Skript legt jetzt auch Zerlegeabläufe an; Map nutzt die neue Werkbank und ein angepasstes Layout.
- 7 neue Automation-Tests (insgesamt 33), Netzwerktest-Rollen E/F, Screenshot-Lauf P3.

### Fehlerbehebungen (während der Phase)
- Kein Hinweis beim Anschauen eines Gegenstands auf der Werkbank (Kollision war aus).
- Lose Gegenstände auf der Werkbank verhinderten das Ablegen.
- Fortschrittsbalken bei anderen Spielern sprang durch abweichende Serverzeit zu früh auf 100 %.
- Doppelte Namen in der Teile-Meldung, schlecht lesbarer Text auf hellen Inventarkacheln, Knackser in den Sounds.

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
