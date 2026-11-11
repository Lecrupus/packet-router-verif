---
name: packet-router-verif regression sheet
description: An offline regression report drawn as a released engineering drawing on a whiteprint sheet.
colors:
  whiteprint: "#eef1ef"
  whiteprint-band: "#e4e9e7"
  blueline: "#1f3a8a"
  blueline-hair: "#9fb0cf"
  blueline-wash: "#d7deea"
  drafting-ink: "#0f1b33"
  drafting-ink-soft: "#3c4a68"
  redline: "#b42a26"
  release-green: "#2a7148"
typography:
  display:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "26px"
    fontWeight: 750
    lineHeight: 1.1
    letterSpacing: "0.01em"
    fontVariation: "'wdth' 80"
  headline:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "19px"
    fontWeight: 700
    letterSpacing: "0.08em"
    fontVariation: "'wdth' 92"
  title:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "15px"
    fontWeight: 700
    letterSpacing: "0.06em"
    fontVariation: "'wdth' 92"
  body:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "15px"
    fontWeight: 400
    lineHeight: 1.45
    fontFeature: "'tnum'"
    fontVariation: "'wdth' 92"
  body-sm:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "14px"
    fontWeight: 400
    lineHeight: 1.45
    fontFeature: "'tnum'"
    fontVariation: "'wdth' 92"
  label:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "12px"
    fontWeight: 600
    letterSpacing: "0.12em"
    fontVariation: "'wdth' 92"
  figure:
    fontFamily: "'Sheet Mono', ui-monospace, Consolas, monospace"
    fontSize: "26px"
    fontWeight: 600
    lineHeight: 1.15
    letterSpacing: "-0.02em"
  figure-phone:
    fontFamily: "'Sheet Mono', ui-monospace, Consolas, monospace"
    fontSize: "24px"
    fontWeight: 600
    lineHeight: 1.15
    letterSpacing: "-0.02em"
  figure-group:
    fontFamily: "'Sheet Mono', ui-monospace, Consolas, monospace"
    fontSize: "18px"
    fontWeight: 600
    lineHeight: 1
  mono:
    fontFamily: "'Sheet Mono', ui-monospace, Consolas, monospace"
    fontSize: "13px"
    fontWeight: 400
  stamp:
    fontFamily: "'Sheet Sans', Bahnschrift, 'DIN Alternate', 'Arial Narrow', sans-serif"
    fontSize: "17px"
    fontWeight: 800
    lineHeight: 1.1
    letterSpacing: "0.14em"
    fontVariation: "'wdth' 75"
rounded:
  none: "0px"
  revision: "18px"
  note: "50%"
spacing:
  hair: "4px"
  cell-y: "7px"
  cell-x: "10px"
  field: "14px"
  zone-x: "28px"
  gutter: "40px"
components:
  title-block-field:
    backgroundColor: "{colors.whiteprint}"
    textColor: "{colors.drafting-ink}"
    typography: "{typography.mono}"
    rounded: "{rounded.none}"
    padding: "7px 12px"
  kpi-cell:
    backgroundColor: "{colors.whiteprint}"
    textColor: "{colors.drafting-ink}"
    typography: "{typography.figure}"
    rounded: "{rounded.none}"
    padding: "12px 14px 14px"
  kpi-cell-below:
    textColor: "{colors.redline}"
  note-ref:
    textColor: "{colors.blueline}"
    typography: "{typography.label}"
    rounded: "{rounded.note}"
    size: "19px"
  note-ref-hover:
    backgroundColor: "{colors.blueline}"
    textColor: "{colors.whiteprint}"
  section-nav-link:
    textColor: "{colors.drafting-ink}"
    typography: "{typography.title}"
    padding: "4px 0"
  gauge:
    backgroundColor: "{colors.blueline-wash}"
    height: "7px"
  gauge-fill-below:
    backgroundColor: "{colors.redline}"
  table-header:
    textColor: "{colors.blueline}"
    typography: "{typography.label}"
    padding: "6px 10px"
  table-cell:
    textColor: "{colors.drafting-ink}"
    typography: "{typography.body}"
    padding: "7px 10px"
  bin-chip:
    textColor: "{colors.drafting-ink}"
    rounded: "{rounded.none}"
    padding: "2px 7px"
  bin-chip-hole:
    textColor: "{colors.redline}"
  heat-cell:
    textColor: "{colors.drafting-ink}"
    typography: "{typography.mono}"
    width: "46px"
    height: "26px"
  revision-box:
    textColor: "{colors.redline}"
    rounded: "{rounded.revision}"
    padding: "12px 14px"
  stamp-released:
    textColor: "{colors.release-green}"
    typography: "{typography.stamp}"
    padding: "7px 10px"
  stamp-not-released:
    textColor: "{colors.redline}"
    typography: "{typography.stamp}"
    padding: "7px 10px"
---

# Design System: packet-router-verif regression sheet

## Overview

**Creative North Star: "The Release Drawing"**

The report is issued the way an engineering drawing is released: one whiteprint sheet framed by a blueline border, numbered 1 to 8 across the top and lettered down the left margin, with a title block that carries the run's identity and a rubber-stamp verdict. Every section is a zone addressed by its margin letter (A to G), and the same letters drive the section navigation, so "zone D" in a source note and the "D" link are one address. Numbers are keyed to small circled source notes that list the file each figure came from; anything short of sign-off is corrected in place in redline: struck values, red gauges, dashed hole bins, hatched zero cells, and revision triangles in a red revision box.

The sheet is dense on purpose. Bin-level detail, per-module coverage and per-test repro commands sit open on the page in hairline-ruled grids; there are no cards, no panels lifted off the ground, no shadows. Depth is only line weight: a 2px blueline frame, 1px blueline rules between zones and under table heads, 1px pale-blue hairlines inside grids. Lettering is a condensed technical sans with tabular figures; measured values are set in a monospace so columns of figures align like a parts list.

Colour is ink on paper. Blueline does the drafting (rules, labels, bars, links); dark drafting ink does the lettering; redline appears only where something failed, is missing, or is below its limit; release green appears only on the RELEASED stamp and pass marks. State is always carried by a mark's shape as well as its hue: a tick, a cross, a struck value, a dashed outline, a hatched cell, a triangle.

**Key Characteristics:**
- One committed whiteprint ground across the whole page, framed by a zone-lettered blueline border.
- Hairline-ruled grids instead of cards; flat, shadowless, square-cornered.
- Title block with a rotated, double-ruled rubber stamp as the verdict.
- Circled source-note references on every figure, resolved in a Notes column.
- Redline corrections in place: strike-through, red gauge fill, dashed holes, hatched zeros, revision triangles.
- True-length bars and gauges with a tick at the sign-off limit.

## Colors

Drafting ink on a cool whiteprint: one blue that draws, one dark ink that letters, and two signal inks that are rationed.

### Primary
- **Blueline Ink** (blueline): every rule, the sheet frame, zone letters, column numbers, field labels, table heads, link text, note circles, gauge and bar fills, chart series and histogram bars. Also the heat-grid tint base, applied at five stepped opacities.

### Secondary
- **Redline** (redline): failures, holes, below-limit figures and gauges, struck-value strike lines, repro commands, the NOT RELEASED stamp, the revision box and its triangles, the faint failing-row wash (6% opacity) and the hatch in zero heat cells (18% opacity).

### Tertiary
- **Release Green** (release-green): the RELEASED stamp, pass ticks, "Pass" status, "No open revisions" and empty-bucket confirmations. Nothing else.

### Neutral
- **Whiteprint** (whiteprint): the page and sheet ground everywhere, including inside every grid cell; also the text colour on dark heat cells and on hovered note circles.
- **Ruled Band** (whiteprint-band): the one tonal shift, used to highlight a targeted source note.
- **Drafting Ink** (drafting-ink): headings, body lettering, figures, chart callouts and limit ticks on gauges.
- **Soft Drafting Ink** (drafting-ink-soft): secondary lettering: source lines beside headings, sub-values, axis labels, captions, zero counts, struck values.
- **Blueline Hairline** (blueline-hair): the interior grid hairlines of tables, KPI strip, title block, bins and charts; also the scrollbar thumb.
- **Blueline Wash** (blueline-wash): empty track of gauges and share bars.

### Named Rules
**The Rationed Signal Rule.** Redline marks something that failed, is missing, or sits below its sign-off limit; release green marks a pass. Neither is ever decoration, accent, or emphasis.

**The Shape-Before-Hue Rule.** Every red or green state also carries a shape: tick, cross, hole glyph, strike-through, dashed outline, hatch, or triangle. A colour change alone is not a state.

**The Five-Step Tint Rule.** Heat-grid cells use blueline at only five opacities (0.08, 0.20, 0.34, 0.80, 1.00), skipping the middle of the ramp so figures stay legible: dark ink on the three light steps, whiteprint on the two dark ones.

## Typography

**Display Font:** Sheet Sans (Ubuntu Sans variable, embedded, width axis 75 to 100%)
**Body Font:** Sheet Sans at 92% width
**Label/Mono Font:** Sheet Mono (Ubuntu Sans Mono, embedded)

**Character:** Condensed technical lettering in the manner of drawing-office stencils, paired with a monospace for every measured figure so values read like entries in a parts list. Both faces are subset WOFF2 embedded as data URIs; the report never depends on an installed face.

### Hierarchy
- **Display** (750, 26px, 1.1, width 80%, uppercase): the sheet title only.
- **Headline** (700, 19px, 0.08em, uppercase): zone headings and the title-block title (the latter at 750 weight, 0.04em).
- **Title** (700, 15px, 0.06em, uppercase): sub-heads inside a zone; section navigation links at 600, 14px.
- **Body** (400, 15px, 1.45, width 92%, tabular figures): all running text; captions at 14px capped at 75ch; source lines and sub-values at 13px.
- **Label** (600, 12px, 0.12em, uppercase, blueline): title-block field names, KPI names, table column heads.
- **Figure** (Sheet Mono 600, 26px, -0.02em; 24px on phones): KPI values. Title-block values use Sheet Mono 600 at 15px; coverage-group percentages 18px.
- **Mono** (Sheet Mono 400, 13px): test names, file names, repro commands, bin counts; numeric table columns at 14px, right-aligned.
- **Stamp** (800, 17px, 0.14em, width 75%, uppercase): verdict stamp lettering only; the NOT RELEASED variant drops to 15px, 0.1em to fit its cell.

### Named Rules
**The Figures-In-Mono Rule.** Every measured value (percentages, counts, seeds, dates, commit) is set in Sheet Mono; every word that names something is set in Sheet Sans. Tabular numerals are on everywhere.

**The Labels-Are-Field-Names Rule.** The small spaced uppercase label exists only to name a field, a column, or a measured characteristic directly above its value. It never floats above a heading as a decorative tag.

## Layout

The page is one sheet, max 1480px wide, centred with 16px top and 32px bottom margin, framed by a 2px blueline border. A 22px band across the top carries eight equal column numbers; a 40px left gutter carries the zone letter for each section. Each zone is a two-column grid (gutter, content) closed by a 1px blueline rule; zone content pads 20px 28px 26px, and a heading row sits 14px above its content with the source line beside it on the baseline.

The header band is gutter, navigation, then a title block up to 600px wide: three equal field columns plus a 176px stamp cell spanning four rows. The KPI strip is an eight-across hairline grid. Zone B sets the results table beside a 340px Notes column. Functional items flow in an auto-fill grid of 320px minimum columns with 28px column gaps; charts sit side by side at 1.7 : 1.

Spacing rhythm is small and tight: 4px between bins and nav rows, 7 to 10px table cell padding, 12 to 14px field padding, 28px between content columns. Wide tables scroll horizontally inside their zone; the sheet never scrolls sideways.

Responsive: at 1320px the KPI strip becomes four across. At 1100px the title block drops under the navigation, and Notes and charts stack under their tables. At 700px the gutter collapses to zero, the frame's column band and zone letters are hidden, the title block becomes two columns with the stamp on its own full-width row, KPIs go two across, optional table columns hide, and zone padding drops to 16px.

## Elevation & Depth

The system is flat. There are no shadows, no raised surfaces and no tonal card layers; everything sits on the one whiteprint ground. Depth and grouping come from line weight alone: 2px blueline for the sheet frame, header base and title-block divider; 1px blueline for zone rules, table head rules and coverage-group rules; 1px pale-blue hairline for interior grid lines. The only tonal shift is the ruled band behind a targeted source note.

### Named Rules
**The Line-Weight-Is-Depth Rule.** Hierarchy is drawn with three stroke weights (2px blueline, 1px blueline, 1px hairline), never with shadow or fill. If a group needs separating, rule it.

## Shapes

Square corners everywhere, by default. Three rounded forms are native and are the only ones: the 19px circled source-note reference, the revision box (18px radius, 1.5px redline border) that stands in for a revision cloud, and the revision triangle drawn as an SVG outline with its number inside. The verdict stamp is a 4px double-ruled rectangle rotated -6deg (-3deg on phones), the one deliberately off-axis object on the sheet. Holes are 1.5px dashed redline outlines; zero heat cells add a 45-degree redline hatch. State marks are 15px stroke-only SVG symbols (tick, cross, timeout clock, crash zigzag, hole box, n/a slash) drawn in the current text colour.

## Components

### Section Navigation
- **Character:** a drawing's sheet index, not a menu bar.
- **Style:** a wrapping row of uppercase links (600, 14px, 0.06em) in drafting ink, each prefixed by its zone letter in blueline bold.
- **States:** hover draws a 2px blueline underline; focus shows a 2px blueline outline offset 2px.

### Title Block
- **Character:** the drawing's signed-off identity panel.
- **Structure:** hairline-ruled fields, each a blueline label over a Sheet Mono value; the title spans three columns; the stamp cell spans four rows on the right.
- **Missing values:** struck through with a 2px redline line in soft ink, with a 12px redline reason beneath. Never hidden, never left blank.

### Verdict Stamp (signature)
- **RELEASED:** release green, 4px double border, stamp lettering with a small 12px count line ("527/527 pass").
- **NOT RELEASED:** redline, same form, smaller lettering, failing count beneath.
- **Motion:** the only animation on the sheet: a 700ms press from -10deg at 115% scale to its resting angle, eased cubic-bezier(.16, 1, .3, 1), 150ms delay, only under no-preference reduced motion; phones use a gentler press that stays inside the cell; print disables it.

### Measured-Characteristics Strip
- **Style:** eight hairline cells; label with note reference, a 26px mono figure, a 13px soft-ink sub-line with its state mark, and a gauge.
- **Below limit:** figure, sub-line and gauge fill turn redline and the sub-line leads with a cross. Unavailable metrics show a struck "n/a" and a slashed-circle mark.

### Gauges and Share Bars
- **Style:** 7px blueline-wash track with a blueline fill drawn to true length; a 2px drafting-ink tick extends 4px above and below at the sign-off limit. Share bars are 9px, no tick. Below-limit fills are redline.

### Source-Note Reference
- **Style:** 19px circle, 1px blueline border, 12px numeral; links to its note. Hover fills blueline with whiteprint numeral. The Notes list repeats the same circle; a targeted note gets the ruled band and a hairline outline.

### Tables
- **Head:** label style, blueline, 1px blueline rule beneath. **Rows:** hairline separators, 7px 10px cells; numeric columns mono and right-aligned.
- **Failing rows:** 6% redline wash; failing counts bold redline, zero counts soft ink; repro commands in mono redline under the test name.

### Coverage Bins and Heat Grid
- **Bins:** hairline-bordered square chips, name plus mono count. **Holes:** 1.5px dashed redline border, redline text, hole mark.
- **Heat cells:** 46 by 26px, hairline border, 2px spacing, blueline tint at the five steps. **Zero cells:** 1.5px redline border, hatch, redline "0".

### Revision Box
- **Style:** 1.5px redline border, 18px radius, redline heading and text; each revision line led by a numbered redline triangle. When there are none, a green tick line reads "No open revisions."

### Charts
- **Style:** inline SVG drawn to scale. Hairline gridlines, blueline ticks and baselines, 12px soft-ink axis labels. Series at 2.25px: functional solid blueline, line coverage dashed drafting ink (6 4). Sign-off limits dotted (2 3) at 70% opacity. Closure points are whiteprint-filled ink rings with a 1px leader to a 13px callout; histogram bars are solid blueline with mono values above.

## Do's and Don'ts

### Do:
- **Do** keep the whiteprint ground under everything, including inside grids, title block and charts.
- **Do** draw every bar and gauge to true length, with a drafting-ink tick at the sign-off limit where one exists.
- **Do** key each figure to a circled source note and list that note's file in the Notes column.
- **Do** show missing or unavailable values as struck-through text with a stated reason.
- **Do** pair every red or green state with its shape mark (tick, cross, dashed hole, hatch, strike, triangle).
- **Do** set every measured value in Sheet Mono with tabular figures.
- **Do** embed the fonts; the report must open offline with no network request.

### Don't:
- **Don't** lift content into cards, panels, or tiles with fills or shadows; rule it with hairlines instead.
- **Don't** use redline or release green for emphasis, branding, links, or decoration.
- **Don't** round corners outside the three native rounded forms (note circle, revision box, revision triangle).
- **Don't** add motion beyond the stamp press.
- **Don't** set uppercase spaced labels above headings as tags; labels only name the field or column they sit on.
- **Don't** use emoji or text glyphs as state marks; use the stroke-only SVG symbols.
- **Don't** shade heat cells between the 0.34 and 0.80 tint steps.
