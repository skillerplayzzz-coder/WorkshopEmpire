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

### 3.1 Steuerung (Stand Phase 6, später frei belegbar)

| Taste | Aktion |
|---|---|
| W A S D / Maus | Bewegen / Umsehen |
| Shift (halten) | Sprinten |
| Strg oder C (halten) | Ducken |
| Leertaste | Springen |
| E | Kleinteile aufheben (ins Inventar), sperrige Gegenstände tragen, Tor/Werkbank/Regal öffnen; mit Gegenstand in der Hand: Ablegen (dort, wo man hinschaut) |
| F | Untersuchen (Kategorie, Zustand, Qualität, Gewicht, Schätzwert, Zerlegefortschritt) / Schließen |
| R | Zweite Aktion, z. B. Gegenstand von der Werkbank nehmen |
| Q | Reparieren: nächster Reparaturschritt am Gerät auf der Werkbank |
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

Tragen: ein Gegenstand gleichzeitig, bis 30 kg. Schwerere Dinge (z. B. Elektromotor, 45 kg) brauchen später Hilfsmittel (Hubwagen, Gabelstapler) oder einen zweiten Spieler. Ob Koop-Spieler schwere Dinge gemeinsam tragen können, ist eine offene Designfrage (§7).

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
