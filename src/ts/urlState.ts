import { Pitch, SPN } from "meantonal";

// SPN (e.g. "C#4", "Bb3") losslessly encodes both the accidental and the
// absolute octave of a pitch. The app's relative LilyPond-style text-box
// format (cantusToString/cantusFromString) is unsuitable for sharing: it
// drops accidentals entirely, and independently resets each voice's first
// note to a fixed octave, which loses the real octave relationship between
// the cantus and counterpoint.
function pitchesToToken(pitches: Pitch[]): string {
    return pitches.map((p) => SPN.fromPitch(p)).join(" ");
}

function pitchesFromToken(token: string): Pitch[] {
    return token.split(" ").map((s) => SPN.toPitch(s));
}

const PANEL_PARAM_BY_SECTION: Record<number, string> = {
    1: "cantus",
    2: "counterpoint",
    3: "unfold",
};
const SECTION_BY_PANEL_PARAM: Record<string, number> = {
    cantus: 1,
    counterpoint: 2,
    unfold: 3,
};

export interface SharedState {
    cantus: Pitch[];
    ctp: Pitch[];
    panel: number;
}

export function buildShareUrl(
    cantus: Pitch[],
    ctp: Pitch[],
    panel: number,
): string {
    const params = new URLSearchParams();
    params.set("cantus", pitchesToToken(cantus));
    params.set("ctp", pitchesToToken(ctp));
    params.set("panel", PANEL_PARAM_BY_SECTION[panel] ?? "cantus");
    return `${location.origin}${location.pathname}?${params.toString()}`;
}

// Never throws. Returns null if cantus/ctp are missing or fail to parse, so
// callers can unconditionally call this on every page load.
export function parseShareState(search: string): SharedState | null {
    const params = new URLSearchParams(search);
    const cantusParam = params.get("cantus");
    const ctpParam = params.get("ctp");
    if (!cantusParam || !ctpParam) return null;

    let cantus: Pitch[];
    let ctp: Pitch[];
    try {
        cantus = pitchesFromToken(cantusParam);
        ctp = pitchesFromToken(ctpParam);
    } catch {
        return null;
    }
    if (cantus.length === 0 || ctp.length === 0) return null;

    const panelParam = params.get("panel");
    const panel =
        panelParam && panelParam in SECTION_BY_PANEL_PARAM
            ? SECTION_BY_PANEL_PARAM[panelParam]
            : 1;

    return { cantus, ctp, panel };
}
