#ifndef CANTUS_H
#define CANTUS_H

#include <stdlib.h>
#include <string.h>

#include "meantonal.h"
#include "rules.h"

#define max(x, y) (((x) >= (y)) ? (x) : (y))
#define min(x, y) (((x) <= (y)) ? (x) : (y))

// Longest cantus firmus / counterpoint the C engine and its WASM bindings
// support; sizes every per-bar array in cantus.c/h and ctp.c/h.
#define MAX_BARS 32

extern int cantus[MAX_BARS];
extern const int notes[17];

extern int BARS;
extern int MODE;
#define FA ((3 * MODE) % 7)
#define MI ((3 + 3 * MODE) % 7)

typedef struct {
    int top;
    int bottom;
    int bar;
    bool repeated_climax;
    bool disconnected_climax;
    int leaps_total; // leaps larger than a 2nd
    int leaps_large; // leaps larger than a 4th
    int leaps_in_row;
    int prev_motion;
    int prev_turn;
    int since_turn;
    bool must_fill;
    int *to_fill;
} CantusState;

void initialise_env(void);
void set_bars(int length);
void set_mode(int mode);
int generate_cantus(int mode, int length);

void try_note(CantusState range);

int get_cantus_value(int i);

static inline bool cantus_complete(CantusState *state) {
    return state->bar == BARS - 1;
}

static inline bool bad_climax(int climax) {
    if (!rules.bad_climax)
        return false;
    return climax == 6 && (MODE == MAJOR || MODE == LYDIAN);
}

static inline bool climax_good(CantusState *state) {
    return !state->repeated_climax && !bad_climax(state->top) &&
           !state->disconnected_climax;
}

static inline int create_range(const CantusState *s, int *out) {
    int top = s->bottom + (rules.max_range - 1);
    if (bad_climax(top))
        top--;
    int bottom = s->top - (rules.max_range - 1);
    int range = top + 1 - bottom;
    int start = bottom + 7;
    if (start < 0)
        start = 0;
    if (start + range > (int)(sizeof notes / sizeof notes[0]))
        range = (sizeof notes / sizeof notes[0]) - start;

    memcpy(out, notes + start, sizeof(out[0]) * range);

    return range;
}

static inline void shuffle(int a[], int length, CantusState *state) {
    for (int i = 0; i < length; i++) {
        int s = rand() % (length - i) + i;
        int temp = a[i];
        a[i] = a[s];
        a[s] = temp;
    }
    // counteract bias towards "early spending" of leaps
    // by strongly preferencing steps as first choices
    if (!rules.step_bias_correction)
        return;
    int s = rand() % 5;
    if (s > 1) {
        for (int i = 0; i < length; i++) {
            if (i == 0 && abs(a[i] - cantus[state->bar - 1]) == 1)
                break;
            if (abs(a[i] - cantus[state->bar - 1]) == 1) {
                int temp = a[0];
                a[0] = a[i];
                a[i] = temp;
                break;
            }
        }
        for (int i = 1; i < length; i++) {
            if (i == 1 && abs(a[i] - cantus[state->bar - 1]) == 1)
                break;
            if (abs(a[i] - cantus[state->bar - 1]) == 1) {
                int temp = a[1];
                a[1] = a[i];
                a[i] = temp;
                break;
            }
        }
    }
}

static inline bool in_cadence(CantusState *state) {
    return state->bar >= BARS - 2;
}

static inline int get_next_note(CantusState *state, int to_try) {
    if (in_cadence(state))
        return cantus[state->bar];
    else {
        return to_try;
    }
}

static inline bool same_sign(int x, int y) {
    return ((x > 0) ^ (y < 0)) || (x == 0 || y == 0);
}

// Size of the cantus firmus's registral "fill" tracking array (to_fill) and
// the offset added to a scale-degree note value to index into it. ctp.h's
// counterpoint-side equivalents (CTP_FILL_LEN/CTP_FILL_OFFSET) size the same
// concept 2 wider, since counterpoint intervals span a wider range than
// cantus firmus scale degrees.
#define CANTUS_FILL_LEN 18
#define CANTUS_FILL_OFFSET 7

static inline int get_lower_boundary(int to_fill[CANTUS_FILL_LEN]) {
    for (int i = 0; i < CANTUS_FILL_LEN; i++) {
        if (to_fill[i] != -1)
            return i - CANTUS_FILL_OFFSET;
    }
    return 0;
}

static inline int get_upper_boundary(int to_fill[CANTUS_FILL_LEN]) {
    for (int i = CANTUS_FILL_LEN - 1; i >= 0; i--) {
        if (to_fill[i] != -1)
            return i - CANTUS_FILL_OFFSET;
    }
    return 0;
}

static inline bool registral_break(CantusState *state, bool *must_fill,
                                   int to_fill[CANTUS_FILL_LEN], int this_note,
                                   int prev_note, int this_motion) {
    if (!rules.registral_break)
        return false;
    if (*must_fill) {
        int lower = get_lower_boundary(state->to_fill);
        int upper = get_upper_boundary(state->to_fill);
        if ((this_note == lower || this_note == upper) &&
            (prev_note == lower || prev_note == upper))
            return true;
        for (int i = 0; i < CANTUS_FILL_LEN; i++) {
            to_fill[i] = state->to_fill[i];
        }
        if (state->to_fill[this_note + CANTUS_FILL_OFFSET] == -1) {
            for (int i = 1; i < CANTUS_FILL_LEN - 1; i++) {
                if (state->to_fill[i] == 0)
                    return true;
            }
            *must_fill = false;
        } else
            to_fill[this_note + CANTUS_FILL_OFFSET]++;
    } else if (abs(this_motion) > 3) {
        for (int i = 0; i < CANTUS_FILL_LEN; i++)
            to_fill[i] = -1; // sentinel value

        int low_bound = min(this_note, prev_note) + CANTUS_FILL_OFFSET;
        int high_bound = max(this_note, prev_note) + CANTUS_FILL_OFFSET;
        for (int i = low_bound; i <= high_bound; i++)
            to_fill[i] = 0;
        to_fill[low_bound] = to_fill[high_bound] = 1;
        *must_fill = true;
    }
    return false;
}

static inline bool large_unrecovered_leap(CantusState *state, int this_motion) {
    if (!rules.large_unrecovered_leap)
        return false;
    return abs(state->prev_motion) > 3 &&
           (same_sign(state->prev_motion, this_motion) || abs(this_motion) > 3);
}

static inline bool repeated_note(CantusState *state, int this_note) {
    if (!rules.repeated_note)
        return false;
    return cantus[state->bar - 1] == this_note;
}

static inline bool climax_disconnected(CantusState *state, int this_motion) {
    if (!rules.climax_disconnected)
        return false;
    return cantus[state->bar - 1] == state->top &&
           abs(state->prev_motion) > 1 && abs(this_motion) > 1;
}

static inline bool dissonant_leap(int this_motion) {
    if (!rules.dissonant_leap)
        return false;
    return abs(this_motion) == 6;
}

static inline bool leap_too_large(int this_motion) {
    return abs(this_motion) > rules.max_leap_size;
}

static inline bool tritone_between(int p, int q) {
    return ((p == MI || p == MI - 7) && (q == FA || q == FA - 7)) ||
           ((p == FA || p == FA - 7) && (q == MI || q == MI - 7));
}

static inline bool tritone_melodic(int prev_note, int this_note) {
    if (!rules.tritone_melodic)
        return false;
    return tritone_between(prev_note, this_note);
}

static inline int update_leaps_total(CantusState *state, int this_motion) {
    return abs(this_motion) > 1 ? state->leaps_total + 1 : state->leaps_total;
}

static inline bool too_many_leaps(int leaps_total) {
    return (double)leaps_total > ((double)(BARS) / rules.leaps_divisor);
}

static inline int update_leaps_large(CantusState *state, int this_motion) {
    return abs(this_motion) > 3 ? state->leaps_large + 1 : state->leaps_large;
}

static inline bool too_many_large_leaps(int leaps_large) {
    return leaps_large > rules.max_large_leaps;
}

static inline bool steps_past_arpeggio(CantusState *state, int this_motion) {
    if (!rules.steps_past_arpeggio)
        return false;
    return state->leaps_in_row == 2 && state->since_turn > 1 &&
           same_sign(state->prev_motion, this_motion);
}

static inline bool arpeggio_past_step(CantusState *state, int this_motion) {
    if (!rules.arpeggio_past_step)
        return false;
    return state->leaps_in_row == 1 && state->since_turn > 1 &&
           same_sign(state->prev_motion, this_motion) && abs(this_motion) > 1;
}

static inline bool large_leap_past_step(CantusState *state, int this_motion) {
    if (!rules.large_leap_past_step)
        return false;
    return (abs(this_motion) > 3 && same_sign(state->prev_motion, this_motion));
}

static inline int update_leaps_in_row(CantusState *state, int this_motion) {
    return abs(this_motion) > 1 ? state->leaps_in_row + 1 : 0;
}

static inline bool too_many_leaps_in_row(int leaps_in_row) {
    return leaps_in_row > rules.max_leaps_in_row;
}

static inline bool bad_consecutive_leaps(CantusState *state, int this_motion) {
    if (!rules.bad_consecutive_leaps)
        return false;
    if ((state->prev_motion == 2) && (this_motion > 3))
        return true;
    if ((state->prev_motion == 3) && (abs(this_motion) > 2))
        return true;

    if ((state->prev_motion == -2) && (this_motion < -3))
        return true;
    if ((state->prev_motion == -3) && (abs(this_motion) > 2))
        return true;

    return false;
}

static inline bool new_climax(CantusState *state, int this_note) {
    return this_note > state->top;
}

static inline bool cannot_surpass(int range) { return range == rules.max_range; }

static inline bool repeat_climax(CantusState *state, int this_note) {
    return this_note == state->top;
}

static inline bool same_direction(CantusState *state, int this_motion) {
    return same_sign(state->prev_motion, this_motion);
}

static inline bool should_change_direction(CantusState *state) {
    return state->since_turn == rules.max_same_direction;
}

static inline bool dissonant_outline(CantusState *state, int prev_note) {
    if (!rules.dissonant_outline)
        return false;
    int outline = abs(state->prev_turn - prev_note);
    if (outline == 6 || outline == 8)
        return true;
    if (tritone_between(state->prev_turn, prev_note))
        return true;
    return false;
}

static inline bool tritone_in_gesture(CantusState *state, int since_turn,
                                      int this_note) {
    if (!rules.tritone_in_gesture)
        return false;
    if (since_turn > 1) {
        if (tritone_between(this_note, cantus[state->bar - 2])) {
            return true;
        }
    } else if (since_turn > 2) {
        if (tritone_between(this_note, cantus[state->bar - 3])) {
            for (int i = 0; i < 3; i++) {
                if (abs(cantus[state->bar - i] - cantus[state->bar - i - 1]) >
                    1)
                    return true;
            }
        }
    }
    return false;
}

static inline bool noodling(CantusState *state, int this_note) {
    if (!rules.noodling)
        return false;
    if ((state->bar >= 3 && this_note == cantus[state->bar - 2]) &&
        (cantus[state->bar - 1] == cantus[state->bar - 3]))
        return true;
    if (state->bar >= 4 && this_note == cantus[state->bar - 2] &&
        this_note == cantus[state->bar - 4])
        return true;
    return false;
}

static inline bool overemphasised_tone(CantusState *state, int this_note) {
    int count = 0;
    if (this_note == 0)
        count++;
    if (this_note == 1 && state->bar != BARS - 2)
        count++;
    for (int i = 0; i < state->bar; i++)
        if (cantus[i] == this_note)
            count++;
    if (count > rules.max_tone_repetition)
        return true;
    return false;
}

static inline bool bad_cadence_approach(CantusState *state, int this_note,
                                        int this_motion, int leaps_in_row) {
    if (!rules.bad_cadence_approach)
        return false;
    if (state->bar == BARS - 2 && (this_motion < -3 || leaps_in_row == 2))
        return true;
    if (MODE == LYDIAN && state->bar == BARS - 3 && this_note == 3)
        return true;
    return false;
}

#endif
