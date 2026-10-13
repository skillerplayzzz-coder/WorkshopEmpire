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

## Phase 5 – Lauf vom 2026-10-13

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **46/46 bestanden, 0 mit Warnungen** (41 alte + 5 neue), Bericht `Saved/TestReports/Phase5/index.json` |
| ↳ `Loop.FullVerticalSlice` (**echte Spieldaten**, dieselben Server-Funktionen wie die Spieleranfragen): 250 € → Schraubendreher, Seitenschneider und defekten PC kaufen → PC zur Werkbank tragen → alle 7 Schritte zerlegen → "Alles nehmen" → alle Teile verkaufen → Werkbank-Unterschrank kaufen → speichern → Konto, Upgrades und Leitfaden zurücksetzen → laden | ✅ 250,00 → 213,15 → 234,36 € (**+21,21 € durch die Teile**) → Upgrade −35 € (+6 Werkbankplätze); nach dem Laden Konto, Upgrade, Wirkung und Leitfaden wieder da |
| ↳ ebenda: Upgrade-Ablehnungen (schon gekauft, Voraussetzung fehlt, unbekannt, zu wenig Geld, wiederholte Anfrage-ID) | ✅ keine davon kostet Geld; nur das eine erlaubte Upgrade wurde abgebucht |
| ↳ `Progression.UpgradeEffectsAndRules` (+6 Werkbankplätze, +10 kg Inventar, Arbeitstempo ×1,25 → 2,5 s werden 2,0 s, Lampen ×2 ohne Aufaddieren beim erneuten Anwenden, Voraussetzung, kein Doppelkauf) | ✅ |
| ↳ `Progression.FurnitureUnlock` (zweites Regal vor dem Kauf unsichtbar und ohne Lager, danach sichtbar und nutzbar) | ✅ |
| ↳ `Progression.GuideFollowsEvents` (6 Schritte nur in der richtigen Reihenfolge, danach "Nächstes Ziel") | ✅ |
| ↳ `Inventory.TransferAllKeepsWhatDoesNotFit` (nimmt, was passt; Rest bleibt; nichts geht verloren, Gewichtsgrenze hält) | ✅ |
| Erzeugung der Upgrade-Daten (6 Upgrades) und der Map (zweites Regal, markierte Lampen) per Skript | ✅ |
| **Spiel mit Grafik, kompletter Ablauf** (`-WEShots=P5`, 2 Läufe) | ✅ 10 Screenshots geprüft: Werkzeuge einstecken → Computer zur Werkbank tragen → 7 Schritte zerlegen → Werkbanklager → "Alles nehmen" → Marktplatz mit Spalte "Werkstatt verbessern" → alle Teile verkauft (+11,36 € bzw. +17,51 €, je nach Zufall der Teile) → LED-Beleuchtung und zweites Regal gekauft (Regal erscheint, Lampen doppelt so hell) → speichern → laden. Leitfaden zählte jeden Schritt korrekt bis 6/6 und zeigt danach "Nächstes Ziel: Werkbank-Unterschrank (35,00 €)". |
| **Netzwerktest Koop als Listen-Server** (`-WESmokeRole=I` Host / `J` Client, neu) | ✅ Host kauft LED + Regal → Client sieht 160,00 €, beide Upgrades, Regal sichtbar, Lampe 60 statt 30. Client versucht LED erneut → "Bereits gekauft", kein Geld weg. Client kauft Werkzeuggürtel → Host und Client: 90,00 €, Tragkraft beider Spieler 35 kg. Host speichert ("Gespeichert: DevNetP5", auch beim Client gemeldet). |
| **Regression Netzwerktests A/B, C/D, E/F, G/H** (Dedicated Server + 2 Clients) | ✅ unverändert (50 Schrauben / 1 RAM bleiben erhalten, doppelte Anfrage einmal gebucht, Motor nur einmal verkauft) |

### Behobene Fehler in Phase 5

| Fehler | Ursache | Behebung |
|---|---|---|
| Leitfaden blieb bei Schritt 1 ("Werkzeug einstecken") stehen, wenn das Werkzeug gekauft statt aufgehoben wurde (vom Loop-Test gefunden) | Gekaufte Kleinteile landen direkt im Inventar, nur das Aufheben zählte | Ein gekauftes Werkzeug im Inventar zählt genauso |
| Dritte Zeile des Leitfadens wurde abgeschnitten (Screenshot) | Feste Höhe des Kastens | Kasten wächst mit dem Text |
| Test-Skript legte den Computer neben die Werkbank | Seit dem Umbau in Phase 4 steht man beim Start ~1,9 m weit weg | Skript geht vorher zur Werkbank (nur der Testablauf) |
| Test `Save.EconomyRoundTrip` erwartete Version 3 | Speicherversion ist jetzt 4 | prüft die aktuelle Version |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Kompletter Ablauf mit echter Maus und Tastatur | Mikes Praxistest; die Knöpfe rufen dieselben Funktionen wie die getesteten |
| Speichern/Laden mit zwei Spielern **mit Grafik** | Nur headless Listen-Server; Laden im Koop nicht ausgelöst (nur Speichern) |
| Upgrades Arbeitsmatte und Profi-Werkbank im Spiel | Nur im Unit-Test (Tempo ×1,25); im Spiel erst nach mehreren Durchläufen erreichbar |
| Spielstand aus Phase 4 (Version 3) im Spiel laden | Migration per Code (fehlende Felder → keine Upgrades); nicht mit einer echten alten Datei geprüft |

### Offene Probleme

Keine bekannten Fehler. Preise der Upgrades sind erste Werte (6–21 € Gewinn pro PC → 2–10 PCs pro Upgrade).

---

## Phase 4 – Lauf vom 2026-10-12

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **41/41 bestanden, 0 mit Warnungen** (33 alte + 8 neue), Bericht `Saved/TestReports/Phase4/index.json` |
| ↳ `Economy.CompanyAccount` (Start 250 €, nie unter 0, negative Beträge abgelehnt, Buchungsliste, doppelte/ungültige Anfrage-ID abgelehnt) | ✅ |
| ↳ `Economy.PricingRules` (Neu 10 € → Händler zahlt 6,00 €, Nachfrage 1,2 → 7,20 €, Zustand, Begrenzung 0,8–1,2 auch nach 200 Schritten, Kategorie ohne Käufer = 0, teilzerlegt = 0, Kaufpreis > Verkaufspreis) | ✅ |
| ↳ `Economy.SellFromInventory` (4 Schrauben = +2,40 €, gleiche Anfrage zweimal = einmal gebucht, fremdes Inventar, zu weit weg) | ✅ |
| ↳ `Economy.BuyStockAndDelivery` (RAM 13,00 € ins Inventar, PC auf die Palette, Vorrat, ausverkauft, zu wenig Geld ändert nichts) | ✅ |
| ↳ `Economy.SellCarriedItem`, `Economy.CustomerOrders` (Angebote, Belohnung, Annehmen, Mindestzustand, Abliefern, Limit 3, Ablauf) | ✅ |
| ↳ `Economy.BalanceDefectivePcIsProfitable` (echte Spieldaten, 30 Durchläufe: kaufen → zerlegen → verkaufen) | ✅ Kosten 14,43 €, Teile im Schnitt 20,57 €, **Gewinn 6,14 €** |
| ↳ `Save.EconomyRoundTrip` (Version 3: Konto, Nachfrage, Vorrat, angenommener Auftrag) | ✅ |
| Erzeugung der Händler- und Auftragsdaten und der Map per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P4`): Terminal → Handel öffnen → 50 Schrauben und 2 Kabel verkaufen → defekten PC und Akkuschrauber kaufen → Auftrag annehmen → Lieferpalette | ✅ 6 Screenshots geprüft; Konto 250,00 → 252,56 → 169,39 €, PC steht auf der Palette, Akkuschrauber im Inventar |
| **Netzwerktest Wirtschaft** (`-WESmokeRole=G/H`) | ✅ G verkauft 10 Schrauben mit **derselben Anfrage-ID zweimal** → Server bucht einmal (+0,28 €) und meldet die Wiederholung; H sieht denselben Kontostand. Beide kaufen gleichzeitig den **einzigen Elektromotor** → nur G bekommt ihn, H erhält "Nicht mehr vorrätig", Konto −18,95 € einmal, 2 Motoren in der Welt (Original + 1 gekaufter). G nimmt einen Auftrag an, H sieht "angenommen von …". |
| **Regression Netzwerktests A/B, C/D, E/F** | ✅ unverändert |

### Behobene Fehler / Anpassungen in Phase 4

| Problem | Ursache | Behebung |
|---|---|---|
| Defekten PC kaufen und zerlegen hätte nach meiner Überschlagsrechnung Verlust gebracht (~14 € Einkauf, ~13 € Teile) | Teile erbten den schlechten Zustand des Geräts | Teile kommen gleich gut oder bis 2 Stufen besser heraus; Zustandsfaktoren angepasst (Defekt 0,3, Schrott 0,1). Jetzt per Test gemessen: +6,14 € im Schnitt |
| Test `Save.WorkbenchProgress` erwartete Version 2 | Speicherversion ist jetzt 3 | prüft die aktuelle Version |
| Compilerfehler (`Slot` verdeckt UWidget-Member, Referenz auf `TObjectPtr`) | – | umbenannt / Signatur angepasst |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Klicken mit echter Maus im Handelsbildschirm | Mikes Praxistest; die Knöpfe rufen dieselben Funktionen wie die getesteten |
| Nachschub nach 10–30 Minuten, Nachfrageänderung nach 5 Minuten, Auftragsfrist im Spiel ablaufen lassen | Nicht abgewartet; dieselben Funktionen sind in Unit-Tests direkt aufgerufen |
| Aufträge im Netzwerk abliefern | Nur Annehmen im Netzwerktest; Abliefern im Unit-Test |

### Offene Probleme

Keine bekannten Fehler. Die Preise sind erste Werte; das Balancing über viele Spielstunden folgt in Phase 16.

---

## Phase 3 – Lauf vom 2026-10-12

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **33/33 bestanden, 0 mit Warnungen** (26 alte + 7 neue), Bericht `Saved/TestReports/Phase3/index.json` |
| ↳ `Workshop.Disassembly.FullSequence`: ablegen, Schritt per [E], Werkzeug-Prompt, alle 3 Schritte, Teile im Werkbanklager, Zustand der Komponente ±1 Stufe zur Quelle, Gegenstand danach verbraucht | ✅ |
| ↳ `Workshop.Disassembly.ToolsRequiredAndFaster` (ohne Schraubendreher abgelehnt mit Begründung, Hände immer verfügbar, Akkuschrauber 2 s → 0,8 s) | ✅ |
| ↳ `Workshop.Disassembly.BusyAndReach` (zweiter Spieler kann nicht parallel arbeiten oder herunternehmen; wer weggeht, bekommt kein Ergebnis) | ✅ |
| ↳ `Workshop.Disassembly.TakeOffKeepsProgress` ([R] nimmt herunter, Fortschritt bleibt am Gegenstand, später geht es bei Schritt 2 weiter) | ✅ |
| ↳ `Workshop.PlaceByAimingAndRules` (Ablegen beim Zielen auf die Werkbank, nicht zerlegbare Kiste wird normal abgelegt, belegte Werkbank lehnt ab) | ✅ (erst rot, siehe Behobene Fehler) |
| ↳ `Workshop.Disassembly.OverflowToWorld`, `Save.WorkbenchProgress` (Speicherversion 2, Gegenstand wieder auf der Werkbank, Fortschritt und Lager wiederhergestellt) | ✅ |
| Erzeugung von 5 Sounds, 18 Item-Definitionen, 3 Zerlegeabläufen und der Map per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P3`): Computer tragen → auf Werkbank legen → 7 Schritte automatisch → Werkbanklager öffnen | ✅ 7 Screenshots geprüft (Prompt "[E] Gehäuseschrauben lösen (Schraubendreher)" / "[R] Von der Werkbank nehmen", Fortschrittsbalken mit Prozent, orange Leiste an der Werkbank, Teile im Lager). 9 kg Computer → 7–9 kg Teile, je nach Zufall (Grafikkarte erscheint mit 85 % Wahrscheinlichkeit; in einem Lauf fehlte sie, dann kommt die Meldung "Nichts Brauchbares dabei"). |
| **Netzwerktest Werkbank** (`-WESmokeRole=E/F`, 2 Läufe) | ✅ E trägt, legt ab und zerlegt alle 7 Schritte; beide Clients sehen dasselbe Werkbanklager. F versucht während der Arbeit zu starten und herunterzunehmen → Server lehnt ab ("Wird gerade bearbeitet" / "not available"). Im 2. Lauf traf F's Versuch die halbe Sekunde zwischen zwei Schritten und F führte Schritt 2 (ohne Werkzeug) regulär aus – gewolltes Koop-Verhalten, keine Duplikate. |
| **Regression Netzwerktests Phase 1 (A/B) und Phase 2 (C/D)** | ✅ unverändert; 50 Schrauben / 1 RAM bleiben erhalten |

### Behobene Fehler in Phase 3

| Fehler | Ursache | Behebung |
|---|---|---|
| Kein Prompt beim Anschauen des Computers auf der Werkbank (Screenshot) | Abgelegte Gegenstände hatten Kollision ganz aus, das Fadenkreuz ging hindurch | Auf der Werkbank `QueryOnly` (anvisierbar, keine Physik) |
| Ablegen auf die Werkbank klappte nicht, wenn ein loser Gegenstand im Blickfeld auf der Werkbank lag (Test rot) | Zielerkennung traf zuerst den losen Gegenstand | lose Gegenstände werden beim Zielen übersprungen; HUD-Prompt nutzt dieselbe Prüfung wie der Server |
| Fortschrittsbalken beim zweiten Spieler sprang zu früh auf 100 % (Netzwerktest) | Die geschätzte Serverzeit des Clients wich um Sekunden ab | Fortschritt wird lokal ab Eintreffen des Schrittstarts gemessen |
| Test-Zielen funktionierte ohne Controller nicht | Ohne Controller nutzt die Server-Sicht die Actor-Rotation | Test setzt die Rotation |
| Ensure "Decoder BINKA not found" beim Sound-Import, Knackser-Warnung | Import mit `-NoSound`; Sounds endeten nicht bei null | Import mit Audio, Ein-/Ausblenden der erzeugten Sounds |
| Meldung listete gleiche Teile doppelt ("Arbeitsspeicher, Arbeitsspeicher"); dunkler Text auf hellen Kacheln schwer lesbar | – | Mengen zusammengefasst ("2× …"); Textfarbe je nach Kachelhelligkeit |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Sounds anhören | Automatisch nicht prüfbar – die Sounds werden abgespielt (Code), ob sie gut klingen, entscheidet Mike |
| Monitor- und Kabelkarton-Zerlegung im Spiel | Nur Daten angelegt; der Ablauf ist derselbe Code wie beim Computer (in Tests und im Spiel geprüft) |
| Speichern während ein Schritt läuft | Laufender Schritt wird bewusst nicht gespeichert (Abschnitt 5 der Architektur); nicht separat getestet |

### Offene Probleme

Keine bekannten Fehler. Hinweis aus dem Phase-2-Regressionstest: Ein Client kann für einen Moment (bis zum nächsten Netzwerk-Update) einen Gegenstand gleichzeitig im alten und im neuen Lager sehen, weil Spielerinventar und Werkbank getrennt repliziert werden. Auf dem Server und eine Sekunde später auch auf dem Client stimmen die Mengen (50 Schrauben).

---

## Phase 2 – Lauf vom 2026-10-12

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **26/26 bestanden, 0 mit Warnungen** (14 alte + 12 neue), Bericht `Saved/TestReports/Phase2/index.json` |
| ↳ `Inventory.StackingAndSplitStacks`: 120 Schrauben → Stapel 50/50/20, Teilstapel wird zuerst aufgefüllt, erste Instanz-ID bleibt | ✅ |
| ↳ `Inventory.DifferentConditionsDoNotStack`, `Inventory.SlotAndWeightLimits` (Platz voll, 5×3 kg in 10 kg → 3, sperrige Gegenstände abgelehnt) | ✅ |
| ↳ `Inventory.MoveMergeSwapSplit` (teilen, zusammenführen, tauschen, zwischen Inventaren, Gewichtsgrenze, ungültige Plätze), `Inventory.SortAndRemove` | ✅ |
| ↳ `Inventory.WorldToInventory` (Stapel einsammeln, ID und Zustand bleiben, Teilaufnahme bei Gewichtsgrenze, Rest bleibt liegen) | ✅ |
| ↳ `Inventory.TwoPlayersSameItem` (zwei Anfragen im selben Server-Frame: genau ein Exemplar), `Inventory.AccessRules` (fremdes Inventar privat, Lager nur in Reichweite) | ✅ |
| ↳ `Items.ValueAndTooltip` (Wert: Neu > Gebraucht > Schrott, Qualität, Menge; Tooltip; `FindById`) | ✅ |
| ↳ `Save.RoundTripWorldContainersDoors` (Welt-Items mit ID/Zustand, Lagerinhalt, Tor; zwischendurch Geändertes wird zurückgesetzt) | ✅ |
| ↳ `Save.CorruptFileFallsBackToBackup` (`.bak` entsteht, kein `.tmp` bleibt, 1 gekipptes Byte → CRC erkennt es → Backup; beide kaputt → saubere Fehlermeldung) | ✅ |
| ↳ `Save.VersionAndValidation` (neuere Version abgelehnt, Slot-Namen wie `../evil` abgelehnt, unbekannte Item-ID übersprungen) | ✅ |
| Erzeugung von 9 Item-Data-Assets und der Map per Skript | ✅ (siehe Behobene Fehler) |
| **Netzwerktest Inventar** (Dedicated Server + 2 Clients, `-WESmokeRole=C/D`) | ✅ siehe unten |
| **Regression Netzwerktest Phase 1** (`A/B`: Tragen, Konflikt, Ablegen, Übergabe, Reichweite, Verlassen) | ✅ identisches Ergebnis wie in Phase 1 |
| **Spiel mit Grafik + Speichern/Laden** (`-WEShots=P2`, Einzelspieler als Host) | ✅ 6 Screenshots geprüft; Ablauf siehe unten |

Netzwerktest Inventar (gekürzt; Gesamtzahl Blechschrauben über alle Inventare + Welt in jedem Schritt = 50, Arbeitsspeicher = 1):

```
Server: WECharacter_0 stored 50 x Blechschrauben          ← C nimmt den Stapel
Server: WECharacter_1 stored 6 x Kupferkabel              ← D nimmt Kabel
C2:    Mine 45, Werkbank 5                                 ← C legt 5 in die Werkbank, D sieht dieselben 5
Server: WEPlayerController_0 dropped 3 x Blechschrauben   ← C legt 3 in die Welt
Server: WEPlayerController_1 denied access to inventory …WEPlayerState_0.Inventory  ← D versucht, aus C's Inventar zu nehmen
Server: WECharacter_1 stored 3 x Blechschrauben           ← D nimmt die abgelegten 3
Server: WECharacter_0 stored 1 x Arbeitsspeicher          ← C bekommt den RAM, D findet ihn nicht mehr
Ende:  C 42 + Werkbank 5 + D 3 = 50; RAM nur bei C
```

Speichern/Laden im Spiel: 4 Kleinteile eingesammelt → Kupferkabel in die Werkbank → F5-Äquivalent (Slot `DevTest`, "Gespeichert") → 50 Schrauben in die Welt geworfen → Laden ("Geladen: DevTest") → Schrauben wieder im Inventar, abgelegter Stapel weg, Kabel weiter in der Werkbank, Summe weiterhin 50.

### Behobene Fehler in Phase 2

| Fehler | Ursache | Behebung |
|---|---|---|
| Linker-Fehler `FUniqueNetIdWrapper::ToString` | Modul `CoreOnline` fehlte | Abhängigkeit ergänzt |
| Asset-Manager-Fehler "PrimaryAssetId does not match" beim Anlegen der Data Assets | Primary Asset Id hing an `ItemId`, die erst nach dem Anlegen gesetzt wird | Primary Id = Asset-Name, Suche über `ItemId` |
| Lange Namen liefen im Inventar in die Nachbarkachel; Welt-Hinweis schien durch das offene Inventar | Kein Clipping, HUD ignorierte offenes Menü | Clipping + Umbruch; Hinweise ausgeblendet, solange das Inventar offen ist |
| Test `CorruptFileFallsBackToBackup` meldete eine Warnung | Die Backup-Warnung ist dort gewollt | als erwartete Meldung deklariert |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Drag & Drop mit echter Maus (Ziehen, Shift, Doppelklick, Rechtsklick, aus dem Fenster ziehen) | Braucht einen Menschen – Mikes Praxistest. Die Server-Funktionen dahinter sind getestet; die Screenshots zeigen die Oberfläche. |
| Speichern/Laden mit zwei verbundenen Spielern | Nur Einzelspieler-Host und Unit-Tests; Clients dürfen nicht speichern (per Code) |
| Autosave nach 300 s | Nicht abgewartet; nutzt dieselbe Funktion wie das getestete Speichern |
| RAM-"Wettlauf" exakt gleichzeitig über das Netzwerk | Im Netzwerktest kam C ~1 s vor D an; der gleichzeitige Fall ist nur im Unit-Test (selber Server-Frame) geprüft |

### Offene Probleme

Keine bekannten Fehler. Hinweise: Lange Namen werden in den Kacheln zeichenweise umbrochen (bis es Icons gibt). Spielerinventare werden an alle Clients repliziert (Zugriff aber nur für den Besitzer).

---

## Phase 1 – Lauf vom 2026-10-12

Umgebung wie Phase 0. Alle Läufe auf dem Entwicklungs-PC.

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build `WorkshopEmpireEditor Win64 Development` | ✅ 0 Warnungen/Fehler im Projektcode |
| Build `WorkshopEmpire Win64 Development` (Game) | ✅ erfolgreich |
| Automation-Suite `WorkshopEmpire` (headless, `-NullRHI`) | ✅ **14/14 bestanden, 0 mit Warnungen**, Bericht `Saved/TestReports/Phase1/index.json` |
| ↳ `Core.Money.*` (4), `Framework.GameModeDefaults` (jetzt inkl. Pawn = `AWECharacter`) | ✅ Regression ok |
| ↳ `Gameplay.Carry.PickupConflictDropHandOver`: A hebt auf → B wird abgelehnt → A legt ab → B hebt auf | ✅ |
| ↳ `Gameplay.Carry.HandsFull`, `Gameplay.Carry.WeightLimit` (45 kg abgelehnt, genau 30 kg erlaubt) | ✅ |
| ↳ `Gameplay.Carry.DropWhenPlayerLeaves` (Charakter zerstört → Gegenstand frei, Physik an) | ✅ (deckte zuerst eine Lücke im Test-Harness auf: Testwelt startete kein BeginPlay; behoben, danach grün) |
| ↳ `Gameplay.Carry.DropStopsBeforeWall` (frei: 110 cm vor dem Spieler; Wand 2 m: direkt davor; Wand 60 cm: nie dahinter) | ✅ |
| ↳ `Gameplay.Interaction.ServerReachValidation` (10 m entfernt abgelehnt, 1,5 m angenommen, `nullptr` abgelehnt) | ✅ |
| ↳ `Gameplay.Door.ToggleViaInteraction`, `Input.DefaultMapping` (11 Belegungen, E/F/Shift/Leertaste), `UI.PromptFormat` | ✅ |
| Map-Erzeugung per Skript (`build_placeholder_material.py`, `build_garage_map.py`) | ✅ nach zwei Korrekturen (siehe Behobene Fehler) |
| Spiel mit Grafik (`-game`, 1280×720) + automatisierte Screenshots `-WEShots=P1` | ✅ 8 Screenshots geprüft: Werkbank, Tor, Seite, Fokus "[E] Aufheben / [F] Untersuchen", Getragen "[E] Ablegen" + "In der Hand: Alter Computer", Untersuchen-Panel (Name, Beschreibung, 9,0 kg), Abgelegt, Motor "Zu schwer zum Tragen (45,0 kg)". Keine Warnungen/Fehler aus Projektcode im Log. |
| **Netzwerk-Rauchtest Phase 1:** headless Dedicated Server auf `L_Garage` + 2 headless Clients (`-WESmokeRole=A/B`), alle Aktionen über die echten Server-RPCs | ✅ siehe unten |

Netzwerk-Rauchtest im Detail (Server-Log, gekürzt):

```
Player joined (1/4), Player joined (2/4)
WECharacter_0 interaction with WEGarageDoor_0 rejected: out of reach      ← A versucht Tor aus ~5 m
WECharacter_0 picked up WEPickupItem_0                                     ← A hebt Computer auf
WECharacter_1 interaction with WEPickupItem_0 rejected: Wird gerade getragen ← B gleichzeitig: abgelehnt
WECharacter_0 dropped WEPickupItem_0 at (-377.5, -40.0, 160.2)             ← A legt auf Werkbank ab
WECharacter_1 picked up WEPickupItem_0                                     ← B übernimmt
Player left … / WECharacter_1 dropped WEPickupItem_0 …                     ← B trennt Verbindung: Gegenstand bleibt in der Welt
```

Client-Sicht: B sieht während A trägt `Holder=<A>` und dieselbe Position wie A (‑265, ‑62, 114); nach dem Ablegen sehen beide den Computer auf der Werkbank (‑375, ‑41, 118,6); Tor bleibt auf beiden Clients `closed`. Logs: `Saved/Logs/P1Net*.log`.

### Behobene Fehler in Phase 1

| Fehler | Ursache | Behebung |
|---|---|---|
| Test `DropWhenPlayerLeaves` schlug fehl | Test-Welt ohne GameMode startete kein BeginPlay, daher kein EndPlay | Test-Harness ruft `NotifyBeginPlay`/`EndPlay` auf |
| Editor-Absturz beim Map-Skript mit `-nullrhi` (Division durch 0 beim Platzieren eines PointLight) | Engine-Problem mit NullRHI | Skripte laufen mit `-RenderOffscreen` |
| Editor-Absturz beim Neuerzeugen der Map | L_Garage ist Startmap und war geladen, als sie gelöscht wurde | Skript lädt vorher eine leere Engine-Map |
| Platzhalter-Farben fehlten (alles grau kariert) | Engine-Würfelmaterial hat keinen Farbparameter; danach fehlte `bUsedWithInstancedStaticMeshes` | eigenes `M_WEPlaceholder` mit Flag |
| Szene überbelichtet, große Gegenstände verdeckten beim Tragen die Sicht | Belichtung / Haltepunkt | Belichtung −0,6, Wandfarben dunkler; große Gegenstände werden weiter vorn/unten gehalten |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Bedienung mit echter Maus/Tastatur (Gefühl von Bewegung, Sprint, Ducken, Kamera) | Braucht einen Menschen – Mikes Praxistest |
| PIE mit 2 Spielern **mit Grafik** im Editor | Mikes Praxistest; Netzwerklogik ist durch den headless Test abgedeckt |
| Sprint/Ducken im Netzwerk (Korrekturen sichtbar?) | Nur per Code geprüft; im Rauchtest nicht geskriptet |
| Tor öffnen über das Netzwerk (Replikation der Animation) | Im Rauchtest nur die Reichweiten-Ablehnung; Öffnen selbst nur im Unit-Test (Standalone) |
| Gleichzeitige Anfragen im selben Frame | Server verarbeitet RPCs nacheinander; der zeitversetzte Konflikt ist getestet, exakt gleichzeitige nicht |
| Performance-Messung | Ab Phase 9 geplant |

### Offene Probleme

Keine bekannten Fehler. Hinweis: Beim Start jedes Editors stehen 6× `LogAutomationTest: Error: Condition failed` im Log, noch bevor Projekttests laufen. Das trat bereits in Phase 0 auf (nachträglich im Phase-0-Log geprüft), kommt aus der Engine-Initialisierung und beeinflusst die Projekttests nicht. Es war im Phase-0-Bericht nicht erwähnt.

### Manueller Test für Mike (Phase 1)

Siehe Development Report Phase 1 (gleicher Inhalt, Schritt für Schritt).

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
