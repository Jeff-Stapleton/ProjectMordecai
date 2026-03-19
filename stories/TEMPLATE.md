# US-XXX: Title

## Overview
Brief description of the feature and why it matters.

## Execution Mode
<!-- HEADLESS or EDITOR — planner must set this -->
<!-- HEADLESS: Pure C++ work. Compile and test via CLI. No GUI editor needed. -->
<!-- EDITOR: Requires running Unreal Editor (creating DataAssets, Blueprints, level setup via Python scripting). -->
**Mode:** HEADLESS | EDITOR

## References
- Design doc: (link to relevant design doc)
- Agent rules: (any specific constraints from agent_rules_v2.md)

---

## Acceptance Criteria
- [ ] AC-XXX.1: Specific, testable behavior description
- [ ] AC-XXX.2: Specific, testable behavior description

## Technical Notes
Implementation guidance, constraints, patterns to follow.

## Tests Required
- [ ] `Mordecai.Feature.BehaviorDescription` — verifies AC-XXX.1
- [ ] `Mordecai.Feature.BehaviorDescription` — verifies AC-XXX.2

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-XXX]` prefix
