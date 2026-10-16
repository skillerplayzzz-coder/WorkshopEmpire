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

## Phase 12 – Lauf vom 2026-10-16

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **95/95 bestanden, 0 mit Warnungen** (88 alte + 7 neue), Bericht `Saved/TestReports/Phase12_final/index.json` |
| ↳ `Expansion.TiersAndBuildings` (Halle mit Tag versteckt und ohne Kollision, Baufläche gesperrt; Stufe 1, 6 Mitarbeiter, 11 kW, nächste Stufe nennt die Erweiterung; Halle braucht erst die Erweiterung; Erweiterung → Stufe 2, 8 Mitarbeiter; Halle → Stufe 3, 12 Mitarbeiter, 38,5 kW, sichtbar, Kollision, Baufläche gilt; Firmenwert = Konto + halber Gebäudepreis; Fabrik → Stufe 4, nächste Stufe "Firmenwert"; 250.000 € → Stufe 5) | ✅ (erst rot, siehe Behobene Fehler) |
| ↳ `Expansion.LockedFurniture` (Schmelzofen ohne Halle: "Benötigt: Gewerbehalle", Aufstellen abgelehnt, nichts bezahlt; nach der Halle aufgestellt) | ✅ (erst rot) |
| ↳ `Expansion.CompanyBooks` (Zuordnung der Buchungstexte zu Bereichen; Summen je Bereich und letzte Stunde; Laden stellt die Summen wieder her) | ✅ |
| ↳ `Expansion.IndustrialOrders` (keine Industrieaufträge in der kleinen Garage, ab Stufe 2 ja; 2 Industrie- + 3 normale Angebote; Liefervertrag 3 Lieferungen: erste Lieferung aus dem Laderaum bezahlt, 1/3, Palette verbraucht; leer → "fehlt"; Frist verpasst → Strafe = ¼ Belohnung, Vertrag weg, als "Aufträge" gebucht; Tour "Auslieferung": Fahrer liefert die Stahlballen für das Stahlwerk, Auftrag erledigt, Ballen weg, Ergebnis nennt den Auftrag) | ✅ |
| ↳ `Machines.FurnaceAndPacker` (15-kW-Ofen läuft am Hallenanschluss; 10 Aluschrott → 2 Barren, 20 Kupfergranulat → 2 Barren; 20 Stahlwinkel-Sets → 1 Palette vor der Anlage, sperrig) | ✅ |
| ↳ `Save.ExpansionRoundTrip` (Version 11: Liefervertrag mit 1/3 Lieferungen, Industrie-Flag, Stufe aus den Gebäuden, Buchhaltung) | ✅ |
| ↳ `Economy.BalanceExpansion` (**echte Daten**) | ✅ Aluminium ×1,97 (5,54 € → 10,89 €), Kupfer ×1,94 (27,72 € → 53,72 €), Palette Stahlwinkel ×1,14, Palette Kabel ×1,11. Aufträge zahlen mehr als der Händler: Stahlwerk 73,50 € statt 50,82 €, Kabelwerk 194,25 € statt 134,31 €, Liefervertrag Kabel 101,50 € je Lieferung statt 66 €. Halle + Ofen (27.800 €) mit Kupfer allein nach ~1.070 Ofenläufen bezahlt |
| Erzeugung von 49 Gegenständen (4 neue), 6 Maschinen (2 neue), 18 Möbeln (4 neue), Upgrades (3 Gebäude), 6 Industrieauftragsvorlagen, Tour "Auslieferung" und Map mit 3 versteckten Gebäuden per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P12`, 2 Läufe) | ✅ 10 Screenshots geprüft: Hof vorher; Erweiterung und Gewerbehalle gekauft → Reiter "Unternehmen" (Stufe 3, nächste Stufe, Firmenwert, Strom, Personal, Ausgaben je Bereich); Halle und Anbau stehen auf dem Gelände; Halle innen mit Schmelzofen, Verpackungsanlage, Großem Materiallager, Hochregal; Ofen schmilzt Aluminium (2 Barren); Industrieauftrag "★ Industrie" angenommen; Fabrikhalle mit Meldung "neue Unternehmensstufe: Fabrik!"; Unternehmen mit Stufe 4; gespeichert und geladen → Gebäude, Stufe, Buchhaltung, Aufträge unverändert |
| **Netzwerktest Expansion** (`-WESmokeRole=W/X`, Dedicated Server + 2 Clients, neu, 2 Läufe) | ✅ W kauft Erweiterung und Halle, nimmt einen Industrieauftrag an, stellt einen Schmelzofen in die Halle und lässt ihn laufen; X sieht vorher alle Gebäude versteckt (Stufe 1), danach sichtbar mit Stufe 3, 38,5 kW, 12 Plätzen, Buchhaltung, Auftrag und laufendem Ofen, läuft in die Halle |
| **Regression** Netzwerktests A/B … U/V (Dedicated) und I/J (Listen-Server), Spielläufe P5–P11 | ✅ unverändert (alle Läufe beendet, 0 Fehler im Log; Baumodus-Menü zeigt gesperrte Einrichtung mit "Benötigt: …") |

### Behobene Fehler in Phase 12

| Fehler | Ursache | Behebung |
|---|---|---|
| Käufer sah die Meldung "neue Unternehmensstufe" nicht | Die Kaufmeldung "Werkstatt verbessert" ersetzte sie sofort | Kaufmeldung nennt die neue Stufe mit |
| Hochregal ließ sich in der Halle nicht aufstellen ("Nur innerhalb der Werkstatt") | Testskript: gedreht ragte es über die Baufläche hinaus (richtig abgelehnt) | Position im Skript korrigiert |
| Zwei Testerwartungen falsch (Stufe 4, Meldungstext) | Tests vergaben Gebäude ohne zu bezahlen, das Konto war schon Firmenwert für Stufe 5 | Konto im Test gesenkt; Erwartung an eine Lognachricht entfernt |
| Kompilierfehler double → float in der Buchhaltung | Implizite Umwandlung | Explizite Umwandlung |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Liefervertrag über alle Lieferungen im Spiel | Im Unit-Test (Lieferung, Strafe, Tour "Auslieferung"); im Spiel nur Annehmen |
| Stufe 5 (250.000 € Firmenwert) im Spiel erspielt | Im Unit-Test; im Spiel nur bis Stufe 4 mit Dev-Geld |
| 20 Mitarbeiter gleichzeitig / Performance in der Fabrik | nicht gemessen |

### Offene Probleme

Keine bekannten Fehler. Hinweise: Gebäude sind Block-out-Platzhalter (Decke innen dunkel). Der Wagen hält weiter vor der Garage, nicht an den Hallentoren.

---

## Phase 11 – Lauf vom 2026-10-16

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **88/88 bestanden, 0 mit Warnungen** (81 alte + 7 neue), Bericht `Saved/TestReports/Phase11_final2/index.json` |
| ↳ `Staff.HireFireAndPayroll` (3 Bewerber, jede Rolle dabei, Lohn nach Rolle/Erfahrung; Einstellen, gleiche Anfrage-ID → einmal; 1 Minute Arbeit = 1 Minutenlohn; Pause unbezahlt; ohne Geld: Lohnabrechnung scheitert, alle "Lohn ausstehend", Konto unberührt, Schuld bleibt, keine neue Lohnzeit; mit Geld: Schuld bezahlt, arbeitet wieder; höchstens 6; Entlassen) | ✅ |
| ↳ `Staff.WarehouseTransport` (Regal → Presse: trägt 25 kg, bringt alle 30 Stahl, lässt den Kunststoff liegen (Presse braucht ihn nicht), wartet dann; Presse macht einen Ballen; [T]-Filter Alles → Material → Alles; Maschinenausgabe → Kiste: sperriger Ballen passt nicht → wartet; kleine Produkte ins Regal) | ✅ (erst rot, siehe Behobene Fehler) |
| ↳ `Staff.MachineOperator` (schaltet die Sortiermaschine ein, holt bei 20 % Wartung ein Wartungsset aus dem Regal und wartet sie (100 %), Takt 5 s / 1,15; sortiert alles, bringt die Produkte ins Regal; Pause → Bonus weg) | ✅ (erst rot) |
| ↳ `Logistics.VanTours` (Wagen erst nach dem Upgrade; ohne Fahrer/leer keine Tour; Verkaufsfahrt: Sprit bezahlt, Wagen und Fahrer weg, nach 4 min zurück, Ladung zum besten Preis +15 %, Laderaum leer; Großeinkauf: 55 € + 8 € beim Losfahren, 4 PCs, 3 Monitore, 4 Kabelkartons im Laderaum, [R] lädt 11 Sperrige aus; Moderner Lieferwagen: 30 Plätze, Touren 60 % schneller) | ✅ (erst rot) |
| ↳ `Logistics.ForkliftCarriesHeavy` (erst nach dem Upgrade; 180-kg-Motor von Hand zu schwer; aufsteigen → 1.500 kg, Kollision aus, Motor auf der Gabel; Schredder nimmt ihn; absteigen → Last abgesetzt, Stapler bleibt stehen, wieder 30 kg; mit vollen Händen nicht aufsteigen) | ✅ |
| ↳ `Save.StaffRoundTrip` (Version 10: Lagerarbeiter mit Aufgabe, Filter und Ladung, Fahrer auf Tour mit Restzeit, Bewerber; Lagerarbeiter entlassen, Tour beendet, geladen → alles wie gespeichert, Fahrer wieder unterwegs) | ✅ (erst rot) |
| ↳ `Economy.BalanceStaffAndTours` (**echte Daten**) | ✅ Großeinkauf Altgeräte **63 €** statt **89,35 €** am Marktplatz, direkt weiterverkauft nur 43,78 € (mit Verkaufsfahrt 50,35 €) → keine Endlosschleife. Industriemotoren 58 € statt 63,18 €, weiterverkauft 23,32 € (Wert entsteht erst im Schredder). Lagerarbeiter füttert 10 min die Presse: 7 Ballen, Wertzuwachs 6,44 € bei 6,00 € Lohn |
| Angepasst: `Save.FlowRoundTrip`, `Save.MachineRoundTrip` (prüfen die jeweils aktuelle Version) | ✅ |
| Erzeugung von 45 Gegenständen (1 neuer), Personal- und Tourdaten, 10 Upgrades (3 neue), Maschinen, Möbel und Map (Wagen, Gabelstapler, Navigationsnetz, Personal-Startpunkt) per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P11`, 8 Läufe) | ✅ 9 Screenshots geprüft: Reiter "Personal & Logistik" mit Bewerbern, Mitarbeitern, Löhnen, Wagen und Stapler; Zuweisen-Hinweis "Aufgabe für …: Quelle wählen …/[E] Schwerlastregal wählen/[X] Abbrechen"; Lagerarbeiter und Maschinenbediener arbeiten (Presse läuft mit Bediener); Gabelstapler mit dem 180-kg-Motor auf der Gabel → in den Schredder; Kabelkarton in den Wagen, Fahrer am Wagen, Tour gestartet (Wagen weg); Mitarbeiter-Details; gespeichert, Bediener entlassen, geladen → wieder da. Mitarbeiter laufen über das Navigationsnetz, der Fahrer durchs offene Tor |
| **Netzwerktest Personal & Logistik** (`-WESmokeRole=U/V`, Dedicated Server + 2 Clients, neu, 4 Läufe) | ✅ U kauft Wagen und Stapler, stellt 3 Mitarbeiter ein, weist den Fahrer zu; V sieht alle drei mit Namen, Aufgabe und Zustand ("kein Weg frei (Tor zu?)", nach dem Öffnen "bereit für eine Tour"); U lädt und schickt den Wagen los, V sieht ihn unterwegs; V pausiert den Lagerarbeiter, U sieht "Pause"; V fährt Gabelstapler (hängt auch beim Client unter dem Fahrer), steigt ab, U sieht ihn am neuen Ort |
| **Regression** Netzwerktests A/B … S/T (Dedicated) und I/J (Listen-Server), Spielläufe P5–P10 | ✅ unverändert |

### Behobene Fehler in Phase 11

| Fehler | Ursache | Behebung |
|---|---|---|
| Fahrer galt am Wagen nie als "da", Touren starteten nicht (Tests rot) | Abstand über Bounding-Box, die bei Platzhaltern unzuverlässig ist | Abstand zur (gedrehten) Grundfläche |
| Maschinenbediener brachte die Produkte nicht ins Lager | Mengenprüfung fragte die Maschine statt das Lager | Ziel wird je Weg übergeben |
| Fahrer lief durch die geschlossene Garagenwand | Kein Navigationsnetz in der Map, Notlösung "direkt" | `RecastNavMesh` in der Map (zur Laufzeit aufgebaut); gibt es keinen Weg, wartet der Mitarbeiter |
| Gabelstapler: Motor ließ sich nicht in den Schredder setzen | Upgrade-Neuberechnung schaltete die Kollision des gefahrenen Staplers wieder ein, der Zielstrahl traf ihn | Stapler behält beim Fahren Kollision aus; Zielstrahl ignoriert ihn |
| Gabelstapler beim fahrenden Client nicht angehängt | Anhängen nur auf dem Server | Anhängen auf jedem Rechner aus dem replizierten Fahrer |
| Spalten im Reiter "Personal & Logistik" überlappten | Umbruchbreiten | Breiten angepasst |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Echte Lohnabrechnung nach 5 Minuten im Spiel | Im Unit-Test direkt ausgelöst (gleicher Timer-Mechanismus wie Strom/Autosave) |
| Tour bis zur Rückkehr im Spiel (4 min) | Im Unit-Test komplett; im Spiel nur Abfahrt und Speichern/Laden während der Fahrt |
| Gabelstapler mit echter Maus/Tastatur fahren | Mikes Praxistest; im Spiel per Teleport des Fahrers |
| Viele Mitarbeiter gleichzeitig / Performance | max. 6 Mitarbeiter; nicht gemessen |

### Offene Probleme

Keine bekannten Fehler. Hinweise: Der Wagen verschwindet beim Losfahren (noch keine Fahranimation). Wird ein Client-Spieler per Dev-Befehl teleportiert, sieht er sich erst nach der nächsten Bewegungskorrektur am neuen Ort (nur Testskripte).

---

## Phase 10 – Lauf vom 2026-10-16

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **81/81 bestanden, 0 mit Warnungen** (75 alte + 6 neue), Bericht `Saved/TestReports/Phase10_final2/index.json` |
| ↳ `Flow.AutomatedRecyclingLine` (echte Daten: Zuführband → Schredder → Band → Sortiermaschine → 2-m-Band → Materiallager; Ports richtig erkannt; PC aufs Band gelegt, danach kein Eingriff: Band bringt ihn in den Schredder, der startet selbst, Bänder leeren beide Ausgaben, im Lager kommen Elektronikschrott 3, Stahl 3, Kunststoff, Kupfer an; 3 Schreddergut warten in der Sortiermaschine auf die nächsten 5; nichts fällt auf den Boden; Bänder ziehen Strom) | ✅ |
| ↳ `Flow.ConveyorMovesBlocksAndJoins` (60 cm/s; kein zweiter Gegenstand auf derselben Stelle; Übergabe über Eck an den Anfang des nächsten Bands; offenes Ende: Band staut mit 35 cm Abstand; volles Band nicht verschiebbar; angehaltenes Band bewegt nichts und zieht keinen Strom; neu gestellte Kiste am Ende nimmt 3 × 5 Stahl; Kiste nimmt keinen sperrigen PC → wartet auf dem Band) | ✅ |
| ↳ `Flow.StoreFilterAndRecipes` ([T] am Fertigungsautomaten: Automatisch → 3 Rezepte → Automatisch; Rezept Kabel gewählt: aus Granulat + Kunststoff werden 2 Kabel, keine Drahtspule, Produkte "Neu"; Lager auf "Alles" schickt den Stahl nicht los, weil die Maschine ihn nicht nimmt – Band bleibt frei; Lager nur Stahl → 1 Stahlwinkel-Set, Granulat bleibt im Lager; [T] am Lager: Alles → Material → Aus (nichts geht raus) → Alles) | ✅ (erst rot, siehe Behobene Fehler) |
| ↳ `Flow.PlayerAndBelt` (getragener PC aufs Band an der Stelle, wo man steht (160 cm); Kundengerät abgelehnt; [E] nimmt den PC wieder in die Hände, Kleinteile ins Inventar; [Q] hält an; volles Band → "Band voll") | ✅ |
| ↳ `Save.FlowRoundTrip` (Version 9: angehaltenes Band mit Stahl bei 40 cm und PC, Lagerausgabe "nur Kupfergranulat", gewähltes Rezept; alles verändert, geladen → wie gespeichert) | ✅ |
| ↳ `Economy.BalanceProduction` (**echte Daten, bester Händler, Strom abgezogen**) | ✅ 4 Kupfergranulat 5,54 € → Kupferdrahtspule **9,90 €**; 2 Granulat + 3 Kunststoff 2,91 € → Verlängerungskabel **5,94 €**; 6 Stahlschrott 0,69 € → Stahlwinkel-Set **1,45 €**; Strom je Stück 3–5 ct; kein Produkt über dem 2,5-Fachen; versiegelte Palette weiterhin unverkäuflich |
| Angepasst: `Input.DefaultMapping` (21 Belegungen, [T]), `Save.MachineRoundTrip` (prüft die aktuelle Version) | ✅ |
| Erzeugung von 44 Gegenständen (3 neue), 4 Maschinen, Möbelkatalog (14), Händlerdaten mit 4 Produktionsaufträgen und Map per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P10`, 5 Läufe) | ✅ 9 Screenshots geprüft: Linie aus 4 Bändern, Schredder, Sortiermaschine, Materiallager und Fertigungsautomat aufgestellt; "[E] Aufs Band legen" mit dem PC in der Hand; zwei PCs laufen in den Schredder; Schreddergut fährt sichtbar auf dem Band ("[E] Vom Band nehmen (Schreddergut Elektronik)", "[Q] Band anhalten"); Lager füllt sich von allein; "[T] Rezept: Stahlwinkel-Set stanzen"; Untersuchen am Band zeigt "Nimmt von: Materiallager · Gibt ab an: Fertigungsautomat"; Stahlwinkel-Set fertig; gespeichert, Band angehalten, geladen → läuft wieder |
| **Netzwerktest Produktionslinie** (`-WESmokeRole=S/T`, Dedicated Server + 2 Clients, neu, 3 Läufe) | ✅ S baut die Linie und legt einen PC auf; T sieht das Schreddergut auf dem Band wandern (60 → 90 cm, dann in der Sortiermaschine); T stellt die Lagerausgabe um und hält das Band zum Lager an → S sieht beides; nach dem Starten läuft das Material weiter. Der Server lehnt Einschalten aus zu großer Entfernung ab ("out of reach") |
| **Regression** Netzwerktests A/B, C/D, E/F, G/H, K/L, M/N, O/P, Q/R (Dedicated) und I/J (Listen-Server), Spielläufe P5–P9 | ✅ unverändert |

### Behobene Fehler in Phase 10

| Fehler | Ursache | Behebung |
|---|---|---|
| Linie im Spiel verstopft: Materiallager schickte Elektronikschrott zum Fertigungsautomaten, der ihn nicht nimmt; das Band stand für immer | Lagerausgabe kannte das Ziel nicht | Lager gibt nur ab, was die Maschine am Ende der Linie gerade annimmt (`FindLineDestination`) |
| Untersuchen-Fenster: Text lief bei Maschinen und Bändern unten aus dem Hintergrund | feste Fensterhöhe | Höhe wächst mit dem Inhalt |
| Testskript: zwei PCs als ein Stapel "Alter Computer (2)" gespawnt | Dev-Befehl ignorierte die Stapelgröße | ein Actor je Stück |
| Linie ließ sich im Spiel nicht aufstellen | Spielerstart-Punkt an der geplanten Stelle | Linie verlegt (nur Testskript) |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Bänder mit echter Maus im Baumodus verlegen | Mikes Praxistest; im Spiel per Skript über denselben Baumodus |
| Lange Linien (> 20 Bänder) / Performance-Messung | Nachbarsuche läuft je Schritt über alle Möbel; bei wenigen Dutzend unkritisch, Cache vorgemerkt |

### Offene Probleme

Keine bekannten Fehler. Hinweise: Gibt eine Maschine etwas aus, das die nächste nicht nimmt (z. B. Kupfergranulat aus dem Schredder vor der Sortiermaschine), staut das Band – gewollt, die Linie muss dann passend gebaut oder über ein Materiallager geführt werden. Eine Weiche/ein Verteiler fehlt noch.

---

## Phase 9 – Lauf vom 2026-10-15

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **75/75 bestanden, 0 mit Warnungen** (67 alte + 8 neue), Bericht `Saved/TestReports/Phase9_final/index.json` |
| ↳ `Machines.ShredSortPressChain` (echte Daten: 3 Maschinen gekauft (1.150 €), Schredder 6 Eingangs-/10 Ausgabeplätze, nimmt sperrige Geräte, Sortiermaschine nicht; defekter PC hinein → läuft sofort, Material beim Start entnommen, Dauer 3 s + 1,2 s/kg × 9 kg = 13,8 s, Last 7,5 kW, Status nennt das Rezept, nach 7 s halb fertig, danach 8 × Schreddergut Elektronik, Verschleiß, Leerlauf 0,4 kW; 10 Schreddergut → 2 Sortiervorgänge mit Elektronikschrott/Stahl/Kunststoff in den erwarteten Spannen; 25 Stahlschrott → 1 Stahlballen vor der Presse, 5 bleiben liegen) | ✅ |
| ↳ `Machines.InsertRules` (Rohstoff in den Schredder → "verarbeitet das nicht", bleibt liegen; Sammlerstück und Kundengerät abgelehnt; Sortiermaschine nimmt keinen ganzen PC; getragener PC hinein → Hände frei; Schreddergut als Stapel in die Sortiermaschine; Werkzeug-Versand verkauft Wartungssets) | ✅ |
| ↳ `Machines.PowerLimitAndUpgrade` (11 kW: von Schredder + Sortiermaschine läuft nur eine, die andere "Stromanschluss überlastet"; mit Starkstromanschluss 22 kW → beide laufen, Last 11,5 kW) | ✅ |
| ↳ `Machines.PowerBill` (10 min Leerlauf = 0,0667 kWh; 6 s Schreddern = 7,5 kW × 6 s; 2 kWh → 5,00 € abgebucht, Zähler auf 0, letzte Rechnung sichtbar; Rechnung bei 1,20 € Kontostand → 0 €, nie negativ; aus = 0 kW) | ✅ |
| ↳ `Machines.WearAndMaintenance` (unter 30 % Wartung 70 % Tempo (8 s → 11,4 s), 3 % Verschleiß je Pressvorgang, bei 0 % "Wartung nötig" und kein Start, Material bleibt im Eingang; ohne Set kein [X], Grund nennt das Wartungsset; mit Set → 100 %, ein Set verbraucht, läuft wieder mit voller Geschwindigkeit) | ✅ |
| ↳ `Machines.OutputFullAndPause` (Ausgabe mit 3 freien Plätzen bei 4 möglichen Produkten → "Ausgabe voll", Eingang unberührt; [R] entnimmt 9 → startet; ausschalten mitten im Vorgang → Fortschritt bleibt 10 s lang gleich, Material bleibt drin, nicht verschiebbar; einschalten → läuft weiter und wird fertig) | ✅ |
| ↳ `Save.MachineRoundTrip` (Version 8: laufender Schredder mit Material, Fortschritt, zweitem Kabelkarton im Eingang, Schreddergut in der Ausgabe und Stromzähler gespeichert; alles verändert, geladen → an, gleicher Vorgang und Fortschritt, Eingang und Ausgabe getrennt richtig, Zähler stimmt, läuft weiter) | ✅ |
| ↳ `Economy.BalanceRecyclingChain` (**echte Daten, je 8 Stück durch Schredder → Sortiermaschine → Presse, verkauft beim besten Händler, Strom und Wartungssets abgezogen**) | ✅ defekter Elektromotor direkt 8,64 € → recycelt **11,63 €**; defekter Röhrenmonitor 2,70 € → **6,76 €**; Kabelkarton 4,03 € → **4,77 €**; defekter PC 5,67 € → nur 1,25 € (Handarbeit lohnt sich mehr, gewollt). Kein Ausreißer über das Dreifache |
| Erzeugung von 41 Gegenständen (8 neue), 3 Maschinen, Möbelkatalog (10), 7 Upgrades, Händlerdaten und Map per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P9`, 4 Läufe) | ✅ 8 Screenshots geprüft: drei Maschinen im Baumodus aufgestellt (Konto 2.250 → 1.100 €); "[E] In die Maschine geben (Schredder)" mit dem PC in der Hand; Schredder läuft (Leiste, "Strom 7.5 / 11 kW"); Sortiermaschine "Stromanschluss überlastet – wartet"; nach dem Schreddern startet sie von selbst; Produkte mit [R] entnommen (8 Schreddergut), [X] gewartet (98 → 100 %); 20 Stahlschrott in die Presse → Stahlballen liegt davor; gespeichert, Sortiermaschine umgeschaltet, geladen → Zustand wie gespeichert |
| **Netzwerktest Maschinen** (`-WESmokeRole=Q/R`, Dedicated Server + 2 Clients, neu, 2 Läufe) | ✅ Q stellt die Maschinen auf, wirft einen PC in den Schredder und schaltet ihn ein; R sieht ihn laufen (Fortschritt 17 % → 39 %, flüssig lokal hochgerechnet); R füllt die Sortiermaschine und schaltet sie ein → "Stromanschluss überlastet" (7,8 / 11 kW); nach dem Schreddern läuft sie an; R entnimmt die 8 Schreddergut, Q sieht die leere Ausgabe und die laufende Sortiermaschine |
| **Regression** Netzwerktests A/B, C/D, E/F, G/H, K/L, M/N, O/P (Dedicated) und I/J (Listen-Server), Spielläufe P5, P6, P7, P8 | ✅ unverändert |

### Behobene Fehler in Phase 9

| Fehler | Ursache | Behebung |
|---|---|---|
| Balance-Test: Kabelkarton und Röhrenmonitor brachten recycelt weniger als direkt verkauft | Die Sortiermaschine fiel nach 50 Vorgängen ohne Wartung aus; Kabel lieferten nur Stahl-Schreddergut | Test kauft Wartungssets (Kosten werden abgezogen); eigenes Schredder-Rezept "Kabel schreddern" → Kupfergranulat |
| Maschinenprodukte stapelten nicht mit Material von der Werkbank | anderer Qualitätswert (60 statt 50) | gleiche Werte wie an der Werkbank |
| Schredder-Verschleiß 2,5 % wirkte als 2 % | Wartung wird ganzzahlig gespeichert | Wert auf 2 % gesetzt |
| UHT-Fehler beim Bauen | Einheit "kW" kennt Unreal nicht | Einheit entfernt |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Stromrechnung nach echten 5 Minuten im Spiel | Im Unit-Test direkt ausgelöst; der Timer selbst ist derselbe Mechanismus wie Autosave |
| Maschinen mit echter Maus/Tastatur bedienen | Mikes Praxistest; im Spiel über dieselben Anfragewege gesteuert |
| Mehrere Stunden Dauerbetrieb / Performance mit vielen Maschinen | erst sinnvoll mit Verbindungen zwischen Maschinen (Phase 10) |

### Offene Probleme

Keine bekannten Fehler. Hinweis: Die Rezeptauswahl ist automatisch (erstes passendes Rezept); eine Auswahl von Hand ist Teil von Phase 10.

---

## Phase 8 – Lauf vom 2026-10-14

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **67/67 bestanden, 0 mit Warnungen** (61 alte + 6 neue), Bericht `Saved/TestReports/Phase8/index.json` |
| ↳ `Build.PlaceRulesAndCosts` (echter Möbelkatalog: freier Platz ok; außerhalb der Garage / halb über der Wand abgelehnt; Aufstellen kostet 60 €, rastet auf 10 cm und 15° ein (203,7/148,2/97° → 200/150/90°); Regal hat 15 Plätze/150 kg; gleicher Platz, Gegenstand oder Spieler im Weg → abgelehnt; zu teuer → nichts gebucht; gleiche Anfrage-ID zweimal → einmal; unbekanntes Möbel) | ✅ |
| ↳ `Build.MoveAndRemove` (große Werkbank verschieben und drehen; auf ein Regal schieben → abgelehnt, bleibt stehen; mit Gerät darauf nicht verschiebbar; große Werkbank arbeitet 2,4 s → 2,0 s und hat 20 Plätze; volles Regal nicht abbaubar, leer → 30 € zurück; Grundausstattung verschiebbar, nicht verkaufbar) | ✅ |
| ↳ `Build.PreviewAndSelection` (Baumodus, Mausrad-Auswahl inkl. Umlauf, Vorschau rastet ein, grün auf freiem Platz, rot auf belegtem/außerhalb mit Grund, Drehen 90°+15°, mit Gegenstand in der Hand kein Baumodus) | ✅ |
| ↳ `Build.NewBenchGetsUpgrades` (neue Werkbank bekommt den Werkbank-Unterschrank: 12 + 6 Plätze) | ✅ |
| ↳ `Build.NewToolsAndHandTruck` (45-kg-Motor ohne Sackkarre zu schwer, mit Sackkarre 60 kg Tragkraft → tragbar; bestes Werkzeug zählt: Bit-Ratsche ×1,6; Digitalmultimeter und Lötstation ×2) | ✅ |
| ↳ `Save.FurnitureRoundTrip` (Version 7: gekauftes Regal mit Inhalt, abgebaute Werkbank kommt mit Namen, Art und Position zurück, verschobene Grundausstattung wieder an ihrer Stelle) | ✅ (erst rot, siehe Behobene Fehler) |
| Erzeugung von 33 Gegenständen (5 neue), Möbelkatalog (7), Händlerdaten und Map (Baubereich) per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P8`, 2 Läufe) | ✅ 7 Screenshots geprüft: Baumodus-Panel mit Katalog und Preisen, grüne Vorschau, rote Vorschau am Marktplatz-Tisch ("Kein Platz – hier steht schon etwas"), Werkbank aufgestellt (−90 €), Lagerkiste aufgestellt (−15 €), "Bearbeiten" → Kiste angefasst und verschoben, abgebaut (+7,50 €), gespeichert, Werkbank verschoben, geladen → Werkbank wieder an ihrem Platz |
| **Netzwerktest Einrichtung** (`-WESmokeRole=O/P`, Dedicated Server + 2 Clients, neu) | ✅ O stellt Werkbank und Kiste auf; P sieht beide mit richtiger Größe und Lagerplätzen (12 / 8); P's Vorschau auf O's Werkbank ist rot; P schickt die Platzierung trotzdem → Server lehnt ab ("Kein Platz"), kein Geld weg; P verschiebt O's Kiste, O sieht die neue Position; P baut sie ab (+7,50 €), beide sehen 152,50 € |
| **Regression** Netzwerktests A/B, C/D, E/F, G/H, K/L, M/N (Dedicated) und I/J (Listen-Server), Spielläufe P5, P6, P7 | ✅ unverändert |

### Behobene Fehler in Phase 8

| Fehler | Ursache | Behebung |
|---|---|---|
| Spielstand laden direkt nach dem Abbauen eines Möbels: das Möbel kam nicht unter seinem Namen zurück, sein Lagerinhalt wäre verloren gegangen (Test rot) | Ein zerstörter Actor belegt seinen Namen bis zur nächsten Speicherbereinigung | Name eines gerade abgebauten Möbels wird vor dem Wiederherstellen freigegeben |
| Statuszeile im Baumodus-Panel lag für ein Bild über dem Hilfetext (Screenshot) | Panelbreite hing von wechselnden Texten ab | feste Panelbreite |
| Test-Platzierung des großen Regals ragte aus der Garage | Testdaten | Test korrigiert (gedreht) |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Baumodus mit echter Maus (Mausrad, Zielen) | Mikes Praxistest; im Spiel per Skript gesteuert, Zielen über echten Blickstrahl |
| Elektromotor mit Sackkarre im Spiel tragen | Nur im Unit-Test |
| Mehrere Werkbänke gleichzeitig von zwei Spielern benutzt | Werkbänke sind unabhängige Actors (Netzwerktest E/F gilt je Bank); nicht gezielt mit zwei Bänken getestet |

### Offene Probleme

Keine bekannten Fehler. Hinweis: Beim Verschieben im Spiel entsteht für einen Moment Bewegungsunschärfe am Möbel (Kamera-Effekt der Engine).

---

## Phase 7 – Lauf vom 2026-10-14

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **61/61 bestanden, 0 mit Warnungen** (53 alte + 8 neue), Bericht `Saved/TestReports/Phase7/index.json` |
| ↳ `Procurement.ReturnPalletBuyAndOpen` (echte Daten: Palette kaufen → steht an der Lieferpalette → "[Q] Öffnen" → 3–5 Funde liegen in der Welt, Palette weg; gleiche Palette = gleicher Inhalt, also kein Neuwürfeln durch Laden; getragen nicht zu öffnen; versiegelte Palette kauft niemand) | ✅ |
| ↳ `Procurement.ScrapPileRefills` (füllt bis 4 auf, voll = nichts Neues, weggetragene/zerstörte Teile werden ersetzt, Hinweis "gratis") | ✅ |
| ↳ `Procurement.RotatingTraderStock` (60 Nachschübe: feste Angebote immer gleich; Angebot mit 50 % Chance 15–45-mal da, Zustand und Qualität schwanken im Rahmen; Kauf nimmt den gewürfelten Zustand) | ✅ |
| ↳ `Procurement.AuctionBiddingAndDelivery` (Startpreis 30 % des Schätzwerts; Mitbieter kontert bis zu seinem Limit; darüber führt die Firma; nicht gegen sich selbst bieten; Verlängerung bei späten Geboten; Geld erst beim Zuschlag; Ware auf die Palette; Zuschlag verfällt, wenn das Geld am Ende fehlt; zu wenig Geld für ein Gebot) | ✅ |
| ↳ `Procurement.AuctionThroughTerminal` (Reichweite, gleiche Anfrage-ID zweimal = ein Gebot) | ✅ |
| ↳ `Procurement.ExtendedOrders` (Mindestqualität, kaputtes Gerät zählt nicht, funktionierendes Gerät wird aus den Händen abgeliefert) | ✅ |
| ↳ `Save.ProcurementRoundTrip` (Version 6: gewürfelter Händlervorrat inkl. Zustand, laufende Auktion inkl. Gebot, Höchstbietendem und geheimem Limit) | ✅ |
| ↳ `Economy.BalanceReturnPalletsAndScrap` (**echte Daten, je 400 Paletten**) | ✅ Elektronik-Palette **35,10 €**: Inhalt direkt verkauft 27,90 € (ohne seltene Funde 24,10 €), Marktwert 45,26 €, seltener Fund in 6,0 % der Paletten. Werkzeug-Palette **29,90 €**: direkt 19,62 € (16,48 €), Marktwert 40,34 €, selten 4,5 %. Schrotthaufen: ein Gratis-Teil bringt im Schnitt 0,96 €, selten in 1,0 % |
| Erzeugung von 28 Gegenständen (6 neue), 3 Beutetabellen, 5 Händlern, 14 Auftragsvorlagen, 7 Auktionsvorlagen und der Map (Schrotthaufen) per Skript | ✅ |
| **Spiel mit Grafik** (`-WEShots=P7`, 3 Läufe) | ✅ 8 Screenshots geprüft: Retourenpalette kaufen → Reiter "Auktionen & Beschaffung" → Bietgefecht (z. B. 5,50 → 13,50 € gegen "Elektro Kaya") → Zuschlag, 3 PCs bzw. Monitore stehen an der Palette, Buchung "Auktion gewonnen" → Palette mit [Q] öffnen ("Geöffnet: PC-Netzteil, Arbeitsspeicher, PC-Netzteil") → Schrotthaufen draußen mit Hinweis "Was herumliegt, ist gratis · neuer Schrott in ca. 8 min" |
| **Netzwerktest Auktion + Palette** (`-WESmokeRole=M/N`, Dedicated Server + 2 Clients, neu, 2 Läufe) | ✅ M bietet, bis die Firma führt; N sieht "WE <M>" als Höchstbietenden, sein Gebot auf dasselbe Los lehnt der Server ab ("Eure Firma bietet hier schon am höchsten"); N kauft eine Werkzeug-Palette und öffnet sie an der Lieferpalette; nach dem Zuschlag sehen beide denselben Kontostand (250 − 25,50 − 29,90 = 194,60 €) und dieselben Teile |
| **Regression** Netzwerktests A/B, C/D, E/F, G/H, K/L (Dedicated) und I/J (Listen-Server), Spielläufe P5 und P6 | ✅ unverändert |

### Behobene Fehler in Phase 7

| Fehler | Ursache | Behebung |
|---|---|---|
| Retourenpaletten lohnten sich nicht (Balance-Test rot: Inhalt 45 € Marktwert für 58,50 €) | Preis zu hoch angesetzt | Elektronik 35,10 €, Werkzeug 29,90 € |
| Mitbieter hieß bei jedem Gegengebot anders (Spiellauf) | Name wurde bei jedem Gebot neu gewählt | ein fester Mitbieter je Los |
| Zwei gleiche Lose liefen gleichzeitig (Netzwerktest) | Zufallsauswahl ohne Ausschluss | dieselbe Art Los nie doppelt gleichzeitig |
| Bestehende Tests fanden "das" PC-Angebot nicht mehr eindeutig | Händler haben jetzt wechselnde Zusatzangebote (z. B. PC in anderem Zustand) | Tests suchen nach Gegenstand **und** Zustand |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Auktion bis zum echten Ende (5–6 Minuten) im Spiel | Im Spiel per Entwickler-Befehl sofort beendet; Fristablauf und Verlängerung im Unit-Test |
| Schrotthaufen-Nachschub nach 8 Minuten im Spiel | Nicht abgewartet; dieselbe Funktion ist im Unit-Test direkt aufgerufen |
| Seltener Fund im Spiel | Zufall (ca. 1–6 %); Häufigkeit über 400 Würfe im Balance-Test gemessen |
| Bedienung mit echter Maus (Reiter, Bieten) | Mikes Praxistest |

### Offene Probleme

Keine bekannten Fehler. Hinweis: Der Elektromotor-Posten wurde bewusst nicht in die Auktionen aufgenommen, weil 45-kg-Motoren noch nicht getragen werden können (Hilfsmittel folgen später).

---

## Phase 6 – Lauf vom 2026-10-13

### Ausgeführt

| Test | Ergebnis |
|---|---|
| Build Editor + Game (Win64 Development) | ✅ 0 Warnungen/Fehler im Projektcode |
| Automation-Suite `WorkshopEmpire` (headless) | ✅ **53/53 bestanden, 0 mit Warnungen** (46 alte + 7 neue), Bericht `Saved/TestReports/Phase6/index.json` |
| ↳ `Repair.FullFlowExchangeTestRefurbish`: ohne Multimeter abgelehnt → Fehler suchen (2 Fehler) → Teil A ausbauen (defektes Teil landet im Werkbanklager) → defektes Teil zählt nicht als Ersatzteil → Ersatzteil aus dem Werkbanklager einbauen → Teil B (aus dem Inventar) → Funktionstest → "Gebraucht", Qualität +15, Wert mehr als doppelt so hoch → ohne Reinigungsset abgelehnt → aufbereiten: Qualität 85 % → "Sehr gut", nur einmal möglich | ✅ |
| ↳ `Repair.DamagedSparePartFailsTest`: beschädigtes Ersatzteil eingebaut → sieht ersetzt aus → Funktionstest fällt durch → wieder ausbauen → gutes Teil → bestanden | ✅ |
| ↳ `Repair.BlockedStatesAndCustomerDevices` (Schrott und teilzerlegt nicht reparierbar, funktionierendes Gerät nur aufbereiten, zweiter Spieler wird abgelehnt solange gearbeitet wird, Kundengerät: reparieren ja, zerlegen nein, verkaufen nein) | ✅ |
| ↳ `Repair.DisassemblyRespectsKnownFaults` (echte PC-Daten: bekannt defektes Netzteil kommt beim Zerlegen als "Defekt" heraus; schon für die Reparatur ausgebautes Netzteil kommt nicht doppelt heraus) | ✅ |
| ↳ `Repair.CustomerRepairOrder` (Annehmen → Kundengerät liegt an der Lieferpalette → kaputt abliefern abgelehnt → reparieren → in der Hand abliefern → Belohnung, Gerät weg; abgelaufener Auftrag → Kunde holt Gerät ab) | ✅ |
| ↳ `Save.RepairStateRoundTrip` (Version 5: Reparaturstand auf der Werkbank und Reparaturauftrag mit Kundengerät-Id) | ✅ |
| ↳ `Economy.BalanceRepairBeatsPartingOut` (**echte Daten, 30 Durchläufe**: defekten PC kaufen, Fehler suchen, Ersatzteile gebraucht kaufen, reparieren, verkaufen) | ✅ PC 11,36 € + Teile 14,23 € → Verkauf 31,50 € (+ 2,29 € für die defekten Teile) = **+8,20 €**; aufbereitet Verkauf 43,99 € (Set 2,99 €) = **+17,70 €**. Zerlegen desselben PCs: **+5,81 €** (`Economy.BalanceDefectivePcIsProfitable`) |
| Erzeugung von 22 Gegenständen (4 neue), 2 Reparaturabläufen, 4 Händlern, 10 Auftragsvorlagen und der Map per Skript | ✅ |
| **Spiel mit Grafik, Reparatur komplett** (`-WEShots=P6`, 4 Läufe) | ✅ 11 Screenshots geprüft: Multimeter und Reinigungsset kaufen → PC auf die Werkbank → "[Q] Reparieren: Fehler suchen (Multimeter)" → "Diagnose: Grafikkarte defekt" (in anderen Läufen: Arbeitsspeicher; Arbeitsspeicher + Mainboard) → [F] zeigt "Reparatur: Grafikkarte defekt" → passendes Ersatzteil am Marktplatz kaufen → ausbauen, einbauen, Funktionstest bestanden → aufbereitet ("Sehr gut, Qualität 80 %") → für 43,99 € verkauft |
| **Netzwerktest Reparaturauftrag** (`-WESmokeRole=K/L`, Dedicated Server + 2 Clients, neu) | ✅ K nimmt "Büro-PC zur Reparatur" an → Gerät erscheint an der Palette → K trägt es zur Werkbank → Diagnose "Arbeitsspeicher defekt" → kauft RAM → repariert → liefert in der Hand ab: **+23,63 €**. L sieht Auftrag, Gerät, Reparaturstand und Kontostand; L's Versuche, während der Arbeit zu reparieren oder das Gerät herunterzunehmen, lehnt der Server ab |
| **Regression Netzwerktests A/B, C/D, E/F, G/H** (Dedicated Server) **und I/J** (Listen-Server) | ✅ unverändert (E/F nach Skriptkorrektur, siehe unten) |
| **Regression Spiel P5** (kompletter Ablauf Phase 5) | ✅ unverändert |

### Behobene Fehler in Phase 6

| Fehler | Ursache | Behebung |
|---|---|---|
| Reparatur blieb im Netzwerktest bei "Ersatzteil fehlt: Mainboard" hängen | Kein Händler verkaufte Mainboards | Ersatzteil-Express führt ein gebrauchtes Mainboard |
| Fortschrittsbalken zeigte während der Reparatur "Gehäuseschrauben lösen" (Screenshot) | Balken las immer den Zerlegeschritt | zeigt den laufenden Reparaturschritt |
| Zweimal "Arbeite …" im Hinweis (Screenshot) | [E]- und [Q]-Zeile meldeten beide "beschäftigt" | nur noch eine Zeile |
| Marktplatz zeigte direkt nach einem Kauf für ein Bild übereinanderliegende Zeilen (Screenshot) | Automatischer Zeilenumbruch kennt die Breite neuer Zeilen erst im nächsten Bild | feste Umbruchbreiten in den Zeilen |
| Reparatur lohnte sich in der ersten Messung kaum mehr als Zerlegen, Aufbereiten brachte nur +0,16 € | Ganzer PC war weniger wert als seine Teile; +20 % Qualität reichten nie für "Sehr gut" | PC-Wert 75 € (defekt ×0,42), Reparatur +15 %, Aufbereiten +30 % Qualität |
| Netzwerktest E/F: Computer landete neben der Werkbank | Zufälliger Startpunkt teils außer Reichweite (trat schon in Phase 5 als Zufall auf) | Testskript geht vorher zur Werkbank |

### Nicht ausgeführt

| Test | Grund |
|---|---|
| Röhrenmonitor reparieren im Spiel (Lötkolben, Kondensatoren, Netzkabel) | Nur Daten; derselbe Code wie beim PC, der im Spiel, in Tests und im Netzwerk geprüft ist |
| Reparaturauftrag ablaufen lassen im Spiel (30 min) | Nur im Unit-Test (Frist künstlich abgelaufen) |
| Beschädigtes Ersatzteil, das im Spiel den Test nicht besteht | Nur im Unit-Test (Zufall); im Spiel wurden immer gebrauchte Teile gekauft |
| Bedienung mit echter Maus/Tastatur | Mikes Praxistest |

### Offene Probleme

Keine bekannten Fehler. Hinweise: Durch die Reparatur kommen beim Zerlegen eines defekten Geräts die tatsächlich kaputten Teile jetzt als "Defekt" heraus. Dadurch sank der Gewinn beim Zerlegen von 6,14 € auf 5,81 € (mit dem neuen PC-Preis). Lange Wörter wie "Werkstattbeleuchtung" ragen im Marktplatz teils unter den Preis; das gab es schon in Phase 5.

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
