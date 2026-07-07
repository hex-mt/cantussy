#include <emscripten/emscripten.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "cantus.h"

int BARS;
int MODE;
int cantus[MAX_BARS] = {0};
const int notes[] = {-7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
bool cantus_success = false;
bool initialised = false;

void initialise_env(void) {
    srand(time(NULL));
    initialised = true;
}

EMSCRIPTEN_KEEPALIVE
void set_bars(int length) { BARS = length; }

EMSCRIPTEN_KEEPALIVE
void set_mode(int mode) { MODE = mode; }

EMSCRIPTEN_KEEPALIVE
int generate_cantus(int mode, int length) {
    if (mode < 0 || mode > 5)
        return 1;
    MODE = mode;

    if (length < 3 || length > 16)
        return 1;
    BARS = length;

    if (!initialised) {
        initialise_env();
    }
    cantus[0] = cantus[BARS - 1] = 0;
    cantus[BARS - 2] = 1;

    try_note((CantusState){.top = 1, .bar = 1, .since_turn = 1});
    cantus_success = false;

    return 0;
}

void try_note(CantusState state) {
    if (cantus_success)
        return;
    if (cantus_complete(&state)) {
        if (climax_good(&state)) {
            cantus_success = true;
            // print_cantus();
            // return;
        }
        return;
    }

    // create shuffled array of notes to try
    int to_try[18];
    int range = create_range(&state, to_try);
    shuffle(to_try, range, &state);

    // recursively try out each note in range
    for (int i = 0; i < range; i++) {
        if (cantus_success)
            return;
        if (in_cadence(&state) && i > 0)
            return;

        int this_note = get_next_note(&state, to_try[i]);
        int prev_note = cantus[state.bar - 1];

        int this_motion = this_note - prev_note;

        int to_fill[CANTUS_FILL_LEN];
        bool must_fill = state.must_fill;

        if (registral_break(&state, &must_fill, to_fill, this_note, prev_note,
                            this_motion))
            continue;

        if (large_unrecovered_leap(&state, this_motion))
            continue;

        bool disconnected_climax = state.disconnected_climax;
        if (climax_disconnected(&state, this_motion)) {
            if (range == 10)
                continue;
            disconnected_climax = true;
        }

        if (repeated_note(&state, this_note))
            continue;

        if (dissonant_leap(this_motion))
            continue;

        if (larger_than_octave(this_motion))
            continue;

        if (tritone_between(prev_note, this_note))
            continue;

        int leaps_total = update_leaps_total(&state, this_motion);
        if (too_many_leaps(leaps_total))
            continue;

        int leaps_large = update_leaps_large(&state, this_motion);
        if (too_many_large_leaps(leaps_large))
            continue;

        if (steps_past_arpeggio(&state, this_motion))
            continue;

        if (arpeggio_past_step(&state, this_motion))
            continue;

        if (large_leap_past_step(&state, this_motion))
            continue;

        int leaps_in_row = update_leaps_in_row(&state, this_motion);
        if (too_many_leaps_in_row(leaps_in_row))
            continue;

        if (bad_consecutive_leaps(&state, this_motion))
            continue;

        bool repeated;
        if (new_climax(&state, this_note)) {
            if (bad_climax(this_note) && cannot_surpass(range))
                continue;
            repeated = false;
            disconnected_climax = false;
        } else if (repeat_climax(&state, this_note)) {
            if (cannot_surpass(range))
                continue;
            repeated = true;
        } else
            repeated = state.repeated_climax;

        int since_turn, new_turn = state.prev_turn;
        if (same_direction(&state, this_motion)) {
            if (should_change_direction(&state))
                continue;
            since_turn = state.since_turn + 1;
        } else {
            if (dissonant_outline(&state, prev_note))
                continue;
            since_turn = 1;
            new_turn = prev_note;
        }

        if (tritone_in_gesture(&state, since_turn, this_note))
            continue;

        if (noodling(&state, this_note))
            continue;

        if (overemphasised_tone(&state, this_note))
            continue;

        if (bad_cadence_approach(&state, this_note, this_motion, leaps_in_row))
            continue;

        // add the prospective note to the cantus
        if (!in_cadence(&state))
            cantus[state.bar] = this_note;
        else if (must_fill)
            continue;
        // construct a new state object and recursively try the next note.
        try_note((CantusState){.top = max(state.top, this_note),
                               .bottom = min(state.bottom, this_note),
                               .bar = state.bar + 1,
                               .repeated_climax = repeated,
                               .disconnected_climax = disconnected_climax,
                               .leaps_total = leaps_total,
                               .leaps_large = leaps_large,
                               .leaps_in_row = leaps_in_row,
                               .prev_motion = this_motion,
                               .since_turn = since_turn,
                               .prev_turn = new_turn,
                               .must_fill = must_fill,
                               .to_fill = must_fill ? to_fill : NULL});
    }
}

EMSCRIPTEN_KEEPALIVE
int get_cantus_value(int i) {
    if (i < 0 || i >= 16)
        return -1;
    return cantus[i];
}
