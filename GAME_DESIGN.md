# WORKSHOP EMPIRE – Game Design

Stand: Phase 4 (2026-10-12). Vollständige Spezifikation: `Docs/MASTER_PROMPT_V1.0.md`. Dieses Dokument fasst die Designentscheidungen zusammen und wird mit jeder Phase präzisiert.

## 1. Spielkonzept

First-Person-Simulation aus **Handwerk + Business + Industrie** für Windows/Steam, Singleplayer und Koop für 2–4 Spieler mit gemeinsamem Unternehmen.

Start: heruntergekommene Garage, **250 €**, alter Transporter, einfache Werkbank, Grundwerkzeug, wenig Lagerfläche. Ziel: von der Garage zum automatisierten Industrieunternehmen.

## 2. Kern-Loop

**Beschaffen → Prüfen → Bearbeiten → Verkaufen → Investieren → Expandieren**

- Beschaffen: Gebrauchtwaren, Schrottplatz, Auktionen, Retourenpaletten, Kundenaufträge, Online-Markt im Spiel.
- Prüfen: Zustand, Qualität, Gewicht, Material, Funktion, Marktwert, Seltenheit, verwertbare Komponenten.
- Bearbeiten: Reinigen, Reparieren, Zerlegen, Ausschlachten, Aufbereiten, Einschmelzen, Weiterverarbeiten.
- Verkaufen: ganze Gegenstände, reparierte Produkte, Ersatzteile, Rohstoffe, Eigenprodukte.
- Investieren/Expandieren: Werkzeuge, Maschinen, Gebäude, Fahrzeuge, Mitarbeiter, Lager, Technologien.

Erster Kontakt (Spec §4.1): defekter Computer → Werkbank → Gehäuse öffnen → Komponenten ausbauen → verkaufen → erstes Geld.

## 3. Gameplay-Systeme (Überblick)

| System | Kernidee | Phase |
|---|---|---|
| First Person | WASD, Sprint, Ducken, Interaktion mit Prompt ("[E] Aufheben"), physische Gegenstände via Physics Handle + Snapping | 1 |
| Items | Datengetriebene Definitionen + individuelle Instanzen; Zustand Neu → Sehr gut → Gebraucht → Beschädigt → Defekt → Schrott | 2 |
| Inventar | Spieler, Werkzeugtasche, Werkbank, Regal, Fahrzeug, Maschine, Firmenlager; Kapazität/Gewicht, Stapeln, Drag & Drop | 2 |
| Werkbank/Zerlegen | Wiederverwendbare Zerlegeabläufe als Schrittfolgen mit Werkzeuganforderung | 3 |
| Wirtschaft | Cent-genaues Firmenkonto, Händler, Preisfaktoren | 4 |
| Reparatur | Defekt finden → Ersatzteil → Tausch → Funktionstest (kein reiner Fortschrittsbalken) | 6 |
| Beschaffung | Schrottplatz, Auktionen, Paletten, seltene Funde (balanciert, nicht zufallsabhängig) | 7 |
| Werkstatt-Ausbau | Platzierung mit Vorschau, Kollision, Rotation, Snapping | 8 |
| Maschinen/Produktion | Schredder, Sortierer, Presse, Ofen, Förderband; datengetriebene Rezepte | 9–10 |
| Mitarbeiter/Logistik | Einfache, nachvollziehbare Aufgaben-KI; Fahrzeuge, Gabelstapler | 11 |
| Expansion | Garage → Werkstatt → Gewerbehalle → Fabrik → Industrieunternehmen | 12 |
| Koop | Gemeinsames Konto, Lager, Maschinen; Host besitzt den Spielstand | durchgehend, Härtung in 13 |

### 3.1 Steuerung (Stand Phase 11, später frei belegbar)

| Taste | Aktion |
|---|---|
| W A S D / Maus | Bewegen / Umsehen |
| Shift (halten) | Sprinten |
| Strg oder C (halten) | Ducken |
| Leertaste | Springen |
| E | Kleinteile aufheben (ins Inventar), sperrige Gegenstände tragen, Tor/Werkbank/Regal öffnen; mit Gegenstand in der Hand: Ablegen (dort, wo man hinschaut) |
| F | Untersuchen (Kategorie, Zustand, Qualität, Gewicht, Schätzwert, Zerlegefortschritt) / Schließen |
| R | Zweite Aktion, z. B. Gegenstand von der Werkbank nehmen |
| Q | Reparieren: nächster Reparaturschritt am Gerät auf der Werkbank; Retourenpalette öffnen |
| B | Baumodus an/aus |
| X | An einer Maschine: Warten (Phase 9) |
| T | Einstellung: Rezept einer Maschine, Ausgabe eines Materiallagers (Phase 10), Material eines Lagerarbeiters, Tour des Wagens (Phase 11) |
| X | Vom Gabelstapler absteigen; Zuweisen abbrechen; Mitarbeiter entlassen (2×) (Phase 11) |
| Im Baumodus | Mausrad: Möbel wählen oder "Bearbeiten" · [E] aufstellen bzw. anfassen und neu abstellen · [R] 90° · [Q] 15° drehen · [X] abbauen bzw. Verschieben abbrechen |
| Tab oder I | Inventar öffnen/schließen |
| F5 / F9 | Schnellspeichern / Schnellladen (nur Host) |

Im Inventar: Ziehen = verschieben, Shift+Ziehen = halber Stapel, Doppelklick = zwischen Inventar und Lager umlagern, Rechtsklick = in die Welt ablegen (Shift: 1 Stück), aus dem Fenster ziehen = ablegen.

### 3.2 Gegenstände und Inventar (Stand Phase 2)

- **Zwei Arten:** Kleinteile (Schrauben, Kabel, Speicherriegel, Werkzeug) wandern ins Inventar. Sperrige Dinge (Computer, Monitor, Motor, Werkzeugkiste, Kartons) werden in den Händen getragen und physisch abgestellt, z. B. auf Werkbank oder Regal.
- **Spielerinventar:** 20 Plätze, 25 kg. **Werkbank:** 8 Plätze, 60 kg. **Regal:** 15 Plätze, 150 kg.
- **Stapeln:** nur gleiche Gegenstände im gleichen Zustand (Schrauben bis 200, Kupferkabel bis 20).
- **Zustand** Neu → Sehr gut → Gebraucht → Beschädigt → Defekt → Schrott plus Qualität 0–100 bestimmen den Schätzwert (Platzhalter-Formel, echte Preise in Phase 4).
- Im Koop sieht jeder Spieler sein eigenes Inventar; Lager sind gemeinsam.

### 3.3 Werkbank und Zerlegen (Stand Phase 3)

1. Sperrigen Gegenstand (z. B. Alter Computer) tragen, auf die Werkbank schauen, [E] = auf den Arbeitsplatz legen.
2. Auf den Gegenstand schauen: [E] startet den nächsten Schritt, z. B. "Gehäuseschrauben lösen (Schraubendreher)". Das Werkzeug muss im Inventar sein; das beste passende Werkzeug bestimmt das Tempo (Akkuschrauber ×2,5).
3. Jeder Schritt dauert wenige Sekunden (Fortschrittsbalken) und liefert Teile ins Werkbanklager; passt etwas nicht hinein, landet es vor der Werkbank.
4. [R] nimmt den Gegenstand wieder herunter; der Fortschritt bleibt am Gegenstand ("teilweise zerlegt").
5. Nach dem letzten Schritt ist der Gegenstand verbraucht.

| Gegenstand | Schritte | Ergebnis (Beispiel) |
|---|---|---|
| Alter Computer | 7 (Schraubendreher, Hände, Seitenschneider) | Schrauben, Stahlschrott, Grafikkarte (85 %), 1–2 RAM, Netzteil, Mainboard, Kunststoff, Kupferkabel |
| Röhrenmonitor | 4 | Schrauben, Kunststoff, Elektronikschrott, Kupferkabel, Glasbruch, Stahlschrott |
| Karton mit Kabeln | 1 | Kupferkabel, Kunststoffteile |

Komponenten kommen im Zustand der Quelle oder bis zu zwei Stufen besser heraus (ein defektes Gerät hat meist nur ein kaputtes Teil) und mit leicht schwankender Qualität; Rohstoffe stapeln immer. Im Koop arbeitet immer nur ein Spieler gleichzeitig an einer Werkbank, die Spieler können sich aber abwechseln.

### 3.4 Reparieren (Stand Phase 6)

Ein Gerät auf der Werkbank kann man entweder zerlegen ([E]) oder reparieren ([Q]). Beides zusammen geht nicht: Ein teilweise zerlegtes Gerät ist nicht mehr reparierbar.

1. **Fehler suchen** (Multimeter): Ein beschädigtes Gerät hat genau einen kaputten Teil, ein defektes Gerät einen oder (30 %) zwei. Erst die Diagnose zeigt welche, z. B. "Diagnose: Netzteil defekt". [F] Untersuchen zeigt den Stand.
2. **Defektes Teil ausbauen**: Das kaputte Teil landet im Werkbanklager (Zustand "Defekt", noch ein paar Cent wert).
3. **Ersatzteil einbauen**: nimmt das beste passende Teil aus dem Inventar oder dem Werkbanklager ("Beschädigt" oder besser). Woher es kommt, entscheidet der Spieler: gekauft (bequem, teurer) oder aus einem anderen zerlegten Gerät (billiger).
4. **Funktionstest**: Sind alle Fehler behoben, läuft das Gerät wieder: Zustand "Gebraucht", Qualität +15. Ein beschädigtes Ersatzteil versagt mit 50 %, dann muss es wieder raus.
5. **Reinigen und aufbereiten** (optional, Reinigungsset): Qualität +30; ab 80 % wird das Gerät "Sehr gut". Einmal pro Gerät.

| Gerät | mögliche Fehler (Ersatzteil, Werkzeug) |
|---|---|
| Alter Computer | Netzteil (Schraubendreher), Arbeitsspeicher (Hände), Grafikkarte (Schraubendreher), Mainboard (Schraubendreher) |
| Röhrenmonitor | aufgeblähte Kondensatoren (Kondensatoren-Set, Lötkolben), gebrochenes Netzkabel (Kupferkabel, Seitenschneider) |

Wer ein defektes Gerät zerlegt, nachdem die Fehler feststehen, bekommt die kaputten Teile als "Defekt" heraus; ein schon für die Reparatur ausgebautes Teil fehlt beim Zerlegen.

**Reparaturaufträge:** Ein Kunde bringt sein Gerät (es erscheint beim Annehmen an der Lieferpalette). Man repariert es und liefert es in der Hand am Marktplatz ab. Kundengeräte kann man nicht verkaufen oder zerlegen. Läuft die Frist ab, holt der Kunde sein Gerät wieder ab.

### 3.5 Werkstatt einrichten (Stand Phase 8)

**Baumodus [B]:** Eine Vorschau des gewählten Möbels folgt dem Fadenkreuz auf dem Boden, rastet im 10-cm-Raster und in 15°-Schritten ein und ist grün, wenn es passt, sonst rot mit Grund ("Kein Platz", "Nur innerhalb der Werkstatt", "Ein Gegenstand liegt im Weg", "Nicht genug Geld"). Der Server prüft beim Bestätigen alles noch einmal. Im Modus "Bearbeiten" fasst [E] ein Möbelstück an (auch die Grundausstattung) und stellt es woanders ab; [X] baut gekaufte Möbel ab (halber Preis zurück). Verschieben und Abbauen gehen nicht, solange auf der Werkbank gearbeitet wird oder Gegenstände darauf liegen; Abbauen nur leer. Der Lagerinhalt zieht beim Verschieben mit um.

| Möbel | Preis | Lager | Besonderheit |
|---|---|---|---|
| Werkbank | 90 € | 12 Plätze, 60 kg | zweiter Arbeitsplatz |
| Große Werkbank | 240 € | 20 Plätze, 100 kg | Arbeiten 20 % schneller |
| Schwerlastregal | 60 € | 15 Plätze, 150 kg | |
| Großes Lagerregal | 140 € | 25 Plätze, 300 kg | |
| Werkzeugwand | 40 € | 10 Plätze, 30 kg | flach, an die Wand |
| Lagerkiste | 15 € | 8 Plätze, 60 kg | |
| Ablagetisch | 30 € | – | Fläche für große Geräte |

**Neue Werkzeuge** (Werkzeug-Versand): Bit-Ratsche (Schraubendreher ×1,6), Profi-Seitenschneider (×1,8), Digitalmultimeter (Fehlersuche ×2), Lötstation (×2). **Sackkarre** (klappbar, 4 kg im Inventar, ca. 82 €): +30 kg Tragkraft.

Tragen: ein Gegenstand gleichzeitig, bis 30 kg; mit Sackkarre im Inventar bis 60 kg (z. B. der 45-kg-Elektromotor). Noch schwerere Dinge brauchen später Gabelstapler oder einen zweiten Spieler. Ob Koop-Spieler schwere Dinge gemeinsam tragen können, ist eine offene Designfrage (§7).

### 3.6 Industriemaschinen (Stand Phase 9)

Maschinen werden wie Möbel im Baumodus gekauft, aufgestellt, verschoben und abgebaut (nur ausgeschaltet, leer und ohne laufenden Vorgang). Jede Maschine hat einen **Eingang** und eine **Ausgabe**.

| Taste an der Maschine | Aktion |
|---|---|
| E | Eingang öffnen (Kleinteile per Inventar hineinlegen); mit einem getragenen Gerät in der Hand: "In die Maschine geben" |
| Q | Einschalten / Ausschalten |
| R | Produkte entnehmen (alles, was ins Inventar passt) |
| X | Warten (verbraucht ein Wartungsset) |
| F | Untersuchen: Zustand, Fortschritt, Strom, Wartung, Belegung, was sie kann |

**Kette:** Gerät oder Schrott → **Schredder** → Schreddergut → **Sortiermaschine** → reine Stoffe → **Metallpresse** → Ballen.

| Maschine | Preis | Strom | Rezepte (Auswahl automatisch nach Eingang) |
|---|---|---|---|
| Schredder | 450 € | 7,5 kW | Kabelkarton → 3–4 Kupfergranulat + Kunststoff; Elektronik/Haushaltsgeräte/Elektrowerkzeug → Schreddergut Elektronik (0,9 je kg); Metallschrott/Motoren/Werkzeug/Ersatzteile → Schreddergut Metall (0,95 je kg). Nimmt sperrige Geräte. Nie: Sammlerstücke, seltene Funde, Kundengeräte, Rohstoffe |
| Sortiermaschine | 380 € | 4 kW | 5 Schreddergut Elektronik → Elektronikschrott, Stahl, Kunststoff, mit Chance Kupfergranulat/Aluminium; 5 Schreddergut Metall → Stahl, mit Chance Aluminium/Kupfer/Kunststoff |
| Metallpresse | 320 € | 5,5 kW | 20 Stahlschrott → Stahlballen (20 kg); 10 Aluminiumschrott → Aluminiumballen; 20 Kupfergranulat → Kupferpaket. Ballen sind sperrig und landen vor der Presse |

**Zustände:** Aus · Bereit (wartet auf Material) · Läuft · Ausgabe voll · Stromanschluss überlastet · Wartung nötig. Ein Vorgang nimmt sein Material beim Start aus dem Eingang (kann dann nicht mehr herausgenommen werden). Ausschalten pausiert den Vorgang, Einschalten setzt ihn fort. Passt ein Produkt nicht mehr in die Ausgabe, landet es vor der Maschine; eine Maschine startet nur, wenn genug Ausgabeplätze frei sind.

**Strom:** Hausanschluss 11 kW. Eine Maschine startet nur, wenn ihre Leistung noch frei ist, sonst wartet sie ("Stromanschluss überlastet"). Schredder und Sortiermaschine (11,5 kW) laufen also nicht gleichzeitig, bis man das Upgrade **Starkstromanschluss** (300 €, +11 kW) kauft. Eingeschaltete, wartende Maschinen ziehen 0,3–0,4 kW. Der Verbrauch wird alle 5 Minuten als "Stromrechnung" vom Firmenkonto abgebucht (2,50 € je kWh, Spielpreis inkl. Anschlussgebühr). HUD: "Strom x / y kW" unter dem Firmenkonto, sobald Maschinen stehen.

**Verschleiß:** Jeder Vorgang kostet 2–3 % Wartung. Unter 30 % arbeitet die Maschine 30 % langsamer, bei 0 % bleibt sie stehen ("Wartung nötig"). Ein **Wartungsset** (Werkzeug-Versand, ca. 10 €) bringt sie mit [X] wieder auf 100 %.

**Gemessen (Phase 9)** (Test mit echten Daten, je 8 Stück, Strom und Wartung abgezogen): defekter Elektromotor direkt verkauft 8,64 € → recycelt 11,63 €; defekter Röhrenmonitor 2,70 € → 6,76 €; Kabelkarton 4,03 € → 4,77 €. Ein defekter PC bringt geschreddert nur 1,25 € (direkt 5,67 €) – PCs zerlegt oder repariert man besser von Hand. Maschinen lohnen sich also für Schrott und Massenware, nicht für alles; die Investition (1.150 € plus Upgrade) amortisiert sich erst über Menge.

### 3.7 Produktion und Automatisierung (Stand Phase 10)

**Förderbänder** (1 m 25 €, 2 m 45 €, je 0,15 kW) werden im Baumodus gestellt; der Pfeil auf dem Band zeigt die Laufrichtung (60 cm/s). Ein Band **nimmt** am hinteren Ende die Produkte der Maschine bzw. des Materiallagers, das dort steht, und **gibt** vorn an das ab, was davor steht: ein weiteres Band (auch über Eck oder seitlich einmündend), einen Maschineneingang, ein Lager. Berühren genügt (bis ca. 10 cm Abstand). Nimmt das Ziel etwas nicht an (falsches Material, voll, kein Ziel), bleibt der vordere Gegenstand am Ende stehen und das Band staut sich mit Abstand – wie im echten Betrieb muss man die Linie dann umbauen oder leeren.

| Taste am Band | Aktion |
|---|---|
| E | Mit Gegenstand in der Hand: aufs Band legen (an die freie Stelle, wo man steht). Mit leeren Händen: vorderen Gegenstand herunternehmen (Kleinteile ins Inventar, Sperriges in die Hände) |
| Q | Band anhalten / starten |
| F | Untersuchen: was darauf liegt, woher es nimmt, wohin es abgibt |

**Materiallager** (150 €, 30 Plätze, 400 kg, nimmt auch Sperriges): Bänder liefern hinein; ein Band, das am Lager beginnt, holt Material heraus. **[T] Ausgabe:** Alles → nur ein bestimmtes Material → Aus. Endet die Linie an einer Maschine, schickt das Lager nur, was diese Maschine gerade annimmt – so verstopft kein Band mit Material, das niemand braucht.

**Fertigungsautomat** (600 €, 6 kW) macht aus sortiertem Material Produkte. **[T] Rezept:** Automatisch (erstes, für das Material da ist) oder ein fest gewähltes; mit festem Rezept nimmt die Maschine vom Band nur dessen Zutaten. Gilt auch für Schredder, Sortiermaschine und Presse.

| Produkt (Neu) | Zutaten | Zeit | Material direkt verkauft → Produkt |
|---|---|---|---|
| Kupferdrahtspule | 4 Kupfergranulat | 10 s | 5,54 € → 9,90 € |
| Verlängerungskabel (5 m) | 2 Kupfergranulat + 3 Kunststoffteile | 12 s | 2,91 € → 5,94 € |
| Stahlwinkel-Set | 6 Stahlschrott | 8 s | 0,69 € → 1,45 € |

Produkte kauft die Gebrauchtbörse; dazu kommen **Produktionsaufträge**: "Elektrohandel bestellt Verlängerungskabel", "Baumarkt bestellt Stahlwinkel-Sets", "Elektriker-Großhandel sucht Kupferdrahtspulen", "Gießerei kauft ein Kupferpaket". Versiegelte Retourenpaletten kauft weiterhin niemand ungeöffnet zurück.

**Beispiellinie (Phase 10, im Spiel getestet):** PC aufs Zuführband → Schredder → Band → Sortiermaschine → Band → Materiallager → Band → Fertigungsautomat (Stahlwinkel). Nach dem Auflegen arbeitet die Linie ohne Spieler; Bänder ziehen die fertigen Produkte aus den Maschinen, sperrige Produkte (Ballen) bleiben dafür in der Ausgabe statt vor der Maschine zu landen.

### 3.8 Mitarbeiter und Logistik (Stand Phase 11)

**Einstellen:** Marktplatz → Reiter "Personal & Logistik". Immer 3 Bewerber (alle 10 Minuten neue, jede Rolle ist dabei), höchstens 6 Mitarbeiter. Neue Mitarbeiter stehen am Tor. Erfahrung 1–3: schneller laufen und arbeiten (+20 % je Stufe), Lohn +25 % je Stufe.

| Rolle | Lohn (Erfahrung 1) | Aufgabe |
|---|---|---|
| Lagerarbeiter | 0,60 €/min | Transport: Quelle (Lager, Maschinenausgabe, Wagen) → Ziel (Lager, Maschine, Wagen). Trägt bis 25 kg bzw. einen sperrigen Gegenstand bis 30 kg; bringt einer Maschine nur, was sie (mit gewähltem Rezept) verarbeitet. [T] am Mitarbeiter: nur ein bestimmtes Material |
| Maschinenbediener | 0,80 €/min | Bedient eine Maschine: schaltet sie ein, wartet sie mit einem Wartungsset aus seinem Lager (unter 40 %), bringt fertige Produkte ins Lager (wenn kein Band dranhängt). Solange er dort steht, arbeitet die Maschine 15 % schneller |
| Fahrer | 0,70 €/min | Fährt den Firmenwagen: wartet am Wagen auf eine Tour |

**Aufgabe zuweisen:** [E] auf den Mitarbeiter, dann in der Werkstatt hinschauen und [E]: Lagerarbeiter Quelle, dann Ziel; Maschinenbediener Maschine, dann sein Lager ([Q] ohne Lager); Fahrer den Wagen. [X] bricht ab. Am Mitarbeiter: [Q] Pause/Weiterarbeiten, [X] zweimal Entlassen, [F] Details. Im Reiter "Personal & Logistik" geht Pause und Entlassen auch vom Marktplatz aus.

**Löhne:** Alle 5 Minuten wird die gearbeitete Zeit bezahlt (Pausen nicht). Reicht das Geld nicht, legen alle die Arbeit nieder ("Lohn ausstehend"), bis die Schuld bei einer späteren Abrechnung bezahlt ist. Entlassene bekommen ihre Zeit bei der nächsten Abrechnung.

**Wege:** Mitarbeiter laufen auf einem Navigationsnetz, das sich nach der Einrichtung richtet. Gibt es keinen Weg (z. B. Tor zu für den Fahrer), warten sie ("kein Weg frei").

**Firmenwagen** (Upgrade "Alter Transporter", 2.400 €; "Moderner Lieferwagen", 4.500 €: 30 statt 20 Plätze, 1.200 statt 600 kg, Touren 60 % schneller). Steht vor dem Tor. [E] Laderaum, getragene Dinge mit [E] einladen, [R] Sperriges ausladen, [T] Tour wählen, [Q] losfahren (braucht einen Fahrer am Wagen):

| Tour | Dauer (alter Wagen) | Kosten | Ergebnis |
|---|---|---|---|
| Verkaufsfahrt | 4 min | 6 € Sprit | ganzer Laderaum direkt beim besten Händler, +15 % |
| Großeinkauf Altgeräte | 5 min | 8 € + 55 € | 4 PCs, 3 Röhrenmonitore, 4 Kabelkartons (am Marktplatz ~89 €) |
| Abholung Industriemotoren | 5 min | 8 € + 50 € | 2 große Industriemotoren je 180 kg (am Marktplatz ~63 €) |

Während der Tour sind Wagen und Fahrer weg; das Ergebnis meldet sich bei allen Spielern.

**Gabelstapler** (Upgrade, 900 €): [E] aufsteigen; dann trägt man einen Gegenstand bis 1.500 kg auf der Gabel, langsamer (380 cm/s) und ohne Sprint/Baumodus; [X] absteigen. Für den neuen **Großen Industriemotor** (180 kg, Schrotthandel/Abholtour) – der Schredder nimmt jetzt bis 300 kg auf.

**Gemessen** (Test mit echten Daten): Großeinkauf Altgeräte kostet 63 € statt ~89 € am Marktplatz; direkt weiterverkauft brächte er nur ~50 € (keine Endlosschleife). Ein Lagerarbeiter, der 10 Minuten eine Presse aus dem Regal füttert, schafft 7 Stahlballen: Wertzuwachs 6,44 € bei 6,00 € Lohn – Mitarbeiter lohnen sich über Menge und freie Hände, nicht als Gelddruckmaschine.

## 4. Wirtschaft

- Währung: Euro, intern ganzzahlige Cent (`FWEMoney`).
- Einnahmen: Verkauf (gebraucht, repariert, Teile, Rohstoffe), Kunden-, Industrie-, Produktionsaufträge, Lieferverträge.
- Ausgaben: Einkauf, Werkzeuge, Maschinen, Gebäude, Fahrzeuge, Material, Gehälter, Strom, Betriebskosten, Logistik.
- Preisfaktoren: Zustand, Qualität, Materialwert, Nachfrage, Seltenheit, Bearbeitungskosten, Marktschwankung.
- Balance-Leitlinie: kleine Gewinne am Anfang, keine schnelle Eskalation, Millionen erst spät.

### 4.1 Stand Phase 4

- **Firmenkonto:** Start 250 €, ein gemeinsames Konto für alle Koop-Spieler (kein persönliches Konto – so umgesetzt, bis Mike anders entscheidet).
- **Preise:** Marktwert = Grundwert × Zustandsfaktor (Neu 1,0 · Sehr gut 0,85 · Gebraucht 0,7 · Beschädigt 0,5 · Defekt 0,3 · Schrott 0,1) × Qualitätsfaktor (0,75–1,25) × Nachfrage der Kategorie (0,8–1,2, ändert sich alle 5 Minuten leicht). Händler zahlen 60 % davon (× eigener Faktor je Kategorie), beim Kauf zahlt man 130 %.
- **Händler** (am Online-Marktplatz in der Garage):

| Händler | kauft an | verkauft |
|---|---|---|
| Schrotthandel Kowalski | Rohstoffe, Metallschrott (besser), Ersatzteile (schlechter), Motoren | Kabelkartons, alte Elektromotoren |
| Gebrauchtbörse 24 | Elektronik, Ersatzteile, Haushaltsgeräte, Werkzeug, Sammlerstücke | defekte Computer, Röhrenmonitore, RAM |
| Werkzeug-Versand Hammerl | Werkzeug (schlecht) | Schraubendreher, Seitenschneider, Akkuschrauber, Multimeter, Lötkolben |
| Ersatzteil-Express (Phase 6) | Ersatzteile, Elektronik | gebrauchte Netzteile, RAM, Grafikkarte, Mainboard; Kondensatoren-Sets, Reinigungssets |
| Retouren-Großhandel Paletti (Phase 7) | – | Retourenpaletten Elektronik und Werkzeug |

- **Lieferung:** Kleinteile ins Inventar, Großes auf die Lieferpalette am Tor. Händler füllen ihren Vorrat alle 10–30 Minuten auf.
- **Kundenaufträge:** immer 3 Angebote (verschwinden nach 10 Minuten), höchstens 3 angenommene gleichzeitig, Frist 15–30 Minuten, Belohnung ca. 1,3–1,7 × Marktwert. Beispiele: "Bastler sucht Arbeitsspeicher", "Elektriker sucht Kupferkabel", "Recyclinghof nimmt Stahlschrott".
- **Gemessene Startbalance:** Defekten PC kaufen (14,43 €) → zerlegen → Teile verkaufen bringt im Schnitt 20,57 €, also etwa 6 € Gewinn in ein paar Minuten. Über Aufträge mehr. Ein Akkuschrauber (ca. 69 €) ist das erste sinnvolle Sparziel.

### 4.2 Vertical Slice (Stand Phase 5)

Der erste vollständige Ablauf: **Werkzeug und Gerät besorgen → zur Werkbank tragen → zerlegen → "Alles nehmen" → Teile am Marktplatz verkaufen → Geld in die Werkstatt stecken → speichern.** Gemessen (Test mit echten Daten): ein gekaufter defekter PC bringt je nach Zufall 6–21 € Gewinn, der Computer, der schon in der Garage steht, 11–18 €.

**Leitfaden** (oben links, 6 Schritte, rückt nur bei echten Aktionen vor): Werkzeug einstecken → Computer auf die Werkbank → zerlegen → Teile verkaufen → etwas kaufen → speichern. Danach zeigt er als "Nächstes Ziel" das günstigste noch kaufbare Upgrade.

**Werkstatt-Upgrades** (Marktplatz, Spalte "Werkstatt verbessern", vom Firmenkonto, gelten für das ganze Team):

| Upgrade | Preis | Wirkung | Voraussetzung |
|---|---|---|---|
| LED-Werkstattbeleuchtung | 30 € | Lampen doppelt so hell | – |
| Werkbank-Unterschrank | 35 € | +6 Plätze im Werkbanklager | – |
| Zweites Regal | 60 € | neues Regal (15 Plätze, 150 kg) neben dem Tor | – |
| Werkzeuggürtel und Tragegurt | 70 € | +10 kg Tragkraft für alle Spieler | – |
| Arbeitsmatte und Magnetschale | 80 € | Zerlegen 25 % schneller | Werkbank-Unterschrank |
| Profi-Werkbankausstattung | 220 € | nochmals 25 % schneller, +6 Plätze | Arbeitsmatte |

Ziel: Das erste Upgrade nach etwa 2–5 PCs, alle nach etwa einer Stunde. Erste Werte, Balancing in Phase 16.

### 4.3 Reparatur (Stand Phase 6)

- **Wert kaputter Geräte:** Komplexe Geräte sind kaputt deutlich weniger wert als funktionierend (zusätzlicher Faktor im Datensatz: PC ×0,42, Röhrenmonitor ×0,5). Der Alte Computer hat jetzt 75 € Grundwert; defekt kostet er beim Händler 11,36 €.
- **Gemessen** (Test mit echten Daten, 30 PCs, Ersatzteile gebraucht gekauft): Zerlegen +5,81 €, Reparieren +8,20 €, Reparieren und Aufbereiten +17,70 € pro PC. Mit Teilen aus einem zweiten PC statt gekauften lohnt sich die Reparatur noch mehr.
- **Einstiegskosten:** Multimeter ca. 18 €, Reinigungsset ca. 3 €, Lötkolben ca. 22 € (nur für Monitore).
- **Reparaturaufträge:** Belohnung = Wert des reparierten Geräts × 0,45 (PC, ca. 24 €) bzw. × 0,6 (Monitor); das Ersatzteil zahlt die Werkstatt.

### 4.4 Beschaffung (Stand Phase 7)

| Quelle | Wie | Gedanke |
|---|---|---|
| **Schrotthaufen** hinter dem Tor | bis zu 4 Teile liegen herum, gratis; alle 8 Minuten Nachschub | langsam, aber kostenlos; ein Teil bringt direkt verkauft im Schnitt ~1 €, Wert entsteht erst durch Zerlegen/Reparieren; selten (~1 %) ein Sammlerstück |
| **Gebrauchtwaren** (Händler am Marktplatz) | feste Grundangebote plus wechselnde Angebote: bei jedem Nachschub wird gewürfelt, was da ist und in welchem Zustand/Qualität | Nachschauen lohnt sich; gelegentlich Röhrenradio (10 %) oder Sammler-Grafikkarte (5 %) |
| **Retourenpaletten** (Retouren-Großhandel Paletti) | Elektronik 35,10 €, Werkzeug 29,90 €; an der Lieferpalette mit [Q] öffnen | Inhalt direkt verkauft weniger wert als der Preis (Elektronik 27,90 €), Marktwert aber höher (45 €): Gewinn durch Arbeit, Spannung durch seltene Funde (4–6 % der Paletten). Inhalt steht beim Kauf fest (kein Neuwürfeln durch Laden) |
| **Auktionen** (Reiter "Auktionen & Beschaffung") | 3 Lose gleichzeitig, 5–6 Minuten, Start bei 30–35 % des Schätzwerts; ein Mitbieter mit geheimem Limit (60–120 % des Werts) bietet sofort dagegen; Gebote kurz vor Schluss verlängern um 20 s; bezahlt wird erst beim Zuschlag, Ware auf die Palette | Posten günstiger als beim Händler, wenn man den Wert kennt; reicht das Geld beim Zuschlag nicht, verfällt er |
| **Kundenaufträge** (erweitert) | Mindestqualität, große Geräte in der Hand abliefern ("Schule sucht einen funktionierenden PC"), Eilaufträge (10 min, doppelte Belohnung), Sammleraufträge | verbindet Reparatur und Beschaffung |

**Seltene Gegenstände:** Grafikkarte (Sammlerstück, 180 €), Röhrenradio (1958, 120 €), Keramik-Prozessor mit Goldkontakten (90 €, sehr selten), Industrie-Servomotor (220 €). Sie tauchen auf dem Schrotthaufen, in Paletten, bei Händlern und in Auktionen auf. Gemessen: Der Gewinn mit Paletten hängt nicht an ihnen (ohne seltene Funde noch 86 % des Erlöses).

## 5. Progression (Designziele, durch Playtests zu prüfen)

| Abschnitt | Spielzeit (Ziel) | Inhalt |
|---|---|---|
| Früh | 0–5 h | Handarbeit, kleine Gewinne, erste Werkzeuge, Reparaturen, kleine Aufträge |
| Mitte | 5–15 h | Größere Werkstatt, erste Maschinen, Lager, erste Mitarbeiter |
| Fortgeschritten | 15–30 h | Gewerbehalle, Produktionsmaschinen, Förderbänder, Teilautomatisierung |
| Spät | 30–60+ h | Fabriken, komplexe Ketten, Logistik, neue Standorte, Industrieaufträge |

Jede Stufe muss neue Aufgaben, neue Werkzeuge/Maschinen, größere Möglichkeiten, neue Entscheidungen und langfristige Ziele bringen, nicht nur größere Zahlen.

## 6. Langzeitziele

Automatisierte Produktionslinien, mehrere Standorte, Lieferverträge, seltene Baupläne/Sammlerstücke, Unternehmensmanagement. Konkretisierung in Phase 12 und 16.

## 7. Offene Produktentscheidungen (Mike)

1. Persönliches Konto zusätzlich zum Firmenkonto im Koop? (Vorschlag: nein – vorerst so umgesetzt: nur Firmenkonto)
2. Referenzhardware für das 60-FPS-Ziel (Vorschlag: RTX 3060 / Ryzen 5900X als "empfohlen")
3. Sprache der Spieltexte zuerst Deutsch, Englisch, oder beides von Anfang an? (Vorschlag: Texte von Anfang an als lokalisierbare FText, Start auf Deutsch – so umgesetzt in Phase 1)
4. Sollen zwei Spieler schwere Gegenstände gemeinsam tragen können? (Vorschlag: ja, als Koop-Mechanik in Phase 13; bis dahin Hilfsmittel)
