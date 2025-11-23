# Error and Validation Flow: Manager Rate Update

## Happy path
- UI (`ConsoleUI::managerAdjustRates`, src/console_ui.cpp) prompts for from/to currency and the new rate.
- UI validates currencies differ and the rate meets the minimum prompt requirement (>= 0.0001).
- UI calls `Manager::setExchangeRate` (src/employee.cpp), which delegates to `ExchangeOffice::updateRate`.
- Domain layer invokes `RateTable::setRate` (src/exchange_manager.cpp) to store the bidirectional rate.
- On success, UI persists rates via `DataStore::saveRates` (src/persistence.cpp) and prints confirmation.

## Red-branch handling (exceptions)
- `ConsoleUI::managerAdjustRates` wraps the flow in a try/catch and reports any failure as: `Rate update failed: <reason>`.
- Validation checks in UI: throws `ExchangeError` if the currencies are identical.
- Domain checks: `RateTable::setRate` throws `ExchangeError` when the rate is non-positive.
- All exceptions in this path bubble to the UI catch block, ensuring the manager menu remains active without crashing and no persistence occurs on failure.

## Error path (call stack)
UI `managerAdjustRates` → Manager `setExchangeRate` → ExchangeOffice `updateRate` → RateTable `setRate` → (throws) → back to UI catch → user-facing error message.
