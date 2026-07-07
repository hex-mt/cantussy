import { Pitch, SPN, TonalContext } from "meantonal";
import { state } from "./state.js";

type EmscriptenType = "number" | "string" | "array" | "boolean" | null;

// The narrow slice of the Emscripten module object this class actually uses.
interface CantussyWasmModule {
    cwrap(
        ident: string,
        returnType: EmscriptenType,
        argTypes: EmscriptenType[],
    ): (...args: number[]) => number;
    _get_ctp(): number;
    HEAP32?: Int32Array;
    asm?: { HEAP32?: Int32Array };
}

// A C `Pitch` is `{ int w; int h; }`: 2 int32 fields per entry.
const PITCH_STRUCT_FIELD_COUNT = 2;
// `ptr` from WASM is a byte offset; >>2 converts it to an Int32Array index
// (each int32 is 4 bytes).
const INT32_INDEX_SHIFT = 2;

function readPitchesFromHeap(
    heap32: Int32Array,
    ptr: number,
    count: number,
): Pitch[] {
    const base = ptr >> INT32_INDEX_SHIFT;
    const pitches: Pitch[] = [];
    for (let i = 0; i < count; i++) {
        const offset = base + i * PITCH_STRUCT_FIELD_COUNT;
        pitches.push(new Pitch(heap32[offset], heap32[offset + 1]));
    }
    return pitches;
}

export class Cantussy {
    private module: CantussyWasmModule;
    private generate_cantus: (mode: number, length: number) => void;
    private get_cantus_value: (index: number) => number;
    private generate_ctp: (x: number) => void;
    private get_solutions: () => number;
    private set_length: (x: number) => void;
    private set_mode: (x: number) => void;
    private set_cantus: (i: number, w: number, h: number) => void;
    constructor(module: CantussyWasmModule) {
        this.module = module;

        this.generate_cantus = this.module.cwrap("generate_cantus", null, [
            "number",
            "number",
        ]);

        this.get_cantus_value = this.module.cwrap("get_cantus_value", "number", [
            "number",
        ]);

        this.generate_ctp = this.module.cwrap("generate_ctp", null, ["number"]);

        this.get_solutions = this.module.cwrap("get_solutions", "number", []);

        this.set_length = this.module.cwrap("set_bars", null, ["number"]);
        this.set_mode = this.module.cwrap("set_mode", null, ["number"]);

        this.set_cantus = this.module.cwrap("set_cantus", null, ["number", "number", "number"]);
    }
    generateCantus() {
        this.generate_cantus(state.actualMode, state.actualLength);

        const values = Array.from({ length: 32 }, (_, i) =>
            this.get_cantus_value(i),
        );

        const reference = SPN.toPitch(state.tonicLetter + "4");
        const ctx = new TonalContext(reference.chroma, state.actualMode);
        let result: Pitch[] = [];
        for (let i = 0; i < state.actualLength; i++) {
            result.push(reference.transposeDiatonic(values[i], ctx));
        }
        return result;
    }

    generateCtp() {
        this.generate_ctp(state.customCantus ? 1 : 0);

        const ptr = this.module._get_ctp();
        const heap32 = this.module.HEAP32 ?? this.module.asm?.HEAP32!;

        return readPitchesFromHeap(heap32, ptr, state.actualLength);
    }

    get solutions() {
        return this.get_solutions();
    }

    updateCantus() {
        this.set_length(state.actualLength)
        this.set_mode(state.actualMode)
        for (let i = 0; i < state.actualLength; i++) {
            this.set_cantus(i, state.cantus[i].w, state.cantus[i].h);
        }
    }
}
