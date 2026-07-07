import { Interval, LilyPond, Pitch } from "meantonal";

// Parses a manually-entered cantus firmus (space-separated LilyPond-style
// note names, e.g. "d e f g a b c d") into a Pitch[], normalizing octaves so
// each note lands within a 4th of the previous one before applying any
// explicit octave marks (' / ,) the user typed.
export function cantusFromString(cantusString: string): Pitch[] {
    const pitchStrings = cantusString.split(" ");
    let pitches = pitchStrings.map(s => LilyPond.toPitch(s));

    pitches[0] = Pitch.fromChroma(pitches[0].chroma, 4);
    for (let i = 1; i < pitches.length; i++) {
        let p = Pitch.fromChroma(pitches[i].chroma, 0);
        let steps = Math.abs(p.stepsTo(pitches[i - 1]))
        while (steps > 3) {
            p = p.transposeReal(new Interval(5, 2));
            steps = Math.abs(p.stepsTo(pitches[i - 1]))
        }
        const raise = pitchStrings[i].split("").filter(x => x === "'").length
        p = p.transposeReal(new Interval(5 * raise, 2 * raise));
        const lower = pitchStrings[i].split("").filter(x => x === ",").length
        p = p.transposeReal(new Interval(-5 * lower, -2 * lower));
        pitches[i] = p;
    }

    return pitches;
}
