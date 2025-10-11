# Copilot Instructions for DMF_protocol

## Project Overview
This repository implements the SmartKraft DMF protocol, primarily targeting embedded systems (Arduino/ESP). The main logic resides in the `SmartKraft_DMF/` directory, with the entry point in `SmartKraft_DMF.ino`.

## Architecture & Major Components
- **config_store.[h|cpp]**: Handles configuration storage and retrieval.
- **mail_functions.[h|cpp]**: Manages email-related operations and notifications.
- **network_manager.[h|cpp]**: Responsible for network connectivity and communication.
- **scheduler.[h|cpp]**: Implements task scheduling and timing logic.
- **web_handlers.[h|cpp]**: Provides web server endpoints and request handling.
- **i18n_*.h**: Language-specific resources for internationalization (DE, EN, TR).
- **test_functions.[h|cpp]**: Contains test utilities and mock functions for local validation.

## Developer Workflows
- **Build**: Use the Arduino IDE or PlatformIO to build and upload `SmartKraft_DMF.ino` to the target device. Ensure all dependencies are installed and board configuration matches the hardware.
- **Testing**: Manual and code-based tests are in `test_functions.cpp`. There is no automated CI/CD or unit test framework detected; tests are run by invoking test functions directly.
- **Debugging**: Debug via serial output or by adding logging statements. Network and mail modules have their own debug outputs.

## Project-Specific Patterns
- **Header/Source Pairing**: Each major module is split into `.h` and `.cpp` files. Always update both when adding new features.
- **Internationalization**: Add new language support by creating additional `i18n_XX.h` files and updating relevant handlers.
- **Configuration**: Use `config_store` for persistent settings. Avoid hardcoding values in other modules.
- **Web Handlers**: All HTTP endpoints are defined in `web_handlers.cpp`. Follow existing patterns for new endpoints.

## Integration Points
- **External Services**: Email and network modules may require credentials/configuration. Store these securely using `config_store`.
- **Cross-Component Communication**: Modules interact via function calls; avoid global variables except for configuration/state.

## Examples
- To add a new scheduled task, update `scheduler.h/cpp` and register it in `SmartKraft_DMF.ino`.
- To support a new language, duplicate an `i18n_XX.h` file and translate strings.
- To add a new web endpoint, follow the structure in `web_handlers.cpp` and update routing logic.

## Key Files
- `SmartKraft_DMF/SmartKraft_DMF.ino` (main entry)
- `SmartKraft_DMF/config_store.[h|cpp]`
- `SmartKraft_DMF/web_handlers.[h|cpp]`
- `SmartKraft_DMF/scheduler.[h|cpp]`

---
For questions or missing conventions, consult module headers or ask maintainers for clarification.