# Cantus Firmus Generator

This program generates a cantus firmus roughly in line with the restrictions given in Salzer and Schachter's _Counterpoint in Composition_.

The melody is generated non-deterministically via randomised recursive exploration of a tree, where each node represents a new note concatenated onto the current melody. Any time a note is tried, various checks are performed, and if no valid cantus can result from continuing down a given branch, backtracking occurs.

This method tries to achieve three objectives:

1. Generation time for a cantus with length between 9 and 16 should be as low as possible.
2. Any valid cantus by the ruleset given should be within the range of the function and possible to generate.
3. The chances of any two canti in the range of the function being generated should be as close to even as possible.

To the extent that these objectives are achieved, it should hopefully be useful for stress-testing other species counterpoint related programs.

The error checks are also parameterised in a way that should make them easy to selectively disable and add inverse checks for to generate canti that are deliberately flawed in a specified way, which might be useful pedagogically.

## Basic Constraints

- [x] Contained within the range of a tenth.
- [x] Only melodic consonances occur.
- [x] One tone should not be over-emphasised.
- [x] Climax sould not be repeated.
- [x] Stepwise motion should predominate.
- [x] Between 2 to 4 leaps.
- [x] Must change direction several times.
- [x] No more than 2 leaps larger than a 4th.
- [x] Leaps larger than a 3rd should change direction, preferably stepwise.
- [x] No more than 2 leaps in a row.
- [x] Direction change should preferably be by step.
- [x] Avoid excessive motion in a single direction.
- [x] Avoid outlining dissonances between changes in direction.
- [x] Avoid climaxing on the leading tone.
- [x] No immediate repetition of tones.
- [ ] Caution in repetition of tones preceded by large leaps.
- [ ] Avoid repetition of groups of tones.
- [ ] Avoid sequences.

## Emergent Biases

- [x] Leaps should be equally likely to occur at any point in a generated cantus.
- [x] Any acceptable range should be equally likely to be spanned. Shouldn't be biased towards narrow or wide canti.

## Building

The generation engine lives in `src/c/*` and is compiled to WebAssembly via [Emscripten](https://emscripten.org/); the output (`src/cantus.js`, `src/cantus.wasm`) is committed to this repo so the frontend build (`npm run build`, `npm run dev`) doesn't itself require the Emscripten SDK.

Whenever `src/c/*.c`/`*.h` changes, rebuild and commit the regenerated output:

```
npm run build:wasm
```

This requires the Emscripten SDK (`emcc`) to be installed and activated on your `PATH`.
