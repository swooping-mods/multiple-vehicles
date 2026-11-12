# Multiple Owned Vehicles: behavior and implementation

Describes the retained payload **0.1.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.1.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.1.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Alt+N buys another copy of the selected garage model at the normal native price. A host-only scoped purchase reserves a collision-free home/type/buyer identity before native TryBuyVehicle. Checked purchase callsites suppress only that transaction’s duplicate lookup and assign its first storage query a reserved negative home ID. Normal lookups outside the synchronous purchase keep vanilla behavior.

## State and lifecycle

Native owned snapshots retain the new storage identity, buyer and model. There is no separate inventory ledger. The temporary reservation and hook scope end with the transaction. Downstream fleet actions must use exact storage IDs rather than model-first APIs; input/world/host gates admit the purchase.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/multiple-vehicles/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/multiple-vehicles/mod.c)
- [base.h](../source/mods/multiple-vehicles/base.h)

## Verification boundary

Experimental virtual-home strategy: save restore, parked-view reconstruction, removal/reinstallation and multiplayer need qualification. Keep the mod available for worlds using these identities; lossless loading without it is not guaranteed.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
