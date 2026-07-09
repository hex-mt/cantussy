import { state } from "./ts/state.js";
import {
    audio,
    setTuning,
    setWaveform,
    playCantus,
    playCompound,
    playCtp,
    playCtpBottom,
    playCtpTop,
} from "./ts/audio.js";
import createCantussyModule from "./cantus.js";
import createVerovioModule from "verovio/wasm";
import { VerovioToolkit } from "verovio/esm";
import { Cantussy } from "./ts/cantussy.js";
import { drawCantus } from "./ts/scoreCantus.js";
import { renderCtp } from "./ts/scoreCtp.js";
import { drawCompound } from "./ts/scoreCompound.js";
import { regenerateCantus, regenerateCtp } from "./ts/renderPipeline.js";
import { cantusFromString } from "./ts/cantusEntry.js";
import { buildShareUrl, parseShareState } from "./ts/urlState.js";
import {
    modeLabel,
    lenLabel,
    cantusInput,
    hand,
    fist,
    scrollOpen,
    scrollRolled,
} from "./ts/domRefs.js";
import { loadRuleValues, applyRuleValuesToWasm, renderRulesPanel } from "./ts/rules.js";
import { attachTooltip, TooltipHandle } from "./ts/tooltip.js";

// Mode / length controls

const modes = [
    "Lydian",
    "Ionian",
    "Mixolydian",
    "Dorian",
    "Aeolian",
    "Phrygian",
    "Random",
];

function updateModeLabel() {
    modeLabel.innerHTML = modes[state.mode];
}

function updateLenLabel() {
    lenLabel.innerHTML = state.length != 8 ? `${state.length + 9}` : "Random";
}

function handleIncrementMode() {
    state.customCantus = false;
    state.mode = (state.mode + 6) % 7;
    updateModeLabel();
    regenerateCantus();
}

function handleDecrementMode() {
    state.customCantus = false;
    state.mode = (state.mode + 1) % 7;
    updateModeLabel();
    regenerateCantus();
}

function handleIncrementLength() {
    state.customCantus = false;
    state.length = (state.length + 1) % 9;
    updateLenLabel();
    regenerateCantus();
}

function handleDecrementLength() {
    state.customCantus = false;
    state.length = (state.length + 8) % 9;
    updateLenLabel();
    regenerateCantus();
}

// Solfège / edit controls

function toggleSolfa() {
    state.solfa = !state.solfa;
    if (state.solfa)
        document.documentElement.style.setProperty("--box-visibility", "visible");
    else document.documentElement.style.setProperty("--box-visibility", "hidden");
    hand?.classList.toggle("hidden");
    fist?.classList.toggle("hidden");
}

function toggleEdit() {
    if (state.edit) state.edit = false;
    else state.edit = true

    document.getElementById("cantus")?.classList.toggle("hidden")
    document.getElementById("edit-input")?.classList.toggle("hidden")
    cantusInput.focus();
}

function confirmEdit() {
    state.customCantus = true;
    state.cantusString = cantusInput.value;
    state.cantus = cantusFromString(state.cantusString);
    state.actualMode = state.cantus[0].chroma + 1;
    state.actualLength = state.cantus.length;
    state.cantussy.updateCantus();
    regenerateCantus();
    toggleEdit();
}

let rulesOpen = false;

// #rules-panel sits below the button row, out of the way of #cantus and the
// controls — nothing above it ever moves. Since section-1 is
// position:absolute, growing #rules-panel doesn't push anything in normal
// document flow either, so the bottom nav (#main-nav, a normal-flow
// sibling of the sections) is pushed down explicitly here, matching
// #rules-panel's rendered height, rather than by relying on layout flow.
function updateNavOffset() {
    const nav = document.getElementById("main-nav");
    const rulesPanel = document.getElementById("rules-panel");
    if (!nav) return;
    if (rulesOpen && rulesPanel) {
        const extra = rulesPanel.getBoundingClientRect().height;
        nav.style.marginTop = `calc(25rem + ${extra}px + 2rem)`;
    } else {
        nav.style.marginTop = "";
    }
}

function toggleRulesPanel() {
    rulesOpen = !rulesOpen;
    document.getElementById("rules-panel")?.classList.toggle("hidden");
    scrollOpen?.classList.toggle("hidden");
    scrollRolled?.classList.toggle("hidden");
    updateNavOffset();
}

function closeRulesPanel() {
    if (!rulesOpen) return;
    rulesOpen = false;
    document.getElementById("rules-panel")?.classList.add("hidden");
    scrollOpen?.classList.add("hidden");
    scrollRolled?.classList.remove("hidden");
    updateNavOffset();
}

// Section switching

// Matches #main-nav's margin-top transition duration in style.css, so that
// when the rules panel is open, it has time to fold away before the tab
// underneath it switches.
const RULES_FOLD_DELAY_MS = 400;

function showSection(next: number) {
    if (next === state.currentSection) return;

    if (rulesOpen) {
        closeRulesPanel();
        window.setTimeout(() => switchSection(next), RULES_FOLD_DELAY_MS);
        return;
    }

    switchSection(next);
}

function switchSection(next: number) {
    const currEl = document.getElementById(`section-${state.currentSection}`)!;
    const nextEl = document.getElementById(`section-${next}`)!;
    const forward = next > state.currentSection;

    const currButton = document.getElementById(
        `section-button-${state.currentSection}`,
    )!;
    const nextButton = document.getElementById(`section-button-${next}`)!;
    currButton.classList.remove("section-button-active");
    nextButton.classList.add("section-button-active");

    // Remove active from current
    currEl.classList.remove("section-active");
    currEl.classList.add(forward ? "to-left" : "to-right");

    // Prep next off-screen without animating
    nextEl.classList.remove("to-left", "to-right", "section-active");
    nextEl.classList.add(forward ? "from-right" : "from-left");

    // Force reflow to apply staging styles
    void nextEl.offsetWidth;

    // Now animate in
    nextEl.classList.remove("from-right", "from-left");
    nextEl.classList.add("section-active");

    state.currentSection = next;
}

// Shared by all 3 "link" buttons — each only exists on its own panel, so
// state.currentSection is already correct at click time. Confirms the copy
// via the button's own tooltip (already visible, since the button had to be
// hovered to click it) rather than the button's content, so the icon never
// disappears.
function handleCopyLink(tooltip: TooltipHandle) {
    const url = buildShareUrl(
        state.cantus,
        state.ctp,
        state.currentSection,
        audio.currentEDO,
        audio.bpm,
        audio.waveform,
    );
    navigator.clipboard.writeText(url).then(() => {
        tooltip.flash("Link copied to clipboard.");
    });
}

// Theme

function updateTheme() {
    document.documentElement.classList.toggle(
        "dark",
        localStorage.theme === "dark" ||
        (!("theme" in localStorage) &&
            window.matchMedia("(prefers-color-scheme: dark)").matches),
    );
}

// Cantus controls

document
    .getElementById("mode-increment")!
    .addEventListener("click", handleIncrementMode);
document
    .getElementById("mode-decrement")!
    .addEventListener("click", handleDecrementMode);

document
    .getElementById("length-increment")!
    .addEventListener("click", handleIncrementLength);
document
    .getElementById("length-decrement")!
    .addEventListener("click", handleDecrementLength);

document
    .getElementById("randomise-cantus")!
    .addEventListener("click", () => {
        state.customCantus = false;
        regenerateCantus();
    });

document.getElementById("play-cantus")!.addEventListener("click", playCantus);

document.getElementById("solfa")!.addEventListener("click", toggleSolfa);

document.getElementById("edit")!.addEventListener("click", toggleEdit);
document.getElementById("cancel-edit")!.addEventListener("click", toggleEdit);
document.getElementById("confirm-edit")!.addEventListener("click", confirmEdit);
document
    .getElementById("toggle-rules")!
    .addEventListener("click", toggleRulesPanel);
const linkCantusButton = document.getElementById("link-cantus")!;
const linkCantusTooltip = attachTooltip(linkCantusButton, "Copy shareable link");
linkCantusButton.addEventListener("click", () => handleCopyLink(linkCantusTooltip));
attachTooltip(
    document.getElementById("toggle-rules")!,
    "Tweak cantus generation rules",
);
attachTooltip(
    document.getElementById("randomise-cantus")!,
    "Regenerate cantus",
);
attachTooltip(document.getElementById("play-cantus")!, "Perform cantus");
attachTooltip(document.getElementById("solfa")!, "Toggle solfeggio");
attachTooltip(document.getElementById("edit")!, "Edit cantus");
attachTooltip(document.getElementById("randomise-both")!, "Regenerate both");
attachTooltip(
    document.getElementById("randomise-ctp")!,
    "Regenerate counterpoint",
);
attachTooltip(document.getElementById("play-ctp-top")!, "Perform upper voice");
attachTooltip(
    document.getElementById("play-ctp-bottom")!,
    "Perform lower voice",
);
attachTooltip(document.getElementById("play-ctp")!, "Perform both");
attachTooltip(document.getElementById("play-compound")!, "Perform melody");
cantusInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter")
        confirmEdit();
});

// Counterpoint controls

document
    .getElementById("randomise-both")!
    .addEventListener("click", () => {
        state.customCantus = false;
        regenerateCantus();
    });
document
    .getElementById("randomise-ctp")!
    .addEventListener("click", () => regenerateCtp());

document.getElementById("play-ctp-top")!.addEventListener("click", playCtpTop);
document
    .getElementById("play-ctp-bottom")!
    .addEventListener("click", playCtpBottom);
document.getElementById("play-ctp")!.addEventListener("click", playCtp);
const linkCtpButton = document.getElementById("link-ctp")!;
const linkCtpTooltip = attachTooltip(linkCtpButton, "Copy shareable link");
linkCtpButton.addEventListener("click", () => handleCopyLink(linkCtpTooltip));

// Compound controls

const playCompoundButton = document.getElementById("play-compound")!;
playCompoundButton.addEventListener("click", playCompound);
const linkCompoundButton = document.getElementById("link-compound")!;
const linkCompoundTooltip = attachTooltip(
    linkCompoundButton,
    "Copy shareable link",
);
linkCompoundButton.addEventListener("click", () =>
    handleCopyLink(linkCompoundTooltip),
);

// Setting controls

[12, 19, 31, 50, 53, 55].forEach((edo) => {
    document.getElementById(`edo-${edo}`)?.addEventListener("click", () => {
        setTuning(edo);
        drawCompound();
    });
});

const bpmValue = document.getElementById("bpm-label")! as HTMLSpanElement;
document.getElementById("bpm-slider")!.addEventListener("input", (event) => {
    audio.bpm = bpmValue.textContent = (event.target as HTMLInputElement).value;
});

["triangle", "sawtooth", "square"].forEach((waveform) => {
    document.getElementById(waveform)?.addEventListener("click", () => {
        setWaveform(waveform as OscillatorType);
    });
});

document.getElementById("light-mode")?.addEventListener("click", () => {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document.getElementById("light-mode")?.classList.add("section-button-active");
    localStorage.theme = "light";
    updateTheme();
})

document.getElementById("dark-mode")?.addEventListener("click", () => {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document.getElementById("dark-mode")?.classList.add("section-button-active");
    localStorage.theme = "dark";
    updateTheme();
})

document.getElementById("system-mode")?.addEventListener("click", () => {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document.getElementById("system-mode")?.classList.add("section-button-active");
    localStorage.removeItem("theme");
    updateTheme();
})

// Section switching controls

for (let i = 1; i <= 4; i++) {
    document
        .getElementById(`section-button-${i}`)
        ?.addEventListener("click", () => {
            showSection(i);
        });
}

// Applies a shared cantus/counterpoint/panel/tuning/tempo/waveform from the
// URL's query string on top of the already-fully-initialised app state,
// mirroring confirmEdit()'s manual-entry flow for the cantus half. No-ops
// silently if there's nothing to restore or the params are malformed.
// Always scrubs the query string afterwards so the address bar returns to a
// plain URL either way.
async function applySharedStateFromUrl() {
    const shared = parseShareState(window.location.search);

    if (shared) {
        state.customCantus = true;
        state.cantus = shared.cantus;
        state.actualMode = state.cantus[0].chroma + 1;
        state.actualLength = state.cantus.length;
        state.cantussy.updateCantus();
        await drawCantus();

        state.ctp = shared.ctp;
        setTuning(shared.edo);
        await renderCtp();
        await drawCompound();

        audio.bpm = bpmValue.textContent = shared.bpm;
        (document.getElementById("bpm-slider") as HTMLInputElement).value =
            shared.bpm;

        setWaveform(shared.waveform);

        showSection(shared.panel);
    }

    history.replaceState(null, "", location.pathname);
}

// State initialisation

const CantussyModule = await createCantussyModule();
state.cantussy = new Cantussy(CantussyModule);
loadRuleValues();
applyRuleValuesToWasm(state.cantussy);
renderRulesPanel(document.getElementById("rules-panel")!, state.cantussy);
const VerovioModule = await createVerovioModule();
state.verovio = new VerovioToolkit(VerovioModule);

showSection(1);
setWaveform("triangle");
await regenerateCantus();
setTuning(31);

await applySharedStateFromUrl();

if (localStorage.theme === "light") {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document
        .getElementById("light-mode")
        ?.classList.add("section-button-active");
} else if (localStorage.theme === "dark") {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document
        .getElementById("dark-mode")
        ?.classList.add("section-button-active");
} else {
    document
        .querySelectorAll("#theme-buttons button")
        .forEach((x) => x?.classList.remove("section-button-active"));
    document
        .getElementById("system-mode")
        ?.classList.add("section-button-active");
}
