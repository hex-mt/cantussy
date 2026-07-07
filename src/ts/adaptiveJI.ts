import { Interval, Pitch } from "meantonal";

// 53-EDO approximates 5-limit just intonation extremely closely, but only if
// certain scale degrees are spelled with their enharmonic equivalent rather
// than the "diatonically obvious" spelling the generator produces — e.g. a
// major third from C4 sounds like a harsh Pythagorean third spelled as E4,
// but an almost-exact 5:4 ratio spelled as Fb4. These functions re-spell
// (transpose by an enharmonic diesis) exactly the scale degrees where this
// substitution buys a closer-to-JI interval, given the generator's output
// is otherwise based on the diatonic MOS.
//
// This is only ever applied when the user has selected 53-EDO tuning
// (audio.currentEDO === 53); every other supported tuning uses the
// generator's output unmodified.

function respellForAdaptiveJI(primary: Pitch[], other: Pitch[]) {
    return primary.map((p, i) => {
        const otherNote = other[i].chroma;
        if (
            (p.chroma === 6 &&
                (otherNote === 5 || otherNote === 3 || otherNote === 2)) ||
            (p.chroma === 5 && otherNote === 6)
        )
            return p.transposeReal(new Interval(-2, 4));
        if (
            (p.chroma === 2 &&
                (otherNote === 6 || otherNote === 3 || otherNote === -1)) ||
            (p.chroma >= 3 && p.chroma < 7)
        )
            return p.transposeReal(new Interval(-1, 2));
        else if (p.chroma >= 7) return p.transposeReal(new Interval(-2, 4));
        return p;
    });
}

// Single-voice form, used for the cantus firmus alone (no counterpoint
// partner to weigh vertical intervals against).
export function respellCantusForAdaptiveJI(cantus: Pitch[]) {
    return cantus.map((p) => {
        if (p.chroma >= 2 && p.chroma < 7)
            return p.transposeReal(new Interval(-1, 2));
        else if (p.chroma >= 7) return p.transposeReal(new Interval(-2, 4));
        return p;
    });
}

export function respellUpperVoiceForAdaptiveJI(
    upperVoice: Pitch[],
    lowerVoice: Pitch[],
) {
    return respellForAdaptiveJI(upperVoice, lowerVoice);
}

export function respellLowerVoiceForAdaptiveJI(
    upperVoice: Pitch[],
    lowerVoice: Pitch[],
) {
    return respellForAdaptiveJI(lowerVoice, upperVoice);
}
