#define MEANTONAL
#include "ctp.h"
#include "cantus.h"
#include "chunks.h"
#include "meantonal.h"
#include <emscripten/emscripten.h>
#include <stdio.h>

Interval firstInts[] = {(Interval){5, 2}, (Interval){3, 1}, (Interval){0, 0},
                        (Interval){-5, -2}};
extern int cantus[MAX_BARS];
Pitch mt_cantus[MAX_BARS] = {0};
Interval cantus_motions[MAX_BARS] = {0};
Pitch mt_ctp[MAX_BARS] = {0};
Interval ctp_motions[MAX_BARS] = {0};
Interval v_ints[MAX_BARS] = {0};
TonalContext context;
Pitch tonic;
TuningMap T;

Pitch result_ctp[MAX_BARS] = {0};
int solutions = 0;

EMSCRIPTEN_KEEPALIVE
void set_cantus(int index, int w, int h) { mt_cantus[index] = (Pitch){w, h}; }

EMSCRIPTEN_KEEPALIVE
Pitch *get_ctp() { return result_ctp; }

EMSCRIPTEN_KEEPALIVE
int get_solutions() { return solutions; }

EMSCRIPTEN_KEEPALIVE
void generate_ctp(bool from_string) {
    solutions = 0;
    for (int i = 0; i < BARS; i++) {
        result_ctp[i] = (Pitch){0, 0};
        mt_ctp[i] = (Pitch){0, 0};
        v_ints[i] = (Interval){0, 0};
    }
    generate_chunks();

    T = tuning_map_from_edo(12, (Pitch){29, 11}, 440);

    // cantus data init
    context = context_from_chroma(MODE - 1, MODE);
    tonic = pitch_from_chroma(MODE - 1, 4);

    if (!from_string)
        for (int i = 0; i < BARS; i++) {
            mt_cantus[i] = transpose_diatonic(tonic, cantus[i], context);
        }

    for (int i = 0; i + 1 < BARS; i++) {
        cantus_motions[i] = interval_between(mt_cantus[i], mt_cantus[i + 1]);
    }

    // shuffle the firstInts array
    for (int i = 0; i < 4; i++) {
        int s = rand() % (4 - i) + i;
        Interval temp = firstInts[i];
        firstInts[i] = firstInts[s];
        firstInts[s] = temp;
    }

    for (int i = 0; i < 4; i++) {
        v_ints[0] = firstInts[i];
        mt_ctp[0] = transpose_real(mt_cantus[0], v_ints[0]);
        next_chunk((CtpState){
            .bar = 1,
            .top = mt_ctp[0],
            .bottom = mt_ctp[0],
        });
    }
}

void next_chunk(CtpState state) {
    chunk_node *chunks =
        get_chunks(v_ints[state.bar - 1], cantus_motions[state.bar - 1]);
    if (!chunks) {
        return;
    }

    chunk_node buf[MAX_CHUNKS];
    chunk_node *cloned_chunks = clone_list_into(chunks, buf);

    bool first_chunk = true;
    for (chunk_node *cur = cloned_chunks; cur != NULL; cur = cur->next) {
        Interval this_int = cur->data.cons;

        Pitch this_note;
        Pitch prev_note = mt_ctp[state.bar - 1];
        if (bars_remaining(&state) == 0)
            this_note = transpose_diatonic(mt_ctp[state.bar - 1], 1, context);
        else
            this_note = transpose_real(mt_cantus[state.bar], this_int);

        Interval this_motion = interval_between(prev_note, this_note);

        Pitch top = pitch_highest((Pitch[]){this_note, state.top}, 2, T);
        Pitch bottom = pitch_lowest((Pitch[]){this_note, state.bottom}, 2, T);
        int range = stepspan(interval_between(bottom, top));

        if (range > 9)
            continue;

        int to_fill[CTP_FILL_LEN];
        bool must_fill = state.must_fill;

        if (ctp_registral_break(&state, &must_fill, to_fill, this_note,
                                prev_note, this_motion))
            continue;

        if (ctp_large_unrecovered_leap(&state, this_motion))
            continue;

        if (consecutive_ties(&state, this_note, prev_note))
            continue;

        int ties = update_ties(&state, this_motion);
        if (too_many_ties(ties))
            continue;

        int imps_in_row = update_imps(&state, this_int);
        if (too_many_imps_in_row(imps_in_row))
            continue;

        int leaps_total = ctp_update_leaps_total(&state, this_motion);
        if (too_many_leaps(leaps_total))
            continue;

        int leaps_large = ctp_update_leaps_large(&state, this_motion);
        if (too_many_large_leaps(leaps_large))
            continue;

        if (chromatic_outside_cadence(&state, this_note))
            continue;

        bool disconnected_climax =
            update_disconnected_climax(&state, this_motion);
        if (disconnected_climax && ctp_cannot_surpass(range))
            continue;

        bool repeated;
        if (ctp_new_climax(&state, this_note)) {
            if (is_leading_tone(this_note) && ctp_cannot_surpass(range))
                continue;
            repeated = false;
            disconnected_climax = false;
        } else if (ctp_repeat_climax(&state, this_note)) {
            if (ctp_cannot_surpass(range))
                continue;
            repeated = true;
        } else
            repeated = state.repeated_climax;

        int since_turn;
        Pitch new_turn = state.prev_turn;
        if (ctp_same_direction(&state, this_motion)) {
            if (ctp_should_change_direction(&state))
                continue;
            since_turn = state.since_turn + 1;
        } else {
            if (ctp_dissonant_outline(&state, prev_note))
                continue;
            since_turn = 1;
            new_turn = prev_note;
        }

        if (ctp_has_tritone(&state, since_turn, this_note))
            continue;

        if (ctp_noodling(&state, this_note))
            continue;

        if (ctp_overemphasised_tone(&state, this_note))
            continue;

        if (bars_remaining(&state) == 1 && ctp_bad_penultima(&state, this_note))
            continue;
        ;

        if (bars_remaining(&state) == 0) {
            if (!first_chunk)
                return;
            first_chunk = false;
            if (ctp_climax_good(&state)) {
                solutions++;
                for (int i = 0; i < state.bar; i++) {
                    result_ctp[i] = mt_ctp[i];
                }
                result_ctp[state.bar] = this_note;
            }
            return;
        }

        v_ints[state.bar] = this_int;
        mt_ctp[state.bar] = this_note;
        ctp_motions[state.bar] = this_motion;

        next_chunk((CtpState){
            .bar = state.bar + 1,
            .top = top,
            .bottom = bottom,
            .leaps_total = leaps_total,
            .leaps_large = leaps_large,
            .ties = ties,
            .imps_in_row = imps_in_row,
            .repeated_climax = repeated,
            .disconnected_climax = disconnected_climax,
            .since_turn = since_turn,
            .prev_turn = new_turn,
        });
    }
}
