# AFLC-Adaptive-Fan-Logic-Control-
# AFLC Board – Naming Convention & Git Workflow

This document defines the naming convention used for commits, posts, and releases for the AFLC Board (Altium project).

---

## 1. General Naming Format

All releases and major posts follow:

```
AFLC-Board V<Major.Minor>-<Phase> (DD-MM-YYYY) [E#-S#-V#]
```

### Example

```
AFLC-Board V1.0-EVT (07-04-2026) [E1-S2-V7]
```

---

## 2. Field Definition

### 2.1 Project Name

```
AFLC-Board
```

---

### 2.2 Version (Major.Minor)

```
V1.0
```

* Major version (V1, V2, etc.) represents major milestones
* Minor version (.0, .1, etc.) represents sub-revisions within the milestone

---

### 2.3 Phase (Hardware Development Stage)

```
EVT
```

| Phase | Description                                   |
| ----- | --------------------------------------------- |
| EVT   | Engineering Validation Test (prototype stage) |
| DVT   | Design Validation Test                        |
| PVT   | Production Validation Test                    |
| PROD  | Production / final release                    |

---

### 2.4 Date Format

```
(DD-MM-YYYY)
```

Example:

```
(07-04-2026)
```

---

### 2.5 Internal Tracking Code

```
[E#-S#-V#]
```

#### E = State (Project maturity level)

| Code | Meaning                     |
| ---- | --------------------------- |
| E1   | Prototype / EVT             |
| E2   | DVT / PVT                   |
| E3   | Test and Improvements Ready |
| E4   | User-friendly / Final ready |

---

#### S = Step / Stage

```
S2
```

* Defined manually
* Represents current step in development

Example:

* S1: Initial bring-up
* S2: Functional validation
* S3: Optimization

---

#### V = Iteration Counter

```
V7
```

* Incremented on every modification or commit
* Represents the most precise version level
* Independent from V1.0

---

## 3. Versioning Logic

* V1.0 = milestone (e.g. EVT ready)
* V# = micro-iteration counter (every change)

This ensures:

* Clear milestone tracking
* Fine-grain change tracking

---

## 4. Git Commit Convention

Each commit message must follow:

```
[E#-S#-V#] <type>: short description
```

### Examples

```
[E1-S1-V1] init: project structure and first schematic
[E1-S2-V2] feat: added PWM fan header circuit
[E1-S2-V3] fix: corrected pull-down on BOOT0
[E1-S2-V4] update: improved power routing
[E1-S3-V5] test: initial functional validation
```

---

## 5. Commit Types

| Type     | Meaning                  |
| -------- | ------------------------ |
| init     | Initial setup            |
| feat     | New feature              |
| fix      | Bug fix                  |
| update   | Modification/improvement |
| test     | Testing/validation       |
| refactor | Internal redesign        |
| doc      | Documentation            |

---

## 6. Git Tag / Release Convention

Git tags follow:

```
V<Major.Minor>-<Phase>
```

### Example

```
V1.0-EVT
V1.1-DVT
V2.0-PROD
```

Release title:

```
AFLC-Board V1.0-EVT (07-04-2026) [E1-S2-V7]
```

---

## 7. Rules

* The V counter must increment at every commit
* E must reflect real project maturity
* S must remain consistent within a phase
* Tags must only be created for stable milestones

---

## 8. Recommended Workflow

1. Work locally and increment V on each commit
2. Push regularly to GitHub
3. Create a tag when a milestone is reached
4. Create a release using the full naming format

---

## 9. Future Extensions

* Add hardware revision identifiers (Rev A, Rev B)
* Integrate with Altium 365 revision system
* Automate version generation via script

---

Author: AFLC Project
Tool: Altium Designer
Purpose: Structured hardware and PCB development tracking
