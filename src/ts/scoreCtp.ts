import { Interval, Pitch } from "meantonal";
import { state } from "./state.js";
import { audio } from "./audio.js";
import { solutionsLabel } from "./domRefs.js";

// Register boundaries used to decide whether the cantus/counterpoint pair
// needs to be transposed by an octave, and whether the lower voice needs an
// 8vb-below treble clef, to keep both voices within a comfortable printed
// range for a 2-staff (treble + treble-or-8vb) system.
const LOW_REGISTER_BOUNDARY = new Pitch(24, 9);
const HIGH_REGISTER_BOUNDARY = new Pitch(35, 14);

export async function drawCtp() {
    audio.stop();

    state.ctp = state.cantussy.generateCtp();
    solutionsLabel.innerHTML = `${state.cantussy.solutions}`;

    if (state.ctp[0].isEqual(new Pitch(0, 0))) {
        document.getElementById("ctp")!.innerHTML =
            `<div class="p-6 my-18 border-stone-700 dark:border-orange-100 border-2 text-stone-700 dark:text-orange-100 text-xl">No solutions found :(</div>`;
        return;
    }

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
          <scoreDef>`;

    const cantusOnTop =
        Pitch.highest(state.cantus).stepsTo(Pitch.highest(state.ctp)) < 0;

    state.upperVoice = cantusOnTop ? state.cantus.slice() : state.ctp.slice();
    state.lowerVoice = !cantusOnTop ? state.cantus.slice() : state.ctp.slice();

    if (Pitch.lowest(state.upperVoice).stepsTo(LOW_REGISTER_BOUNDARY) > 0) {
        state.upperVoice = state.upperVoice.map((p) =>
            p.transposeReal(new Interval(5, 2)),
        );
        state.lowerVoice = state.lowerVoice.map((p) =>
            p.transposeReal(new Interval(5, 2)),
        );
    }

    if (Pitch.highest(state.upperVoice).stepsTo(HIGH_REGISTER_BOUNDARY) < 0) {
        state.upperVoice = state.upperVoice.map((p) =>
            p.transposeReal(new Interval(-5, -2)),
        );
        state.lowerVoice = state.lowerVoice.map((p) =>
            p.transposeReal(new Interval(-5, -2)),
        );
    }

    let lowerClef;

    if (Pitch.lowest(state.lowerVoice).stepsTo(LOW_REGISTER_BOUNDARY) > 0)
        lowerClef = `<staffDef n="2" lines="5" clef.shape="G" clef.line="2" clef.dis="8" clef.dis.place="below" />`;
    else lowerClef = `<staffDef n="2" lines="5" clef.shape="G" clef.line="2" />`;

    mei += `<staffGrp symbol="bracket">
              <staffDef n="1" lines="5" clef.shape="G" clef.line="2" />
              ${lowerClef}
            </staffGrp>
          </scoreDef>
          <section>
            <pb xml:id="jytxbtq" />
            <measure n="1">
              <staff n="1">
                <layer n="1">`;

    state.upperVoice.forEach((p) => {
        mei += `<note pname="${p.letter.toLowerCase()}" oct="${p.octave}" dur="1">`;
        if (p.accidental !== 0)
            mei += `<accid accid="${p.accidental === 1 ? "s" : "f"}" />`;
        mei += `</note>`;
    });

    mei += `</layer>
              </staff>
              <staff n="2">
                <layer n="1">`;

    state.lowerVoice.forEach((p) => {
        mei += `<note pname="${p.letter.toLowerCase()}" oct="${p.octave}" dur="1">`;
        if (p.accidental !== 0)
            mei += `<accid accid="${p.accidental === 1 ? "s" : "f"}" />`;
        mei += `</note>`;
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

    document.getElementById("ctp")!.innerHTML = svg;
}
