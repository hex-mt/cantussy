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

const RuleParams RULE_DEFAULTS = {
    .registral_break = true,
    .large_unrecovered_leap = true,
    .climax_disconnected = true,
    .repeated_note = true,
    .dissonant_leap = true,
    .max_leap_size = 7,
    .tritone_melodic = true,
    .leaps_divisor = 4,
    .max_large_leaps = 2,
    .steps_past_arpeggio = true,
    .arpeggio_past_step = true,
    .large_leap_past_step = true,
    .max_leaps_in_row = 2,
    .bad_consecutive_leaps = true,
    .max_range = 10,
    .bad_climax = true,
    .max_same_direction = 3,
    .dissonant_outline = true,
    .tritone_in_gesture = true,
    .noodling = true,
    .max_tone_repetition = 2,
    .bad_cadence_approach = true,
    .step_bias_correction = true,
};
RuleParams rules = RULE_DEFAULTS;

void initialise_env(void) {
    srand(time(NULL));
    initialised = true;
}

EMSCRIPTEN_KEEPALIVE
void set_bars(int length) { BARS = length; }

EMSCRIPTEN_KEEPALIVE
void set_mode(int mode) { MODE = mode; }

EMSCRIPTEN_KEEPALIVE
void set_rule_param(int rule_id, int value) {
    switch (rule_id) {
    case RULE_REGISTRAL_BREAK: rules.registral_break = value; break;
    case RULE_LARGE_UNRECOVERED_LEAP: rules.large_unrecovered_leap = value; break;
    case RULE_CLIMAX_DISCONNECTED: rules.climax_disconnected = value; break;
    case RULE_REPEATED_NOTE: rules.repeated_note = value; break;
    case RULE_DISSONANT_LEAP: rules.dissonant_leap = value; break;
    case RULE_MAX_LEAP_SIZE: rules.max_leap_size = value; break;
    case RULE_TRITONE_MELODIC: rules.tritone_melodic = value; break;
    case RULE_LEAPS_DIVISOR: rules.leaps_divisor = value; break;
    case RULE_MAX_LARGE_LEAPS: rules.max_large_leaps = value; break;
    case RULE_STEPS_PAST_ARPEGGIO: rules.steps_past_arpeggio = value; break;
    case RULE_ARPEGGIO_PAST_STEP: rules.arpeggio_past_step = value; break;
    case RULE_LARGE_LEAP_PAST_STEP: rules.large_leap_past_step = value; break;
    case RULE_MAX_LEAPS_IN_ROW: rules.max_leaps_in_row = value; break;
    case RULE_BAD_CONSECUTIVE_LEAPS: rules.bad_consecutive_leaps = value; break;
    case RULE_MAX_RANGE: rules.max_range = value; break;
    case RULE_BAD_CLIMAX: rules.bad_climax = value; break;
    case RULE_MAX_SAME_DIRECTION: rules.max_same_direction = value; break;
    case RULE_DISSONANT_OUTLINE: rules.dissonant_outline = value; break;
    case RULE_TRITONE_IN_GESTURE: rules.tritone_in_gesture = value; break;
    case RULE_NOODLING: rules.noodling = value; break;
    case RULE_MAX_TONE_REPETITION: rules.max_tone_repetition = value; break;
    case RULE_BAD_CADENCE_APPROACH: rules.bad_cadence_approach = value; break;
    case RULE_STEP_BIAS_CORRECTION: rules.step_bias_correction = value; break;
    }
}

EMSCRIPTEN_KEEPALIVE
int get_rule_param(int rule_id) {
    switch (rule_id) {
    case RULE_REGISTRAL_BREAK: return rules.registral_break;
    case RULE_LARGE_UNRECOVERED_LEAP: return rules.large_unrecovered_leap;
    case RULE_CLIMAX_DISCONNECTED: return rules.climax_disconnected;
    case RULE_REPEATED_NOTE: return rules.repeated_note;
    case RULE_DISSONANT_LEAP: return rules.dissonant_leap;
    case RULE_MAX_LEAP_SIZE: return rules.max_leap_size;
    case RULE_TRITONE_MELODIC: return rules.tritone_melodic;
    case RULE_LEAPS_DIVISOR: return rules.leaps_divisor;
    case RULE_MAX_LARGE_LEAPS: return rules.max_large_leaps;
    case RULE_STEPS_PAST_ARPEGGIO: return rules.steps_past_arpeggio;
    case RULE_ARPEGGIO_PAST_STEP: return rules.arpeggio_past_step;
    case RULE_LARGE_LEAP_PAST_STEP: return rules.large_leap_past_step;
    case RULE_MAX_LEAPS_IN_ROW: return rules.max_leaps_in_row;
    case RULE_BAD_CONSECUTIVE_LEAPS: return rules.bad_consecutive_leaps;
    case RULE_MAX_RANGE: return rules.max_range;
    case RULE_BAD_CLIMAX: return rules.bad_climax;
    case RULE_MAX_SAME_DIRECTION: return rules.max_same_direction;
    case RULE_DISSONANT_OUTLINE: return rules.dissonant_outline;
    case RULE_TRITONE_IN_GESTURE: return rules.tritone_in_gesture;
    case RULE_NOODLING: return rules.noodling;
    case RULE_MAX_TONE_REPETITION: return rules.max_tone_repetition;
    case RULE_BAD_CADENCE_APPROACH: return rules.bad_cadence_approach;
    case RULE_STEP_BIAS_CORRECTION: return rules.step_bias_correction;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void reset_rule_params(void) { rules = RULE_DEFAULTS; }

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

        if (leap_too_large(this_motion))
            continue;

        if (tritone_melodic(prev_note, this_note))
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
