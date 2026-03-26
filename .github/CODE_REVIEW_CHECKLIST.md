# Code Review Checklist

Use this checklist when reviewing pull requests or before committing significant changes.

## Compilation & Build

- [ ] Code compiles without warnings on both environments:
  - [ ] `pio run -e esp32s3-supermini`
  - [ ] `pio run -e esp32s3-xiao`
- [ ] No new compiler warnings introduced
- [ ] Binary size within acceptable limits (<3MB)

## Code Style

- [ ] K&R brace style, 4-space indent
- [ ] PascalCase for classes (e.g., `Scale`, `Display`)
- [ ] camelCase for methods and variables (e.g., `getWeight()`, `currentWeight`)
- [ ] SCREAMING_SNAKE_CASE for constants (e.g., `MAX_SAMPLES`)
- [ ] No commented-out code left behind
- [ ] No TODO comments without issue reference

## Memory & Performance

- [ ] No heap allocations in loops
- [ ] No blocking operations in main loop
- [ ] Proper use of `static` for local buffers
- [ ] Circular buffers used where appropriate

## Error Handling

- [ ] HX711 connection checked before operations
- [ ] NULL pointer checks for optional components
- [ ] Graceful degradation when hardware unavailable
- [ ] Serial error messages for critical failures

## API Design (if adding endpoints)

- [ ] Follows REST conventions (GET/POST/PUT/DELETE)
- [ ] Proper HTTP status codes returned
- [ ] JSON responses properly formatted
- [ ] API documented in AGENTS.md

## Preferences (NVS)

- [ ] New preference keys documented
- [ ] Default values specified
- [ ] Namespace usage correct (scale, display, wifi)
- [ ] Value validation before save

## Hardware Testing

- [ ] Tested on ESP32-S3 Supermini
- [ ] Tested on XIAO ESP32S3
- [ ] Display still works headless (no OLED)
- [ ] BLE still works when WiFi disabled

## Web UI (if modifying)

- [ ] Alpine.js follows existing patterns
- [ ] No console errors in browser
- [ ] Responsive design works on mobile
- [ ] API calls handle errors gracefully

## Testing

- [ ] Unit tests added for new logic
- [ ] Tests pass: `pio test -e native`
- [ ] No regression in existing tests

## Documentation

- [ ] AGENTS.md updated if needed
- [ ] Comments explain "why", not "what"
- [ ] Complex algorithms documented
- [ ] API changes reflected in docs

## Security

- [ ] No secrets/keys committed
- [ ] No sensitive data in preferences without encryption
- [ ] WiFi credentials properly protected

## Git

- [ ] commits are atomic (one logical change per commit)
- [ ] commit messages are descriptive
- [ ] no large binary files committed
- [ ] branch name follows convention (feature/, bugfix/, etc.)
