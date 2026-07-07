import { Pitch } from "meantonal";
import { VerovioToolkit } from "verovio/esm";
import { Cantussy } from "./cantussy.js";

export const state = {
    cantus: [] as Pitch[],
    cantusString: "",
    customCantus: false,
    ctp: [] as Pitch[],
    repositionedCantus: [] as Pitch[],
    upperVoice: [] as Pitch[],
    lowerVoice: [] as Pitch[],
    compound: [] as Pitch[],
    compoundAdjusted: [] as Pitch[],
    mode: 6,
    length: 8,
    solfa: false,
    edit: false,
    currentSection: 2,
    verovio: undefined as unknown as VerovioToolkit,
    cantussy: undefined as unknown as Cantussy,
    actualMode: 0,
    tonicLetter: "",
    actualLength: 0,
};
