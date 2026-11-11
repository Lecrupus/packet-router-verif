---
version: 1
slug: "scripts-templates-dashboard-html-j2"
primary_target: "scripts/templates/dashboard.html.j2"
related_targets: ["scripts/dashboard.py"]
---

# Regression dashboard (scripts/templates/dashboard.html.j2)

Scope: the offline HTML report each regression run writes to `out/<run>/dashboard/index.html`. Mode: Operate.
Audience and job: interviewers (≈30 s, via README screenshot or shared file) must believe verification is closed; the author triages failures, buckets, seeds and coverage holes. Constraints: one self-contained file, no network, system fonts only, every number names its source file, failing and n/a states rendered as clearly as passing ones.

## Direction contract

THESIS: The run is issued like a released engineering drawing: the title block signs it off, every figure is keyed to a numbered source note, and corrections are redlined in place. Refuses the category default of floating KPI cards on a grey/dark canvas.

OWN-WORLD: Whiteprint ground #eef1ef owns the whole page; blueline ink #1f3a8a for all rules, labels and the zone border; body ink #0f1b33; redline #c8302c only for failures, holes and revision clouds; release green #2f7d4f only for the stamp and pass marks. Technical lettering: Bahnschrift / DIN Alternate / Arial Narrow; figures in ui-monospace. Hairline ruled grids instead of cards; state carried by mark shape (✓, ✕, struck, clouded), never hue alone.

STORY: In one glance the visitor reads RELEASED, 527/527, 100% functional, honest waivers; then drills into tests, buckets, bins and files by the sheet's own detail views.

FIRST VIEWPORT: Zone-lettered sheet border (1–8 across, A–E down). Top band: section navigation left, title block right (title, verdict stamp, run, master seed, commit, date, sheet). Below: eight-cell KPI strip, each value with its note number and true-length bar. Then Detail A (results by test, share strips drawn to scale) beside the Notes column listing source files and any redline revision notes.

FORM: Release Drawing (engineering drawing sheet), position 4 of 7 on the grounded list, assigned by the roll; raises kept from declined challengers: cancel-don't-hide, state by shape, exact lengths, one working text size, committed ground. seed:e169fd408494eec0

FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance
