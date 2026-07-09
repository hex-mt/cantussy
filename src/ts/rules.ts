import { Cantussy } from "./cantussy.js";
import { regenerateCantus } from "./renderPipeline.js";

// Mirrors src/c/rules.h's RULE_* constants exactly — keep both lists in
// sync when adding/removing a rule.
export const RULE_REGISTRAL_BREAK = 0;
export const RULE_LARGE_UNRECOVERED_LEAP = 1;
export const RULE_CLIMAX_DISCONNECTED = 2;
export const RULE_REPEATED_NOTE = 3;
export const RULE_DISSONANT_LEAP = 4;
export const RULE_MAX_LEAP_SIZE = 5;
export const RULE_TRITONE_MELODIC = 6;
export const RULE_LEAPS_DIVISOR = 7;
export const RULE_MAX_LARGE_LEAPS = 8;
export const RULE_STEPS_PAST_ARPEGGIO = 9;
export const RULE_ARPEGGIO_PAST_STEP = 10;
export const RULE_LARGE_LEAP_PAST_STEP = 11;
export const RULE_MAX_LEAPS_IN_ROW = 12;
export const RULE_BAD_CONSECUTIVE_LEAPS = 13;
export const RULE_MAX_RANGE = 14;
export const RULE_BAD_CLIMAX = 15;
export const RULE_MAX_SAME_DIRECTION = 16;
export const RULE_DISSONANT_OUTLINE = 17;
export const RULE_TRITONE_IN_GESTURE = 18;
export const RULE_NOODLING = 19;
export const RULE_MAX_TONE_REPETITION = 20;
export const RULE_BAD_CADENCE_APPROACH = 21;
export const RULE_STEP_BIAS_CORRECTION = 22;

export type RuleMeta = {
    id: number;
    key: string;
    label: string;
    description: string;
    group: string;
} & (
    | { type: "toggle"; default: 0 | 1 }
    | { type: "numeric"; default: number; min: number; max: number; step?: number }
);

export const RULES: RuleMeta[] = [
    // Range
    {
        id: RULE_REGISTRAL_BREAK, key: "registralBreak", type: "toggle", default: 1,
        group: "Range", label: "Registral fill",
        description: "Require a leap larger than a 3rd to be \"filled in\" stepwise before leaping past the same boundary again.",
    },
    {
        id: RULE_MAX_RANGE, key: "maxRange", type: "numeric", default: 10, min: 6, max: 14,
        group: "Range", label: "Max range",
        description: "Widest interval (in scale steps + 1) the whole cantus firmus may span, e.g. 10 = a 10th.",
    },
    // Climax
    {
        id: RULE_CLIMAX_DISCONNECTED, key: "climaxDisconnected", type: "toggle", default: 1,
        group: "Climax", label: "Disallow disconnected climax",
        description: "Reject reaching the climax again via leaps with no stepwise connection, once range is maximally used.",
    },
    {
        id: RULE_BAD_CLIMAX, key: "badClimax", type: "toggle", default: 1,
        group: "Climax", label: "Avoid leading-tone climax",
        description: "In Ionian/Lydian, avoid climaxing on the leading tone (scale degree 7).",
    },
    // Leaps
    {
        id: RULE_LARGE_UNRECOVERED_LEAP, key: "largeUnrecoveredLeap", type: "toggle", default: 1,
        group: "Leaps", label: "Recover large leaps",
        description: "A leap larger than a 3rd must be followed by opposite-direction motion, not another leap or same-direction step.",
    },
    {
        id: RULE_DISSONANT_LEAP, key: "dissonantLeap", type: "toggle", default: 1,
        group: "Leaps", label: "No tritone leap",
        description: "Disallow a melodic leap that is exactly a tritone in size.",
    },
    {
        id: RULE_MAX_LEAP_SIZE, key: "maxLeapSize", type: "numeric", default: 7, min: 4, max: 12,
        group: "Leaps", label: "Max leap size",
        description: "Largest single melodic leap allowed, in scale steps (7 = an octave).",
    },
    {
        id: RULE_TRITONE_MELODIC, key: "tritoneMelodic", type: "toggle", default: 1,
        group: "Leaps", label: "No melodic tritone",
        description: "Disallow an adjacent melodic tritone (mode-relative).",
    },
    {
        id: RULE_LEAPS_DIVISOR, key: "leapsDivisor", type: "numeric", default: 4, min: 2, max: 16,
        group: "Leaps", label: "Leap frequency limit",
        description: "Higher = stricter. Caps total leaps to roughly (bars / this value).",
    },
    {
        id: RULE_MAX_LARGE_LEAPS, key: "maxLargeLeaps", type: "numeric", default: 2, min: 0, max: 6,
        group: "Leaps", label: "Max large leaps",
        description: "Most leaps larger than a 4th allowed across the whole line.",
    },
    {
        id: RULE_STEPS_PAST_ARPEGGIO, key: "stepsPastArpeggio", type: "toggle", default: 1,
        group: "Leaps", label: "No steps continuing an arpeggio",
        description: "After 2 same-direction leaps, disallow continuing in that direction.",
    },
    {
        id: RULE_ARPEGGIO_PAST_STEP, key: "arpeggioPastStep", type: "toggle", default: 1,
        group: "Leaps", label: "No leap continuing an arpeggio",
        description: "After 1 leap, disallow another same-direction leap.",
    },
    {
        id: RULE_LARGE_LEAP_PAST_STEP, key: "largeLeapPastStep", type: "toggle", default: 1,
        group: "Leaps", label: "No large leap continuing direction",
        description: "Disallow a large leap continuing the same direction as the previous motion.",
    },
    {
        id: RULE_MAX_LEAPS_IN_ROW, key: "maxLeapsInRow", type: "numeric", default: 2, min: 1, max: 5,
        group: "Leaps", label: "Max leaps in a row",
        description: "Most consecutive leaps allowed before a step is required.",
    },
    {
        id: RULE_BAD_CONSECUTIVE_LEAPS, key: "badConsecutiveLeaps", type: "toggle", default: 1,
        group: "Leaps", label: "Restrict consecutive leap sizing",
        description: "Disallow specific 3rd/4th-then-larger-leap sequences.",
    },
    {
        id: RULE_STEP_BIAS_CORRECTION, key: "stepBiasCorrection", type: "toggle", default: 1,
        group: "Leaps", label: "Prefer steps as first choice",
        description: "Bias note-candidate order towards stepwise motion first, to counteract leaps otherwise clustering near the start of the exercise. Turn off to let all melodic intervals compete equally (useful for testing other constraints in isolation).",
    },
    // Direction
    {
        id: RULE_MAX_SAME_DIRECTION, key: "maxSameDirection", type: "numeric", default: 3, min: 2, max: 6,
        group: "Direction", label: "Force direction change",
        description: "Bars of same-direction motion allowed before a turn is required.",
    },
    {
        id: RULE_DISSONANT_OUTLINE, key: "dissonantOutline", type: "toggle", default: 1,
        group: "Direction", label: "Avoid dissonant outlines",
        description: "On a direction change, avoid outlining a 6th, octave, or mode-relative tritone since the last turn.",
    },
    {
        id: RULE_TRITONE_IN_GESTURE, key: "tritoneInGesture", type: "toggle", default: 1,
        group: "Direction", label: "Avoid tritones across a gesture",
        description: "Avoid a tritone forming across a short 2-3 note directional gesture.",
    },
    // Repetition
    {
        id: RULE_REPEATED_NOTE, key: "repeatedNote", type: "toggle", default: 1,
        group: "Repetition", label: "No immediate repetition",
        description: "Disallow repeating the same tone on consecutive notes.",
    },
    {
        id: RULE_NOODLING, key: "noodling", type: "toggle", default: 1,
        group: "Repetition", label: "No noodling",
        description: "Disallow short back-and-forth repetition patterns over the last 3-4 notes.",
    },
    {
        id: RULE_MAX_TONE_REPETITION, key: "maxToneRepetition", type: "numeric", default: 2, min: 1, max: 5,
        group: "Repetition", label: "Max tone repetition",
        description: "Most times an ordinary tone may recur across the whole cantus firmus (the tonic and 2nd degree are held to a stricter limit regardless).",
    },
    // Cadence
    {
        id: RULE_BAD_CADENCE_APPROACH, key: "badCadenceApproach", type: "toggle", default: 1,
        group: "Cadence", label: "Shape cadence approach",
        description: "Disallow a big downward leap or 2 leaps in a row arriving at the penultimate bar, plus a Lydian-specific restriction.",
    },
];

export const RULE_GROUPS = [...new Set(RULES.map((r) => r.group))];

const STORAGE_KEY = "cantusRules";

export const ruleValues: Record<string, number> = Object.fromEntries(
    RULES.map((r) => [r.key, r.default]),
);

export function loadRuleValues() {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return;
    try {
        const parsed = JSON.parse(raw) as Record<string, number>;
        for (const rule of RULES) {
            if (typeof parsed[rule.key] === "number") {
                ruleValues[rule.key] = parsed[rule.key];
            }
        }
    } catch {
        // malformed/stale blob: fall back to defaults already seeded above
    }
}

export function saveRuleValues() {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(ruleValues));
}

export function applyRuleValuesToWasm(cantussy: Cantussy) {
    for (const rule of RULES) {
        cantussy.setRuleParam(rule.id, ruleValues[rule.key]);
    }
}

export function resetRuleValues(cantussy: Cantussy) {
    for (const rule of RULES) {
        ruleValues[rule.key] = rule.default;
    }
    cantussy.resetRuleParams();
    localStorage.removeItem(STORAGE_KEY);
}

// Populates `#rules-controls` (a child of the `#rules-panel` container
// already present in index.html) from the RULES metadata, and wires the
// hand-authored `#reset-rules` button inside the same container.
export function renderRulesPanel(container: HTMLElement, cantussy: Cantussy) {
    const controlsContainer = container.querySelector<HTMLElement>("#rules-controls")!;
    controlsContainer.innerHTML = "";

    for (const group of RULE_GROUPS) {
        const heading = document.createElement("div");
        heading.className = "text-sm text-stone-700/50 dark:text-orange-100/50 mt-4 mb-1 w-full";
        heading.textContent = group;
        controlsContainer.appendChild(heading);

        for (const rule of RULES.filter((r) => r.group === group)) {
            const row = document.createElement("div");
            row.className = "flex items-center gap-3 w-full py-1";
            row.title = rule.description;

            const label = document.createElement("label");
            label.className = "flex-1 text-stone-700 dark:text-orange-100 text-sm";
            label.textContent = rule.label;
            label.htmlFor = `rule-${rule.key}`;
            row.appendChild(label);

            if (rule.type === "toggle") {
                const checkbox = document.createElement("input");
                checkbox.type = "checkbox";
                checkbox.className = "rule-checkbox";
                checkbox.id = `rule-${rule.key}`;
                checkbox.checked = ruleValues[rule.key] === 1;
                checkbox.addEventListener("change", () => {
                    ruleValues[rule.key] = checkbox.checked ? 1 : 0;
                    saveRuleValues();
                    cantussy.setRuleParam(rule.id, ruleValues[rule.key]);
                    regenerateCantus();
                });
                row.appendChild(checkbox);
            } else {
                const valueLabel = document.createElement("span");
                valueLabel.className = "w-8 text-right text-stone-700 dark:text-orange-100 text-sm";
                valueLabel.textContent = `${ruleValues[rule.key]}`;

                const slider = document.createElement("input");
                slider.type = "range";
                slider.className = "slider flex-1";
                slider.id = `rule-${rule.key}`;
                slider.min = `${rule.min}`;
                slider.max = `${rule.max}`;
                slider.step = `${rule.step ?? 1}`;
                slider.value = `${ruleValues[rule.key]}`;
                slider.addEventListener("input", () => {
                    valueLabel.textContent = slider.value;
                });
                slider.addEventListener("change", () => {
                    const value = Number(slider.value);
                    ruleValues[rule.key] = value;
                    saveRuleValues();
                    cantussy.setRuleParam(rule.id, value);
                    regenerateCantus();
                });
                row.appendChild(slider);
                row.appendChild(valueLabel);
            }

            controlsContainer.appendChild(row);
        }
    }

    // `#reset-rules` is a persistent hand-authored element re-queried on
    // every render (including the one this handler itself triggers) — use
    // `.onclick` assignment rather than `addEventListener` so re-rendering
    // replaces the handler instead of stacking a duplicate on top of it.
    const resetButton = container.querySelector<HTMLButtonElement>("#reset-rules")!;
    resetButton.onclick = () => {
        resetRuleValues(cantussy);
        renderRulesPanel(container, cantussy);
        regenerateCantus();
    };
}
