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
import { drawCompound } from "./ts/scoreCompound.js";
import { regenerateCantus, regenerateCtp } from "./ts/renderPipeline.js";
import { cantusFromString } from "./ts/cantusEntry.js";
import { modeLabel, lenLabel, cantusInput, hand, fist } from "./ts/domRefs.js";

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

// Section switching

function showSection(next: number) {
    if (next === state.currentSection) return;

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

// Compound controls

const playCompoundButton = document.getElementById("play-compound")!;
playCompoundButton.addEventListener("click", playCompound);

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

// State initialisation

const CantussyModule = await createCantussyModule();
state.cantussy = new Cantussy(CantussyModule);
const VerovioModule = await createVerovioModule();
state.verovio = new VerovioToolkit(VerovioModule);

showSection(1);
setWaveform("triangle");
regenerateCantus();
setTuning(31);

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
