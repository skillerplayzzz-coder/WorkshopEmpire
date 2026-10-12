# WORKSHOP EMPIRE – Game Design

Stand: Phase 2 (2026-10-12). Vollständige Spezifikation: `Docs/MASTER_PROMPT_V1.0.md`. Dieses Dokument fasst die Designentscheidungen zusammen und wird mit jeder Phase präzisiert.

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

### 3.1 Steuerung (Stand Phase 2, später frei belegbar)

| Taste | Aktion |
|---|---|
| W A S D / Maus | Bewegen / Umsehen |
| Shift (halten) | Sprinten |
| Strg oder C (halten) | Ducken |
| Leertaste | Springen |
| E | Kleinteile aufheben (ins Inventar), sperrige Gegenstände tragen, Tor/Werkbank/Regal öffnen; mit Gegenstand in der Hand: Ablegen (dort, wo man hinschaut) |
| F | Untersuchen (Kategorie, Zustand, Qualität, Gewicht, Schätzwert) / Schließen |
| Tab oder I | Inventar öffnen/schließen |
| F5 / F9 | Schnellspeichern / Schnellladen (nur Host) |

Im Inventar: Ziehen = verschieben, Shift+Ziehen = halber Stapel, Doppelklick = zwischen Inventar und Lager umlagern, Rechtsklick = in die Welt ablegen (Shift: 1 Stück), aus dem Fenster ziehen = ablegen.

### 3.2 Gegenstände und Inventar (Stand Phase 2)

- **Zwei Arten:** Kleinteile (Schrauben, Kabel, Speicherriegel, Werkzeug) wandern ins Inventar. Sperrige Dinge (Computer, Monitor, Motor, Werkzeugkiste, Kartons) werden in den Händen getragen und physisch abgestellt, z. B. auf Werkbank oder Regal.
- **Spielerinventar:** 20 Plätze, 25 kg. **Werkbank:** 8 Plätze, 60 kg. **Regal:** 15 Plätze, 150 kg.
- **Stapeln:** nur gleiche Gegenstände im gleichen Zustand (Schrauben bis 200, Kupferkabel bis 20).
- **Zustand** Neu → Sehr gut → Gebraucht → Beschädigt → Defekt → Schrott plus Qualität 0–100 bestimmen den Schätzwert (Platzhalter-Formel, echte Preise in Phase 4).
- Im Koop sieht jeder Spieler sein eigenes Inventar; Lager sind gemeinsam.

Tragen: ein Gegenstand gleichzeitig, bis 30 kg. Schwerere Dinge (z. B. Elektromotor, 45 kg) brauchen später Hilfsmittel (Hubwagen, Gabelstapler) oder einen zweiten Spieler. Ob Koop-Spieler schwere Dinge gemeinsam tragen können, ist eine offene Designfrage (§7).

## 4. Wirtschaft

- Währung: Euro, intern ganzzahlige Cent (`FWEMoney`).
- Einnahmen: Verkauf (gebraucht, repariert, Teile, Rohstoffe), Kunden-, Industrie-, Produktionsaufträge, Lieferverträge.
- Ausgaben: Einkauf, Werkzeuge, Maschinen, Gebäude, Fahrzeuge, Material, Gehälter, Strom, Betriebskosten, Logistik.
- Preisfaktoren: Zustand, Qualität, Materialwert, Nachfrage, Seltenheit, Bearbeitungskosten, Marktschwankung.
- Balance-Leitlinie: kleine Gewinne am Anfang, keine schnelle Eskalation, Millionen erst spät.

**Offene Designfrage (für Phase 4):** Gibt es neben dem Firmenkonto ein persönliches Spielerkonto im Koop? Vorschlag: nein, nur Firmenkonto (einfacher, passt zu "gemeinsames Unternehmen").

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

1. Persönliches Konto zusätzlich zum Firmenkonto im Koop? (Vorschlag: nein)
2. Referenzhardware für das 60-FPS-Ziel (Vorschlag: RTX 3060 / Ryzen 5900X als "empfohlen")
3. Sprache der Spieltexte zuerst Deutsch, Englisch, oder beides von Anfang an? (Vorschlag: Texte von Anfang an als lokalisierbare FText, Start auf Deutsch – so umgesetzt in Phase 1)
4. Sollen zwei Spieler schwere Gegenstände gemeinsam tragen können? (Vorschlag: ja, als Koop-Mechanik in Phase 13; bis dahin Hilfsmittel)
