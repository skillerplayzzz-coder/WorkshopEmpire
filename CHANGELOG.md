# Changelog

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
