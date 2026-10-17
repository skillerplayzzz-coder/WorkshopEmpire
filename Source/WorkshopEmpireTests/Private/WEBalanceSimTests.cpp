#include "Misc/AutomationTest.h"

#include "Economy/WEContractComponent.h"
#include "Economy/WEMarketComponent.h"
#include "Economy/WETraderDefinition.h"
#include "Economy/WETraderTerminal.h"
#include "Framework/WEGameState.h"
#include "HAL/FileManager.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Items/WEPickupItem.h"
#include "Machines/WEMachineDefinition.h"
#include "Machines/WEPowerGridComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/WECharacter.h"
#include "Progression/WEMilestoneComponent.h"
#include "Progression/WEProgressionComponent.h"
#include "Progression/WEUpgradeDefinition.h"
#include "Staff/WEStaffTypes.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "Workshop/WEDisassemblyDefinition.h"
#include "Workshop/WEFurnitureDefinition.h"
#include "Workshop/WERepairDefinition.h"
#include "Workshop/WEWorkbench.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Phase 16: long-term playtest as a simulation on the real game data.
 *
 * Every value comes from the game's own code and Data Assets: item prices from the market and trader
 * rules, disassembly results from a real workbench (averaged over seeds), machine results from the
 * recipes, prices of tools, upgrades, buildings, machines, staff wages, tours and contracts from their
 * catalogs. What a simulation cannot measure - how long a person needs to carry, click and walk - is a
 * documented assumption (FAssumptions). A greedy but sensible player follows a build order for 60 hours;
 * the result (milestones and income per hour) goes to Saved/Balance/ProgressionCurve.md and is checked
 * against the design targets in GAME_DESIGN.md §5.
 */
namespace WEBalanceSim
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** What the simulation cannot measure. Change with care: the targets were set against these. */
	struct FAssumptions
	{
		/** Share of a player's time spent on productive work (the rest: walking, menus, looking around). */
		double Efficiency = 0.7;
		/** Buy, fetch from the pallet, carry to the bench, "take all", sell the parts - per device. */
		double HandleDeviceSeconds = 45.0;
		/** Pressing [E]/[Q] and watching one disassembly or repair step. */
		double PerStepSeconds = 2.0;
		/** Ordering and fetching a spare part for a repair. */
		double FetchPartSeconds = 15.0;
		/** Carrying one device to a machine and its products away, without a warehouse worker. */
		double FeedMachineSeconds = 12.0;
		/** Units one warehouse worker moves per minute (shelf -> machine -> storage, measured ~14). */
		double WorkerUnitsPerMinute = 12.0;
		/** Customer orders a player handles per hour (3 accepted at a time, 15-40 min each). */
		double OrdersPerHour = 4.0;
		/** Industrial orders / deliveries per hour from tier 2 on (2 at a time, 20-60 min each). */
		double IndustrialOrdersPerHour = 1.5;
		/** Loading and unloading the van per tour. */
		double TourOverheadMinutes = 1.0;
		/** Players in the session (co-op multiplies the hands, not the supply). */
		int32 Players = 1;
	};

	struct FUnit
	{
		const UWEItemDefinition* Def = nullptr;
		EWEItemCondition Condition = EWEItemCondition::Used;
		uint8 Quality = 50;
	};

	/** A supply that refills: trader offer (per restock) or a pickup tour (per trip). */
	struct FSource
	{
		FString Name;
		FUnit Unit;
		double PerRefill = 0.0;
		double RefillMinutes = 10.0;
		double UnitCost = 0.0;
		/** Tours only. */
		FName TourId;
		int32 MinTier = 1;
	};

	/** One way to turn a device or material into money. */
	enum class ERoute : uint8 { Sell, PartOut, Repair, Machine };

	struct FRouteValue
	{
		ERoute Route = ERoute::Sell;
		double Euro = 0.0;          // revenue per unit after running costs (parts, power, maintenance)
		double PlayerSeconds = 0.0; // hands needed per unit (0 for machines with workers)
		double ShredderSeconds = 0.0;
	};

	struct FPurchase
	{
		enum class EKind : uint8 { Tool, Upgrade, Furniture, Hire } Kind = EKind::Upgrade;
		FName Id;
		double Cost = 0.0;
		FString Label;
		double BoughtAtHour = -1.0;
	};
}

using namespace WEBalanceSim;
using namespace WETestItems;

namespace WEBalanceSim
{
	/** Prices and route values from the real game rules. */
	class FEconomyModel
	{
	public:
		FWETestWorld TW;
		AWEGameState* GameState = nullptr;
		AWETraderTerminal* Terminal = nullptr;
		AWEWorkbench* Bench = nullptr;
		AWECharacter* Player = nullptr;
		TArray<UWETraderDefinition*> Traders;
		UWEUpgradeCatalog* Upgrades = nullptr;
		UWEFurnitureCatalog* Furniture = nullptr;
		UWEStaffCatalog* Staff = nullptr;
		UWETourCatalog* Tours = nullptr;
		UWEContractCatalog* Contracts = nullptr;
		UWEMilestoneCatalog* Milestones = nullptr;
		bool bValid = false;

		/** Machines and tools the simulated company owns (route values depend on them). */
		TSet<const UWEMachineDefinition*> Machines;
		TSet<FName> Tools;
		double WorkSpeed = 1.0;
		double OperatorSpeed = 1.0;

		FEconomyModel()
		{
			GameState = TW.SpawnGameState<AWEGameState>();
			GameState->GetContracts()->SetCatalog(nullptr);
			for (const TCHAR* Name : { TEXT("Scrapyard"), TEXT("UsedGoods"), TEXT("ToolShop"), TEXT("SpareParts"), TEXT("Returns") })
			{
				Traders.Add(LoadObject<UWETraderDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Economy/DA_Trader_%s.DA_Trader_%s"), Name, Name)));
			}
			Upgrades = LoadObject<UWEUpgradeCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Progression/DA_UpgradeCatalog.DA_UpgradeCatalog"));
			Furniture = LoadObject<UWEFurnitureCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Workshop/DA_FurnitureCatalog.DA_FurnitureCatalog"));
			Staff = LoadObject<UWEStaffCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Staff/DA_StaffCatalog.DA_StaffCatalog"));
			Tours = LoadObject<UWETourCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Staff/DA_TourCatalog.DA_TourCatalog"));
			Contracts = LoadObject<UWEContractCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Economy/DA_ContractCatalog.DA_ContractCatalog"));
			Milestones = LoadObject<UWEMilestoneCatalog>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Progression/DA_MilestoneCatalog.DA_MilestoneCatalog"));
			bValid = !Traders.Contains(nullptr) && Upgrades && Furniture && Staff && Tours && Contracts && Milestones;
			if (!bValid)
			{
				return;
			}
			Terminal = TW.Spawn<AWETraderTerminal>(FVector(0.f, 400.f, 0.f));
			Terminal->SetTraders(Traders);
			Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
			Bench = TW.Spawn<AWEWorkbench>(FVector(150.f, 0.f, 0.f));
		}

		static const UWEItemDefinition* Item(const TCHAR* Id) { return UWEItemDefinition::FindById(FName(Id)); }

		double Sell(const UWEItemDefinition* Def, EWEItemCondition Condition, uint8 Quality, int32 Quantity = 1) const
		{
			return Def ? Terminal->GetBestSellPrice(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), Quantity, Condition, Quality)).GetCents() / 100.0 : 0.0;
		}

		double Buy(const UWEItemDefinition* Def, EWEItemCondition Condition, uint8 Quality, float PriceMultiplier = 1.f) const
		{
			return Def ? UWEMarketComponent::GetBuyPrice(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), 1, Condition, Quality), PriceMultiplier,
				GameState->GetMarket()).GetCents() / 100.0 : 0.0;
		}

		double Market(const UWEItemDefinition* Def, EWEItemCondition Condition, uint8 Quality, int32 Quantity) const
		{
			return Def ? UWEMarketComponent::GetMarketValue(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Def), Quantity, Condition, Quality),
				GameState->GetMarket()).GetCents() / 100.0 : 0.0;
		}

		/** Shop price of a tool or consumable (first trader offer, as in the game). */
		double ShopPrice(const UWEItemDefinition* Def) const
		{
			for (const UWETraderDefinition* T : Traders)
			{
				for (const FWETraderStockEntry& E : T->Stock)
				{
					if (E.Item == Def)
					{
						return Buy(Def, E.Condition, E.Quality, E.PriceMultiplier);
					}
				}
			}
			return 0.0;
		}

		double PowerEuro(const UWEMachineDefinition* M, double Seconds) const
		{
			return Seconds / 3600.0 * M->PowerKw * UWEPowerGridComponent::CentsPerKWh / 100.0;
		}

		double MaintenanceEuro(const UWEMachineDefinition* M) const
		{
			const double Range = FMath::Max(1.0, 100.0 - M->WornThreshold);
			return M->MaintenanceItem ? M->WearPerCycle / Range * ShopPrice(M->MaintenanceItem) : 0.0;
		}

		/** Best value of one unit: sold, or run through the owned machines (recursively). */
		double Value(const UWEItemDefinition* Def, EWEItemCondition Condition, uint8 Quality, int32 Depth = 0) const
		{
			const double Direct = Sell(Def, Condition, Quality);
			return FMath::Max(Direct, ProcessValue(Def, Depth).Euro);
		}

		/** Machine route for a unit (devices go whole into the shredder). */
		FRouteValue ProcessValue(const UWEItemDefinition* Def, int32 Depth = 0) const
		{
			FRouteValue Best;
			Best.Route = ERoute::Machine;
			Best.Euro = -1.0;
			if (!Def || Depth > 5)
			{
				return Best;
			}
			for (const UWEMachineDefinition* M : Machines)
			{
				for (const FWEMachineRecipe& R : M->Recipes)
				{
					const bool bByItem = R.InputItem == Def;
					const bool bByCategory = !R.InputItem && R.InputCategories.Contains(Def->Category);
					if (!bByItem && !bByCategory)
					{
						continue;
					}
					// The game picks the first matching recipe automatically, [T] can choose another: the player chooses the best.
					const int32 Quantity = FMath::Max(1, R.InputQuantity);
					const double InputKg = Def->WeightKg * Quantity;
					const double Seconds = (R.BaseSeconds + R.SecondsPerKg * InputKg) / OperatorSpeed;
					double Out = 0.0;
					for (const FWEMachineOutput& O : R.Outputs)
					{
						const double Expected = O.PerInputKg > 0.f ? FMath::RoundToDouble(InputKg * O.PerInputKg)
							: 0.5 * (O.MinQuantity + O.MaxQuantity);
						Out += O.Chance * Expected * Value(O.Item, O.Condition, static_cast<uint8>(O.Quality), Depth + 1);
					}
					if (R.SecondInputItem)
					{
						Out -= R.SecondInputQuantity * Value(R.SecondInputItem, EWEItemCondition::Used, 50, Depth + 1);
					}
					Out -= PowerEuro(M, Seconds) + MaintenanceEuro(M);
					const double PerUnit = Out / Quantity;
					if (PerUnit > Best.Euro)
					{
						Best.Euro = PerUnit;
						// Only the first machine of a device chain limits throughput (shredders are the bottleneck).
						Best.ShredderSeconds = Depth == 0 ? Seconds / Quantity : 0.0;
					}
				}
			}
			return Best;
		}

		/** Expected parts of a disassembled device, from a real workbench (averaged over seeds). */
		struct FPartsBundle { TArray<FWEItemInstance> Parts; int32 Runs = 0; double StepSeconds = 0.0; int32 Steps = 0; };
		TMap<FString, FPartsBundle> PartsCache;

		const FPartsBundle& PartOutBundle(const FUnit& Unit)
		{
			const FString Key = FString::Printf(TEXT("%s_%d_%d"), *Unit.Def->ItemId.ToString(), static_cast<int32>(Unit.Condition), Unit.Quality);
			if (const FPartsBundle* Found = PartsCache.Find(Key))
			{
				return *Found;
			}
			FPartsBundle Bundle;
			int32 Added = 0;
			for (const TCHAR* ToolId : { TEXT("Screwdriver"), TEXT("SideCutter") })
			{
				Player->GetPlayerInventory()->TryAddItem(FWEItemInstance::Make(const_cast<UWEItemDefinition*>(Item(ToolId))), Added);
			}
			constexpr int32 Runs = 24;
			for (int32 Run = 0; Run < Runs; ++Run)
			{
				Bench->GetStorage()->ClearAll();
				Bench->SetRandomSeed(500 + Run);
				AWEPickupItem* Device = SpawnItem(TW, const_cast<UWEItemDefinition*>(Unit.Def), 1, FVector(100.f, -100.f, 30.f), Unit.Condition);
				FWEItemInstance Instance = Device->GetItem();
				Instance.Quality = Unit.Quality;
				Device->SetItem(Instance);
				Bench->PlaceItem(Device);
				while (Bench->GetPlacedItem() && Bench->TryStartStep(Player))
				{
					Bench->CompleteActiveStep();
				}
				for (const FWEInventorySlotData& Slot : Bench->GetStorage()->GetAllSlots())
				{
					Bundle.Parts.Add(Slot.Item);
				}
			}
			Bundle.Runs = Runs;
			if (const UWEDisassemblyDefinition* Dis = Unit.Def->Disassembly)
			{
				for (const FWEDisassemblyStep& Step : Dis->Steps)
				{
					Bundle.StepSeconds += Step.BaseSeconds;
					++Bundle.Steps;
				}
			}
			return PartsCache.Add(Key, MoveTemp(Bundle));
		}

		FRouteValue PartOutValue(const FUnit& Unit)
		{
			FRouteValue R;
			R.Route = ERoute::PartOut;
			if (!Unit.Def->CanBeDisassembled())
			{
				R.Euro = -1.0;
				return R;
			}
			const FPartsBundle& Bundle = PartOutBundle(Unit);
			double Total = 0.0;
			for (const FWEItemInstance& Part : Bundle.Parts)
			{
				Total += Value(Part.Definition, Part.Condition, Part.Quality) * Part.Quantity;
			}
			R.Euro = Total / FMath::Max(1, Bundle.Runs);
			const double ToolSpeed = Tools.Contains(TEXT("CordlessDrill")) ? 2.5 : Tools.Contains(TEXT("BitRatchet")) ? 1.6 : 1.0;
			R.PlayerSeconds = Assumptions.HandleDeviceSeconds + Bundle.Steps * Assumptions.PerStepSeconds + Bundle.StepSeconds / (ToolSpeed * WorkSpeed);
			return R;
		}

		/** Diagnose, exchange the broken part(s) with bought used parts, test, refurbish, sell. */
		FRouteValue RepairValue(const FUnit& Unit) const
		{
			FRouteValue R;
			R.Route = ERoute::Repair;
			R.Euro = -1.0;
			const UWERepairDefinition* Rep = Unit.Def->Repair;
			if (!Rep || !Tools.Contains(TEXT("Multimeter")) || Rep->Faults.Num() == 0)
			{
				return R;
			}
			double WeightSum = 0.0;
			for (const FWERepairFault& F : Rep->Faults)
			{
				WeightSum += F.Weight;
			}
			double PartCost = 0.0;
			double PartBack = 0.0;
			double FaultSeconds = 0.0;
			bool bNeedsTool = false;
			for (const FWERepairFault& F : Rep->Faults)
			{
				const double P = F.Weight / FMath::Max(0.01, WeightSum) * (1.0 + Rep->SecondFaultChance);
				const bool bStackPart = F.Part && F.Part->bStackable;
				PartCost += P * (bStackPart ? ShopPrice(F.Part) : Buy(F.Part, EWEItemCondition::Used, 55));
				PartBack += F.bReturnsBrokenPart ? P * Sell(F.Part, EWEItemCondition::Defective, Unit.Quality) : 0.0;
				FaultSeconds += P * (F.RemoveSeconds + F.InstallSeconds);
				bNeedsTool |= F.Tool == EWEToolType::SolderingIron && !Tools.Contains(TEXT("SolderingIron"));
			}
			if (bNeedsTool)
			{
				return R;
			}
			// Repaired + refurbished: "Sehr gut", quality + repair and refurbish gains (as the workbench does).
			const uint8 Quality = static_cast<uint8>(FMath::Clamp(Unit.Quality + Rep->RepairQualityGain + Rep->RefurbishQualityGain, 0, 100));
			const double Kit = Rep->RefurbishMaterial ? ShopPrice(Rep->RefurbishMaterial) : 0.0;
			R.Euro = Sell(Unit.Def, EWEItemCondition::VeryGood, Quality) - PartCost + PartBack - Kit;
			const double Speed = (Tools.Contains(TEXT("DigitalMultimeter")) ? 1.5 : 1.0) * WorkSpeed;
			R.PlayerSeconds = Assumptions.HandleDeviceSeconds + Assumptions.FetchPartSeconds + 5 * Assumptions.PerStepSeconds
				+ (Rep->DiagnoseSeconds + FaultSeconds + Rep->TestSeconds + Rep->RefurbishSeconds) / Speed;
			return R;
		}

		FRouteValue MachineValue(const FUnit& Unit, bool bWorkers) const
		{
			FRouteValue R = ProcessValue(Unit.Def);
			R.PlayerSeconds = bWorkers ? 0.0 : Assumptions.FeedMachineSeconds;
			return R;
		}

		FAssumptions Assumptions;
	};

	/** The simulated company over time. */
	struct FCompanySim
	{
		FEconomyModel& E;
		double Money = 250.0;
		double Minute = 0.0;
		int32 Tier = 1;
		double InvestedForValue = 0.0;
		TSet<FName> Owned;              // upgrades and furniture ids
		TMap<EWEStaffRole, int32> Hired;
		int32 Shredders = 0;
		TArray<FSource> Sources;
		TArray<double> Stock;           // units waiting per source
		TArray<double> NextRefill;
		double TourBusyUntil = 0.0;
		double Hands = 0.0;
		FName LastTour;
		TMap<EWEMilestoneKind, double> Counts;
		TSet<FName> MilestonesDone;
		double Revenue = 0.0;
		int32 ToursThisHour = 0;
		TArray<FString> HourDetails;
		TMap<FString, double> RouteThisHour;
		double WorkerUnits = 0.0;
		double ShredderSeconds = 0.0;
		TMap<FString, double> IncomeByRoute;
		TArray<FString> Log;
		TArray<double> MoneyAtHour;
		TArray<double> IncomeAtHour;
		TArray<double> ValueAtHour;
		TArray<int32> TierAtHour;
		double IncomeThisHour = 0.0;
		TMap<int32, double> TierReachedHour;
		TArray<FPurchase> Plan;
		int32 NextPurchase = 0;

		explicit FCompanySim(FEconomyModel& InE) : E(InE) {}

		int32 Staff(EWEStaffRole Role) const { const int32* N = Hired.Find(Role); return N ? *N : 0; }

		double CompanyValue() const { return Money + 0.5 * InvestedForValue; }

		void AddSources()
		{
			for (const UWETraderDefinition* T : E.Traders)
			{
				for (const FWETraderStockEntry& Entry : T->Stock)
				{
					if (!Entry.Item || Entry.Item->IsOpenable() || Entry.Item->Category == EWEItemCategory::Tool || Entry.Item->Category == EWEItemCategory::PowerTool)
					{
						continue; // tools are bought once (plan), pallets and auctions are not simulated
					}
					FSource S;
					S.Name = FString::Printf(TEXT("%s: %s"), *T->DisplayName.ToString(), *Entry.Item->DisplayName.ToString());
					S.Unit = { Entry.Item, Entry.Condition, static_cast<uint8>(Entry.Quality) };
					S.PerRefill = Entry.Quantity * Entry.Chance;
					S.RefillMinutes = FMath::Max(1.f, T->RestockMinutes);
					S.UnitCost = E.Buy(Entry.Item, Entry.Condition, static_cast<uint8>(Entry.Quality), Entry.PriceMultiplier);
					Sources.Add(S);
				}
			}
			for (const FWETourDefinition& Tour : E.Tours->Tours)
			{
				if (Tour.Kind != EWETourKind::Pickup)
				{
					continue;
				}
				for (const FWETourItem& TI : Tour.PickupItems)
				{
					if (!TI.Item)
					{
						continue;
					}
					FSource S;
					S.Name = FString::Printf(TEXT("Tour %s: %s"), *Tour.DisplayName.ToString(), *TI.Item->DisplayName.ToString());
					S.Unit = { TI.Item, TI.Condition, static_cast<uint8>(TI.Quality) };
					S.PerRefill = TI.Quantity * TI.Chance;
					S.TourId = Tour.Id;
					S.MinTier = Tour.MinTier;
					S.RefillMinutes = Tour.Minutes;
					S.UnitCost = 0.0; // the tour price is paid per trip
					Sources.Add(S);
				}
			}
			Stock.Init(0.0, Sources.Num());
			NextRefill.Init(0.0, Sources.Num());
		}

		/** Best route for a unit right now (value per player second decides when hands are short). */
		FRouteValue BestRoute(const FUnit& Unit, bool bWorkers)
		{
			FRouteValue Best;
			Best.Route = ERoute::Sell;
			Best.Euro = E.Sell(Unit.Def, Unit.Condition, Unit.Quality);
			Best.PlayerSeconds = 5.0;
			auto Consider = [&Best](const FRouteValue& R)
			{
				if (R.Euro < 0.0)
				{
					return;
				}
				// Prefer what earns more; between similar results the one needing fewer hands.
				const double Score = R.Euro - 0.02 * R.PlayerSeconds;
				const double BestScore = Best.Euro - 0.02 * Best.PlayerSeconds;
				if (Score > BestScore)
				{
					Best = R;
				}
			};
			Consider(E.PartOutValue(Unit));
			Consider(E.RepairValue(Unit));
			if (E.Machines.Num() > 0)
			{
				Consider(E.MachineValue(Unit, bWorkers));
			}
			return Best;
		}

		/** All routes for a unit, cached until the company buys something (route values depend on machines and tools). */
		TMap<FString, TArray<FRouteValue>> RouteCache;

		const TArray<FRouteValue>& RoutesFor(const FUnit& Unit, bool bWorkers)
		{
			const FString Key = FString::Printf(TEXT("%s_%d_%d_%d"), *Unit.Def->ItemId.ToString(), static_cast<int32>(Unit.Condition), Unit.Quality, bWorkers ? 1 : 0);
			if (const TArray<FRouteValue>* Found = RouteCache.Find(Key))
			{
				return *Found;
			}
			TArray<FRouteValue> Routes;
			FRouteValue SellRoute;
			SellRoute.Euro = E.Sell(Unit.Def, Unit.Condition, Unit.Quality);
			SellRoute.PlayerSeconds = 5.0;
			Routes.Add(SellRoute);
			for (const FRouteValue& R : { E.PartOutValue(Unit), E.RepairValue(Unit) })
			{
				if (R.Euro >= 0.0)
				{
					Routes.Add(R);
				}
			}
			if (Shredders > 0 || E.Machines.Num() > 0)
			{
				const FRouteValue M = E.MachineValue(Unit, bWorkers);
				if (M.Euro >= 0.0 && (M.ShredderSeconds <= 0.0 || Shredders > 0))
				{
					Routes.Add(M);
				}
			}
			return RouteCache.Add(Key, MoveTemp(Routes));
		}

		/** Chooses the most profitable pickup tour available at this tier (profit after processing). */
		const FWETourDefinition* BestTour(double& OutProfit)
		{
			const FWETourDefinition* Best = nullptr;
			OutProfit = 0.0;
			const bool bWorkers = Staff(EWEStaffRole::WarehouseWorker) > 0;
			for (const FWETourDefinition& Tour : E.Tours->Tours)
			{
				if (Tour.Kind != EWETourKind::Pickup || Tour.MinTier > Tier)
				{
					continue;
				}
				bool bTooHeavy = false;
				for (const FWETourItem& TI : Tour.PickupItems)
				{
					bTooHeavy |= TI.Item && TI.Item->WeightKg > 60.f && !Owned.Contains(TEXT("Forklift")); // needs the forklift
				}
				if (bTooHeavy)
				{
					continue;
				}
				double Profit = -(Tour.PickupPrice.GetCents() + Tour.FuelCost.GetCents()) / 100.0;
				for (const FWETourItem& TI : Tour.PickupItems)
				{
					Profit += TI.Quantity * TI.Chance * BestRoute({ TI.Item, TI.Condition, static_cast<uint8>(TI.Quality) }, bWorkers).Euro;
				}
				if (Profit > OutProfit)
				{
					OutProfit = Profit;
					Best = &Tour;
				}
			}
			return Best;
		}

		double ContractPremiumPerHour(const TSet<const UWEItemDefinition*>& Produced) const
		{
			double Normal = 0.0, Industrial = 0.0;
			int32 NumNormal = 0, NumIndustrial = 0;
			for (const FWEContractTemplate& T : E.Contracts->Templates)
			{
				if (!T.Item || T.MinTier > Tier)
				{
					continue;
				}
				double Premium = 0.0;
				const int32 Quantity = (T.MinQuantity + T.MaxQuantity) / 2;
				if (T.Kind == EWEContractKind::Repair)
				{
					if (!E.Tools.Contains(TEXT("Multimeter")))
					{
						continue;
					}
					// Reward = value of the repaired device x factor; the part costs about a third of it.
					Premium = 0.67 * T.RewardFactor * E.Market(T.Item, T.MinCondition, 50, 1);
				}
				else
				{
					if (!Produced.Contains(T.Item))
					{
						continue;
					}
					const uint8 Quality = FMath::Max<uint8>(50, T.MinQuality);
					Premium = T.RewardFactor * E.Market(T.Item, T.MinCondition, Quality, Quantity) - E.Value(T.Item, T.MinCondition, Quality) * Quantity;
					Premium *= FMath::Max(1, T.Rounds);
				}
				if (Premium <= 0.0)
				{
					continue;
				}
				if (T.bIndustrial)
				{
					Industrial += Premium / FMath::Max(1, T.Rounds);
					++NumIndustrial;
				}
				else
				{
					Normal += Premium;
					++NumNormal;
				}
			}
			const double Players = FMath::Min(2, E.Assumptions.Players);
			return (NumNormal ? Normal / NumNormal * E.Assumptions.OrdersPerHour * Players : 0.0)
				+ (NumIndustrial ? Industrial / NumIndustrial * E.Assumptions.IndustrialOrdersPerHour : 0.0);
		}

		void Earn(const FString& Route, double Euro)
		{
			RouteThisHour.FindOrAdd(Route) += Euro;
			Money += Euro;
			IncomeThisHour += Euro;
			IncomeByRoute.FindOrAdd(Route) += Euro;
		}

		void ApplyPurchase(FPurchase& P)
		{
			RouteCache.Reset();
			Money -= P.Cost;
			P.BoughtAtHour = Minute / 60.0;
			Log.Add(FString::Printf(TEXT("%6.1f h  %-40s %10.0f EUR"), P.BoughtAtHour, *P.Label, P.Cost));
			switch (P.Kind)
			{
			case FPurchase::EKind::Tool:
				E.Tools.Add(P.Id);
				break;
			case FPurchase::EKind::Hire:
				Hired.FindOrAdd(P.Id == TEXT("Driver") ? EWEStaffRole::Driver : P.Id == TEXT("Operator") ? EWEStaffRole::MachineOperator : EWEStaffRole::WarehouseWorker)++;
				E.OperatorSpeed = Staff(EWEStaffRole::MachineOperator) > 0 ? 1.15 : 1.0;
				break;
			case FPurchase::EKind::Upgrade:
				Owned.Add(P.Id);
				InvestedForValue += P.Cost;
				for (const FWEUpgradeDefinition& U : E.Upgrades->Upgrades)
				{
					if (U.Id == P.Id)
					{
						E.WorkSpeed *= U.WorkSpeedMultiplier;
						Tier = FMath::Max(Tier, U.CompanyTier);
						if (U.CompanyTier > 0 && !TierReachedHour.Contains(U.CompanyTier))
						{
							TierReachedHour.Add(U.CompanyTier, P.BoughtAtHour);
						}
					}
				}
				break;
			case FPurchase::EKind::Furniture:
				Owned.Add(P.Id);
				InvestedForValue += P.Cost;
				for (const FWEFurnitureDefinition& F : E.Furniture->Items)
				{
					if (F.Id == P.Id && F.Machine)
					{
						E.Machines.Add(F.Machine);
						Shredders += F.Id == TEXT("Shredder") ? 1 : 0;
					}
				}
				break;
			}
		}

		/** Milestone rewards (Phase 16) from the real catalog, with counts estimated from the simulated work. */
		void PayMilestones()
		{
			int32 Employees = 0;
			for (const auto& Pair : Hired)
			{
				Employees += Pair.Value;
			}
			for (const FWEMilestoneDefinition& M : E.Milestones->Milestones)
			{
				if (MilestonesDone.Contains(M.Id))
				{
					continue;
				}
				double Progress = 0.0;
				switch (M.Kind)
				{
				case EWEMilestoneKind::Revenue: Progress = Revenue * 100.0; break;
				case EWEMilestoneKind::CompanyValue: Progress = CompanyValue() * 100.0; break;
				case EWEMilestoneKind::CompanyTier: Progress = Tier; break;
				case EWEMilestoneKind::Employees: Progress = Employees; break;
				case EWEMilestoneKind::RareFinds: Progress = 0.0; break; // luck: not simulated
				default: { const double* C = Counts.Find(M.Kind); Progress = C ? *C : 0.0; } break;
				}
				if (Progress >= M.Target)
				{
					MilestonesDone.Add(M.Id);
					Earn(TEXT("Meilensteine"), M.Reward.GetCents() / 100.0);
					if (M.Reward.GetCents() >= 100000)
					{
						Log.Add(FString::Printf(TEXT("%6.1f h  Meilenstein %s (+%.0f EUR)"), Minute / 60.0, *M.Title.ToString(), M.Reward.GetCents() / 100.0));
					}
				}
			}
		}

		/** Simulates one minute. */
		void Step()
		{
			const bool bWorkers = Staff(EWEStaffRole::WarehouseWorker) > 0;
			// Supply refills.
			for (int32 i = 0; i < Sources.Num(); ++i)
			{
				const FSource& S = Sources[i];
				if (!S.TourId.IsNone() || Minute < NextRefill[i])
				{
					continue;
				}
				Stock[i] = FMath::Max(Stock[i], S.PerRefill);
				NextRefill[i] = Minute + S.RefillMinutes;
			}
			// The van: one pickup at a time, needs a driver.
			if (Owned.Contains(TEXT("OldVan")) && Staff(EWEStaffRole::Driver) > 0 && Minute >= TourBusyUntil)
			{
				double Profit = 0.0;
				if (const FWETourDefinition* Tour = BestTour(Profit))
				{
					const double Price = (Tour->PickupPrice.GetCents() + Tour->FuelCost.GetCents()) / 100.0;
					if (Money >= Price)
					{
						if (Tour->Id != LastTour)
						{
							LastTour = Tour->Id;
							Log.Add(FString::Printf(TEXT("%6.1f h  Tour ab jetzt: %s (Gewinn je Fahrt %.0f EUR)"), Minute / 60.0, *Tour->DisplayName.ToString(), Profit));
						}
						++ToursThisHour;
						Counts.FindOrAdd(EWEMilestoneKind::ToursCompleted) += 1.0;
						Money -= Price;
						IncomeByRoute.FindOrAdd(TEXT("Touren (Einkauf + Sprit)")) -= Price;
						IncomeThisHour -= Price;
						for (int32 i = 0; i < Sources.Num(); ++i)
						{
							if (Sources[i].TourId == Tour->Id)
							{
								Stock[i] += Sources[i].PerRefill;
							}
						}
						const double Speed = Owned.Contains(TEXT("Truck")) ? 2.4 : Owned.Contains(TEXT("ModernVan")) ? 1.6 : 1.0;
						TourBusyUntil = Minute + Tour->Minutes / Speed + E.Assumptions.TourOverheadMinutes;
					}
				}
			}

			// Work: hands, workers and shredders process the waiting supply, best value per hand-second first.
			// Work time carries over a few minutes (a device takes longer than a minute), machines idle otherwise.
			Hands = FMath::Min(Hands + 60.0 * E.Assumptions.Efficiency * E.Assumptions.Players, 300.0 * E.Assumptions.Players);
			WorkerUnits = FMath::Min(WorkerUnits + Staff(EWEStaffRole::WarehouseWorker) * E.Assumptions.WorkerUnitsPerMinute, 5.0 * Staff(EWEStaffRole::WarehouseWorker) * E.Assumptions.WorkerUnitsPerMinute);
			ShredderSeconds = FMath::Min(ShredderSeconds + 60.0 * Shredders, 120.0 * Shredders);
			// Every unit goes the best way the hands allow: hand work (repair, parting out) where it beats the
			// machines, machines with workers for the rest, and nothing is bought that would lose money.
			struct FCandidate { int32 Source; FRouteValue Route; double Profit; };
			TArray<FCandidate> HandWork, FreeWork;
			for (int32 i = 0; i < Sources.Num(); ++i)
			{
				if (Stock[i] < 0.5)
				{
					continue;
				}
				const TArray<FRouteValue>& Routes = RoutesFor(Sources[i].Unit, bWorkers);
				double BestFree = 0.2;
				for (const FRouteValue& R : Routes)
				{
					if (R.PlayerSeconds <= 0.0 && R.Euro - Sources[i].UnitCost > BestFree)
					{
						BestFree = R.Euro - Sources[i].UnitCost;
						FreeWork.RemoveAll([i](const FCandidate& C) { return C.Source == i; });
						FreeWork.Add({ i, R, BestFree });
					}
				}
				for (const FRouteValue& R : Routes)
				{
					const double Profit = R.Euro - Sources[i].UnitCost;
					if (R.PlayerSeconds > 0.0 && Profit > BestFree + 0.5)
					{
						HandWork.Add({ i, R, Profit });
					}
				}
			}
			HandWork.Sort([](const FCandidate& A, const FCandidate& B) { return A.Profit / A.Route.PlayerSeconds > B.Profit / B.Route.PlayerSeconds; });
			FreeWork.Sort([](const FCandidate& A, const FCandidate& B) { return A.Profit > B.Profit; });
			HandWork.Append(FreeWork);
			static const TCHAR* Names[] = { TEXT("Verkauf"), TEXT("Zerlegen"), TEXT("Reparieren"), TEXT("Maschinen") };
			for (const FCandidate& C : HandWork)
			{
				const FSource& S = Sources[C.Source];
				while (Stock[C.Source] >= 0.5 && Money >= S.UnitCost)
				{
					const bool bMachine = C.Route.Route == ERoute::Machine;
					if (Hands < C.Route.PlayerSeconds || (bMachine && ShredderSeconds < C.Route.ShredderSeconds)
						|| (bMachine && C.Route.PlayerSeconds <= 0.0 && WorkerUnits < 1.0))
					{
						break;
					}
					Hands -= C.Route.PlayerSeconds;
					if (bMachine)
					{
						ShredderSeconds -= C.Route.ShredderSeconds;
						WorkerUnits -= C.Route.PlayerSeconds <= 0.0 ? 1.0 : 0.0;
					}
					Stock[C.Source] -= 1.0;
					Money -= S.UnitCost;
					Earn(Names[static_cast<int32>(C.Route.Route)], C.Route.Euro);
					Revenue += C.Route.Euro;
					Counts.FindOrAdd(C.Route.Route == ERoute::PartOut ? EWEMilestoneKind::DevicesDisassembled : C.Route.Route == ERoute::Repair ? EWEMilestoneKind::DevicesRepaired
						: EWEMilestoneKind::MachineCycles) += C.Route.Route == ERoute::Machine ? 1.5 : 1.0; // shredder + share of sorter/press/assembler cycles
					IncomeThisHour -= S.UnitCost;
					IncomeByRoute.FindOrAdd(Names[static_cast<int32>(C.Route.Route)]) -= S.UnitCost;
				}
			}
			// Tour goods nobody has time for pile up; a full yard is sold off as it is.
			for (int32 i = 0; i < Sources.Num(); ++i)
			{
				if (!Sources[i].TourId.IsNone() && Stock[i] > 4.0 * Sources[i].PerRefill + 10.0)
				{
					const double Excess = Stock[i] - 4.0 * Sources[i].PerRefill;
					Earn(TEXT("Verkauf"), Excess * E.Sell(Sources[i].Unit.Def, Sources[i].Unit.Condition, Sources[i].Unit.Quality));
					Stock[i] -= Excess;
				}
			}
			// Orders, wages, power for idle machines are inside the machine values; wages here.
			TSet<const UWEItemDefinition*> Produced;
			for (const FSource& S : Sources)
			{
				Produced.Add(S.Unit.Def);
			}
			for (const UWEMachineDefinition* M : E.Machines)
			{
				for (const FWEMachineRecipe& R : M->Recipes)
				{
					for (const FWEMachineOutput& O : R.Outputs)
					{
						Produced.Add(O.Item);
					}
				}
			}
			for (const auto& Pair : E.PartsCache)
			{
				for (const FWEItemInstance& Part : Pair.Value.Parts)
				{
					Produced.Add(Part.Definition);
				}
			}
			Earn(TEXT("Aufträge (Aufpreis)"), ContractPremiumPerHour(Produced) / 60.0);
			Counts.FindOrAdd(EWEMilestoneKind::OrdersCompleted) += E.Assumptions.OrdersPerHour / 60.0;
			Counts.FindOrAdd(EWEMilestoneKind::IndustrialOrders) += Tier >= 2 ? E.Assumptions.IndustrialOrdersPerHour / 60.0 : 0.0;
			PayMilestones();
			double Wages = 0.0;
			for (const FWEStaffRoleDefinition& Role : E.Staff->Roles)
			{
				Wages += Staff(Role.Role) * Role.WageCentsPerMinute / 100.0;
			}
			Earn(TEXT("Löhne"), -Wages);

			// Buy the next thing of the plan once it is affordable - keeping working capital for an hour of wages and two tours.
			double TourPrice = 0.0;
			if (Owned.Contains(TEXT("OldVan")))
			{
				double Unused = 0.0;
				if (const FWETourDefinition* Tour = BestTour(Unused))
				{
					TourPrice = (Tour->PickupPrice.GetCents() + Tour->FuelCost.GetCents()) / 100.0;
				}
			}
			const double Reserve = Wages * 60.0 + 2.0 * TourPrice;
			while (NextPurchase < Plan.Num() && Money >= Plan[NextPurchase].Cost + Reserve)
			{
				ApplyPurchase(Plan[NextPurchase]);
				++NextPurchase;
			}
			if (Tier >= 4 && CompanyValue() >= UWEProgressionComponent::IndustrialCompanyValueCents / 100.0 && !TierReachedHour.Contains(5))
			{
				Tier = 5;
				TierReachedHour.Add(5, Minute / 60.0);
				Log.Add(FString::Printf(TEXT("%6.1f h  Stufe 5: Industrieunternehmen (Firmenwert %.0f EUR)"), Minute / 60.0, CompanyValue()));
			}

			Minute += 1.0;
			if (FMath::IsNearlyEqual(FMath::Fmod(Minute, 60.0), 0.0))
			{
				MoneyAtHour.Add(Money);
				IncomeAtHour.Add(IncomeThisHour);
				ValueAtHour.Add(CompanyValue());
				TierAtHour.Add(Tier);
				FString Detail = FString::Printf(TEXT("h%d tours=%d"), MoneyAtHour.Num(), ToursThisHour);
				for (const auto& Pair : RouteThisHour)
				{
					Detail += FString::Printf(TEXT(" %s=%.0f"), *Pair.Key, Pair.Value);
				}
				HourDetails.Add(Detail);
				RouteThisHour.Reset();
				ToursThisHour = 0;
				IncomeThisHour = 0.0;
			}
		}
	};

	FPurchase Make(FEconomyModel& E, FPurchase::EKind Kind, const TCHAR* Id)
	{
		FPurchase P;
		P.Kind = Kind;
		P.Id = FName(Id);
		switch (Kind)
		{
		case FPurchase::EKind::Tool:
			P.Cost = E.ShopPrice(FEconomyModel::Item(Id));
			P.Label = FString::Printf(TEXT("Werkzeug: %s"), *FEconomyModel::Item(Id)->DisplayName.ToString());
			break;
		case FPurchase::EKind::Upgrade:
			for (const FWEUpgradeDefinition& U : E.Upgrades->Upgrades)
			{
				if (U.Id == P.Id)
				{
					P.Cost = U.Cost.GetCents() / 100.0;
					P.Label = U.DisplayName.ToString();
				}
			}
			break;
		case FPurchase::EKind::Furniture:
			for (const FWEFurnitureDefinition& F : E.Furniture->Items)
			{
				if (F.Id == P.Id)
				{
					P.Cost = F.Cost.GetCents() / 100.0;
					P.Label = F.DisplayName.ToString();
				}
			}
			break;
		case FPurchase::EKind::Hire:
			P.Cost = 0.0;
			P.Label = FString::Printf(TEXT("Einstellen: %s"), Id);
			break;
		}
		return P;
	}

	/** A sensible build order: tools first, then the workshop, machines, logistics, buildings, more lines. */
	TArray<FPurchase> BuildOrder(FEconomyModel& E)
	{
		using K = FPurchase::EKind;
		TArray<FPurchase> Plan;
		auto Add = [&](K Kind, const TCHAR* Id) { Plan.Add(Make(E, Kind, Id)); };
		Add(K::Tool, TEXT("Screwdriver"));
		Add(K::Tool, TEXT("SideCutter"));
		Add(K::Upgrade, TEXT("BenchCabinet"));
		Add(K::Tool, TEXT("Multimeter"));
		Add(K::Upgrade, TEXT("LedLights"));
		Add(K::Tool, TEXT("CordlessDrill"));
		Add(K::Upgrade, TEXT("WorkMat"));
		Add(K::Tool, TEXT("SolderingIron"));
		Add(K::Upgrade, TEXT("SecondShelf"));
		Add(K::Upgrade, TEXT("ToolBelt"));
		Add(K::Upgrade, TEXT("ProBench"));
		Add(K::Tool, TEXT("DigitalMultimeter"));
		Add(K::Furniture, TEXT("Shredder"));
		Add(K::Upgrade, TEXT("HeavyCurrent"));
		Add(K::Furniture, TEXT("Sorter"));
		Add(K::Furniture, TEXT("Press"));
		Add(K::Furniture, TEXT("Assembler"));
		Add(K::Upgrade, TEXT("OldVan"));
		Add(K::Hire, TEXT("Driver"));
		Add(K::Hire, TEXT("Worker"));
		Add(K::Upgrade, TEXT("Forklift"));
		Add(K::Upgrade, TEXT("WorkshopExtension"));
		Add(K::Furniture, TEXT("Packer"));
		Add(K::Hire, TEXT("Operator"));
		Add(K::Upgrade, TEXT("ModernVan"));
		Add(K::Furniture, TEXT("Shredder"));
		Add(K::Hire, TEXT("Worker"));
		Add(K::Upgrade, TEXT("CommercialHall"));
		Add(K::Furniture, TEXT("Furnace"));
		Add(K::Furniture, TEXT("BigMaterialStore"));
		Add(K::Upgrade, TEXT("Truck"));
		Add(K::Hire, TEXT("Worker"));
		Add(K::Furniture, TEXT("Shredder"));
		Add(K::Furniture, TEXT("Sorter"));
		Add(K::Upgrade, TEXT("FactoryHall"));
		Add(K::Furniture, TEXT("Refinery"));
		Add(K::Hire, TEXT("Worker"));
		Add(K::Furniture, TEXT("Shredder"));
		Add(K::Furniture, TEXT("Sorter"));
		Add(K::Furniture, TEXT("Furnace"));
		Add(K::Furniture, TEXT("Refinery"));
		return Plan;
	}

	/** Value of each supply unit by route with the machines owned at the end (for the report and tuning). */
	FString RouteTable(FCompanySim& Sim)
	{
		FEconomyModel& E = Sim.E;
		FString Out = TEXT("Einheit | Kosten | Verkauf | Zerlegen (s) | Reparieren (s) | Maschinen (s Schredder)\n");
		TSet<FString> Seen;
		for (const FSource& S : Sim.Sources)
		{
			const FString Key = FString::Printf(TEXT("%s/%d/%d"), *S.Unit.Def->ItemId.ToString(), static_cast<int32>(S.Unit.Condition), S.Unit.Quality);
			if (Seen.Contains(Key))
			{
				continue;
			}
			Seen.Add(Key);
			const FRouteValue P = E.PartOutValue(S.Unit);
			const FRouteValue R = E.RepairValue(S.Unit);
			const FRouteValue M = E.MachineValue(S.Unit, true);
			Out += FString::Printf(TEXT("%-34s %7.2f | %7.2f | %7.2f (%3.0f) | %7.2f (%3.0f) | %7.2f (%4.1f)\n"), *Key, S.UnitCost,
				E.Sell(S.Unit.Def, S.Unit.Condition, S.Unit.Quality), P.Euro, P.PlayerSeconds, R.Euro, R.PlayerSeconds, M.Euro, M.ShredderSeconds);
		}
		Out += TEXT("\nTouren (Gewinn je Fahrt nach Verarbeitung, Stufe):\n");
		for (const FWETourDefinition& Tour : E.Tours->Tours)
		{
			if (Tour.Kind != EWETourKind::Pickup)
			{
				continue;
			}
			double Profit = -(Tour.PickupPrice.GetCents() + Tour.FuelCost.GetCents()) / 100.0;
			for (const FWETourItem& TI : Tour.PickupItems)
			{
				Profit += TI.Quantity * TI.Chance * Sim.BestRoute({ TI.Item, TI.Condition, static_cast<uint8>(TI.Quality) }, true).Euro;
			}
			Out += FString::Printf(TEXT("  %-40s Stufe %d  %8.2f EUR\n"), *Tour.DisplayName.ToString(), Tour.MinTier, Profit);
		}
		return Out;
	}

	FString RunCurve(FEconomyModel& E, int32 Hours, FCompanySim*& OutSim)
	{
		FCompanySim* Sim = new FCompanySim(E);
		Sim->Plan = BuildOrder(E);
		Sim->AddSources();
		for (int32 M = 0; M < Hours * 60; ++M)
		{
			Sim->Step();
		}
		OutSim = Sim;
		FString Report;
		Report += FString::Printf(TEXT("Spieler: %d, Effizienz %.0f %%\n\n"), E.Assumptions.Players, E.Assumptions.Efficiency * 100.0);
		Report += TEXT("Kauf-Zeitpunkte:\n");
		for (const FString& Line : Sim->Log)
		{
			Report += Line + TEXT("\n");
		}
		Report += TEXT("\nStunde | Konto | Ergebnis der Stunde | Firmenwert | Stufe\n");
		for (int32 H = 0; H < Sim->MoneyAtHour.Num(); ++H)
		{
			if (H < 10 || (H + 1) % 5 == 0)
			{
				Report += FString::Printf(TEXT("%3d | %10.0f | %9.0f | %10.0f | %d\n"), H + 1, Sim->MoneyAtHour[H], Sim->IncomeAtHour[H], Sim->ValueAtHour[H], Sim->TierAtHour[H]);
			}
		}
		Report += TEXT("\nDetails je Stunde:\n");
		for (const FString& Line : Sim->HourDetails)
		{
			Report += Line + TEXT("\n");
		}
		Report += TEXT("\nWerte je Einheit am Ende (alle gekauften Maschinen):\n");
		Report += RouteTable(*Sim);
		Report += TEXT("\nErgebnis nach Bereich (gesamt):\n");
		for (const auto& Pair : Sim->IncomeByRoute)
		{
			Report += FString::Printf(TEXT("  %-28s %12.0f EUR\n"), *Pair.Key, Pair.Value);
		}
		return Report;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEProgressionCurveTest, "WorkshopEmpire.Balance.ProgressionCurve", Flags)
bool FWEProgressionCurveTest::RunTest(const FString& Parameters)
{
	FEconomyModel E;
	if (!TestTrue(TEXT("Game data loaded"), E.bValid))
	{
		return false;
	}
	FCompanySim* Solo = nullptr;
	const FString SoloReport = RunCurve(E, 60, Solo);

	FEconomyModel E2;
	E2.Assumptions.Players = 2;
	FCompanySim* Duo = nullptr;
	const FString DuoReport = RunCurve(E2, 60, Duo);

	const FString Path = FPaths::ProjectSavedDir() / TEXT("Balance/ProgressionCurve.md");
	FFileHelper::SaveStringToFile(FString::Printf(TEXT("# Fortschrittskurve (Simulation auf echten Spieldaten)\n\n## Allein\n\n```\n%s```\n\n## Zu zweit\n\n```\n%s```\n"),
		*SoloReport, *DuoReport), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	AddInfo(FString::Printf(TEXT("Report: %s"), *Path));
	for (const FString& Line : Solo->Log)
	{
		AddInfo(Line);
	}
	for (int32 TierIndex = 2; TierIndex <= 5; ++TierIndex)
	{
		const double* Hour = Solo->TierReachedHour.Find(TierIndex);
		AddInfo(FString::Printf(TEXT("Solo tier %d at %s"), TierIndex, Hour ? *FString::Printf(TEXT("%.1f h"), *Hour) : TEXT("never (60 h)")));
		const double* DuoHour = Duo->TierReachedHour.Find(TierIndex);
		AddInfo(FString::Printf(TEXT("Duo tier %d at %s"), TierIndex, DuoHour ? *FString::Printf(TEXT("%.1f h"), *DuoHour) : TEXT("never (60 h)")));
	}
	// Design targets (GAME_DESIGN.md §5) for one attentive player: mid game 5-15 h, advanced 15-30 h, late 30-60 h.
	auto Reached = [](const FCompanySim* Sim, int32 TierIndex) { const double* H = Sim->TierReachedHour.Find(TierIndex); return H ? *H : 1000.0; };
	TestTrue(TEXT("Solo: big workshop (tier 2) after 7-14 h"), Reached(Solo, 2) >= 7.0 && Reached(Solo, 2) <= 14.0);
	TestTrue(TEXT("Solo: commercial hall (tier 3) after 14-26 h"), Reached(Solo, 3) >= 14.0 && Reached(Solo, 3) <= 26.0);
	TestTrue(TEXT("Solo: factory (tier 4) after 26-42 h"), Reached(Solo, 4) >= 26.0 && Reached(Solo, 4) <= 42.0);
	TestTrue(TEXT("Solo: industrial company (tier 5) after 38-60 h"), Reached(Solo, 5) >= 38.0 && Reached(Solo, 5) <= 60.0);
	TestTrue(TEXT("Co-op is faster, but not trivial (tier 5 after 25 h or more)"), Reached(Duo, 5) < Reached(Solo, 5) && Reached(Duo, 5) >= 25.0);
	bool bFirstUpgradeEarly = false;
	for (const FPurchase& P : Solo->Plan)
	{
		bFirstUpgradeEarly |= P.Kind == FPurchase::EKind::Upgrade && P.BoughtAtHour >= 0.0 && P.BoughtAtHour < 1.0;
	}
	TestTrue(TEXT("First workshop upgrade within the first hour"), bFirstUpgradeEarly);
	bool bNoLosingHours = true;
	for (int32 H = 2; H < Solo->IncomeAtHour.Num(); ++H)
	{
		bNoLosingHours &= Solo->IncomeAtHour[H] > -200.0;
	}
	TestTrue(TEXT("No hour after the start loses money (beyond small dips)"), bNoLosingHours);
	delete Solo;
	delete Duo;
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
