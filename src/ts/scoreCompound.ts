import { Pitch } from "meantonal";
import { audio } from "./audio.js";
import { state } from "./state.js";
import {
    respellUpperVoiceForAdaptiveJI,
    respellLowerVoiceForAdaptiveJI
} from "./adaptiveJI.js";

// Reference pitches used to pick the nearest of the 3 clef options for the
// single-staff unfolded compound melody.
const TREBLE_CLEF_CENTER = new Pitch(30, 11);
const TREBLE_8VB_CLEF_CENTER = new Pitch(25, 9);
const BASS_CLEF_CENTER = new Pitch(21, 8);

function unfoldCtp() {
    return state.lowerVoice.flatMap((val, i) => [val, state.upperVoice[i], val]);
}

function unfoldAdjustedCtp() {
    const upper = respellUpperVoiceForAdaptiveJI(state.upperVoice, state.lowerVoice);
    const lower = respellLowerVoiceForAdaptiveJI(state.upperVoice, state.lowerVoice);
    return lower.flatMap((val, i) => [val, upper[i], val]);
}

export async function drawCompound() {
    audio.stop();

    if (state.ctp[0].isEqual(new Pitch(0, 0))) {
        document.getElementById("compound")!.innerHTML =
            `<div class="p-6 my-8 border-stone-700 dark:border-orange-100 border-2 text-stone-700 dark:text-orange-100 text-xl">No solutions found :(</div>`;
        return;
    }

    state.compound = unfoldCtp();
    state.compoundAdjusted =
        audio.currentEDO === 53 ? unfoldAdjustedCtp() : state.compound;

    const top = Pitch.highest(state.compound);
    const bottom = Pitch.lowest(state.compound);
    let middle = new Pitch((top.w + bottom.w) / 2, (top.h + bottom.h) / 2);
    let clefIndex = [
        TREBLE_CLEF_CENTER,
        TREBLE_8VB_CLEF_CENTER,
        BASS_CLEF_CENTER,
    ].reduce(
        (a, c, i) => {
            const distance = Math.abs(middle.stepsTo(c));
            if (distance < a.distance) return { index: i, distance: distance };
            return a;
        },
        { index: -1, distance: 99 },
    );
    const clef = [
        `<staffDef n="1" lines="5" clef.shape="G" clef.line="2" />`,
        `<staffDef n="1" lines="5" clef.shape="G" clef.line="2" clef.dis="8" clef.dis.place="below" />`,
        `<staffDef n="1" lines="5" clef.shape="F" clef.line="4" />`,
    ][clefIndex.index];

    let mei = `<?xml version="1.0" encoding="UTF-8"?>
<mei xmlns="http://www.music-encoding.org/ns/mei" meiversion="5.1">
  <meiHead>
    <fileDesc>
      <titleStmt>
        <title></title>
      </titleStmt>
      <pubStmt/>
    </fileDesc>
  </meiHead>
  <music>
    <body>
      <mdiv>
        <score>
          <scoreDef>
            <staffGrp symbol="bracket">
              ${clef}
            </staffGrp>
          </scoreDef>
          <section>
            <pb xml:id="jytxbtq" />
            <measure n="1">
              <staff n="1">
                <layer n="1">`;

    state.compound.forEach((p, i) => {
        if (i % 3 === 0) mei += `<beam>`;
        mei += `<note pname="${p.letter.toLowerCase()}" oct="${p.octave}" dur="8">`;
        if (p.accidental !== 0)
            mei += `<accid accid="${p.accidental === 1 ? "s" : "f"}" />`;
        mei += `</note>`;
        if (i % 3 === 2) mei += `</beam>`;
    });

    mei += `</layer>
              </staff>
            </measure>
          </section>
        </score>
      </mdiv>
    </body>
  </music>
</mei>`;
    state.verovio.setOptions({
        footer: "none",
        adjustPageWidth: true,
        adjustPageHeight: true,
        pageHeight: 120,
        scale: 50,
    });

    state.verovio.loadData(mei);
    const svg = state.verovio.renderToSVG(1);

    document.getElementById("compound")!.innerHTML = svg;
}
