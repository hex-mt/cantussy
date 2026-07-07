#ifndef CTP_H
#define CTP_H

#include "cantus.h"
#define IMP_LIMIT 3
#define TIE_LIMIT (BARS - 3) / 4

typedef struct {
    Pitch top;
    Pitch bottom;
    int bar;
    bool repeated_climax;
    bool disconnected_climax;
    int ties;
    int imps_in_row;
    int leaps_total; // leaps larger than a 2nd
    int leaps_large; // leaps larger than a 4th
    int leaps_in_row;
    Pitch prev_turn;
    int since_turn;
    bool must_fill;
    int *to_fill;
} CtpState;

extern Pitch mt_cantus[MAX_BARS];
extern Interval cantus_motions[MAX_BARS];
extern Pitch mt_ctp[MAX_BARS];
extern Interval ctp_motions[MAX_BARS];
extern Interval v_ints[MAX_BARS];
extern TonalContext context;
extern Pitch tonic;

extern Pitch result_ctp[MAX_BARS];
extern int solutions;

void next_chunk(CtpState state);

static inline int bars_remaining(CtpState *state) {
    return BARS - state->bar - 1;
}

static inline int pitch_index(Pitch p) {
    return stepspan(interval_between(mt_ctp[0], p));
}

// Counterpoint-side equivalents of CANTUS_FILL_LEN/CANTUS_FILL_OFFSET
// (cantus.h): sized 2 wider since a counterpoint interval's stepspan ranges
// 2 further in each direction than a cantus firmus scale degree does.
#define CTP_FILL_LEN 19
#define CTP_FILL_OFFSET 9

static inline int ctp_get_lower_boundary(int to_fill[CTP_FILL_LEN]) {
    for (int i = 0; i < CTP_FILL_LEN; i++) {
        if (to_fill[i] != -1)
            return i - CTP_FILL_OFFSET;
    }
    return 0;
}

static inline int ctp_get_upper_boundary(int to_fill[CTP_FILL_LEN]) {
    for (int i = CTP_FILL_LEN - 1; i >= 0; i--) {
        if (to_fill[i] != -1)
            return i - CTP_FILL_OFFSET;
    }
    return 0;
}

// Audit (2026-07-06): mirrors registral_break in cantus.h. Confirmed
// logic-equivalent (same must-fill state machine and fill-array
// bookkeeping, modulo int-scale-degree vs Pitch/stepspan representation).
// This pair previously had a real bug: it called cantus.h's boundary helpers
// (sized for CANTUS_FILL_LEN==18) on this file's CTP_FILL_LEN==19 array, so
// the highest registral boundary was never detected. Fixed by giving this
// file its own ctp_get_lower_boundary/ctp_get_upper_boundary sized for
// CTP_FILL_LEN. Kept separate (not merged) pending test coverage.
static inline bool ctp_registral_break(CtpState *state, bool *must_fill,
                                       int to_fill[CTP_FILL_LEN], Pitch this_note,
                                       Pitch prev_note, Interval this_motion) {
    if (*must_fill) {
        int lower = ctp_get_lower_boundary(state->to_fill);
        int upper = ctp_get_upper_boundary(state->to_fill);
        if ((pitch_index(this_note) == lower ||
             pitch_index(this_note) == upper) &&
            (pitch_index(prev_note) == lower ||
             pitch_index(prev_note) == upper))
            return true;
        for (int i = 0; i < CTP_FILL_LEN; i++) {
            to_fill[i] = state->to_fill[i];
        }
        if (state->to_fill[pitch_index(this_note) + CTP_FILL_OFFSET] == -1) {
            for (int i = 1; i < CTP_FILL_LEN - 1; i++) {
                if (state->to_fill[i] == 0)
                    return true;
            }
            *must_fill = false;
        } else
            to_fill[pitch_index(this_note) + CTP_FILL_OFFSET]++;
    } else if (abs(stepspan(this_motion)) > 3) {
        for (int i = 0; i < CTP_FILL_LEN; i++)
            to_fill[i] = -1; // sentinel value

        int low_bound =
            min(pitch_index(this_note), pitch_index(prev_note)) + CTP_FILL_OFFSET;
        int high_bound =
            max(pitch_index(this_note), pitch_index(prev_note)) + CTP_FILL_OFFSET;
        for (int i = low_bound; i <= high_bound; i++)
            to_fill[i] = 0;
        to_fill[low_bound] = to_fill[high_bound] = 1;
        *must_fill = true;
    }
    return false;
}

// Audit (2026-07-06): mirrors large_unrecovered_leap in cantus.h. Confirmed
// logic-equivalent modulo representation — this reads the previous motion
// from the ctp_motions[] global instead of a CantusState-style state field,
// but the comparison logic is otherwise identical. Kept separate pending
// test coverage.
static inline bool ctp_large_unrecovered_leap(CtpState *state,
                                              Interval this_motion) {
    Interval prev_motion = ctp_motions[state->bar - 1];
    return abs(stepspan(prev_motion)) > 3 &&
           (same_sign(stepspan(prev_motion), stepspan(this_motion)) ||
            abs(stepspan(this_motion)) > 3);
}

static inline bool consecutive_ties(CtpState *state, Pitch this_note,
                                    Pitch prev_note) {
    if (pitches_equal(this_note, prev_note) &&
        pitches_equal(prev_note, mt_ctp[state->bar - 2]))
        return true;
    return false;
}

static inline int update_ties(CtpState *state, Interval this_motion) {
    return stepspan(this_motion) == 0 ? state->ties + 1 : state->ties;
}

static inline bool too_many_ties(int ties) { return ties > TIE_LIMIT; }

static inline int update_imps(CtpState *state, Interval this_int) {
    int abs_this_int = abs(stepspan(this_int));
    bool is_imp = abs_this_int == 2 || abs_this_int == 5 || abs_this_int == 9;

    if (!is_imp)
        return 0;

    int abs_prev_int = abs(stepspan(v_ints[state->bar - 1]));
    bool same_int = abs_this_int == abs_prev_int;

    if (!same_int)
        return 1;

    return state->imps_in_row + 1;
}

static inline bool too_many_imps_in_row(int imps) { return imps > IMP_LIMIT; }

static inline int ctp_update_leaps_total(CtpState *state,
                                         Interval this_motion) {
    return abs(stepspan(this_motion)) > 1 ? state->leaps_total + 1
                                          : state->leaps_total;
}

static inline int ctp_update_leaps_large(CtpState *state,
                                         Interval this_motion) {
    return abs(stepspan(this_motion)) > 3 ? state->leaps_large + 1
                                          : state->leaps_large;
}

static inline bool chromatic_outside_cadence(CtpState *state, Pitch this_note) {
    int alteration = degree_alteration(this_note, context);
    int degree = degree_number(this_note, context);

    if (bars_remaining(state) > 2 && alteration)
        return true;

    if (bars_remaining(state) == 2 && alteration == -1 ||
        (alteration == 1 && ((MODE != 2 && MODE != 3 && MODE != 4) ||
                             (degree != 5 && degree != 6))))
        return true;
    return false;
}

static inline bool update_disconnected_climax(CtpState *state,
                                              Interval this_motion) {
    Interval prev_motion = ctp_motions[state->bar - 1];
    return state->disconnected_climax ||
           (pitches_equal(mt_ctp[state->bar - 1], state->top) &&
            abs(stepspan(prev_motion)) > 1 && abs(stepspan(this_motion)) > 1);
}

// Audit (2026-07-06): mirrors new_climax in cantus.h. Confirmed
// logic-equivalent (both test "is this note higher than the current
// climax", just via direct int comparison vs. interval stepspan sign).
static inline bool ctp_new_climax(CtpState *state, Pitch this_note) {
    return stepspan(interval_between(state->top, this_note)) > 0;
}

// Audit (2026-07-06): mirrors cannot_surpass in cantus.h (range==10 there vs
// range==9 here). Confirmed equivalent despite the different literal: the
// `range` each receives is computed differently at its call site.
// cantus.c's create_range() returns a *count* of candidate next-note values
// still consistent with staying within a 10th (so range==10 means the count
// of candidates has narrowed to exactly 10, i.e. the 10th is maximally
// used); ctp.c passes stepspan(interval_between(bottom, top)) directly, a
// *step-span* rather than a count (so range==9 directly means a 9-step,
// i.e. 10th, span). A count of 10 corresponds to a span of 9, so both
// fire at the same underlying condition. Kept separate since the two range
// computations are genuinely different (count-of-candidates vs
// direct-span) and not worth entangling just to share this one-line check.
static inline bool ctp_cannot_surpass(int range) { return range == 9; }

// Audit (2026-07-06): mirrors repeat_climax in cantus.h. Confirmed
// logic-equivalent (pitch equality test, just expressed via stepspan of the
// pitch cast to an Interval-from-origin rather than direct int equality).
static inline bool ctp_repeat_climax(CtpState *state, Pitch this_note) {
    return stepspan((Interval)this_note) == stepspan((Interval)state->top);
}

// Audit (2026-07-06): mirrors same_direction in cantus.h. Confirmed
// logic-equivalent modulo representation (reads previous motion from the
// ctp_motions[] global rather than a state field, same as
// ctp_large_unrecovered_leap above).
static inline bool ctp_same_direction(CtpState *state, Interval this_motion) {
    return same_sign(stepspan(ctp_motions[state->bar - 1]),
                     stepspan(this_motion));
}

static inline bool ctp_should_change_direction(CtpState *state) {
    return state->since_turn == 3;
}

// Audit (2026-07-06): mirrors dissonant_outline in cantus.h. NOT confirmed
// equivalent — cantus.h's version flags a 6th/8ve outline OR a mode-relative
// tritone via tritone_between() (an explicit FA/MI scale-degree
// comparison); this version instead flags a 6th/8ve outline OR
// abs(interval_quality(outline)) > 1 (a generic, mode-independent
// doubly-augmented/diminished interval-quality check). These are different
// tritone-detection primitives and may not agree on which outlines they
// flag in every mode. Kept separate pending test coverage.
static inline bool ctp_dissonant_outline(CtpState *state, Pitch prev_note) {
    Interval outline = interval_between(state->prev_turn, prev_note);
    int steps = abs(stepspan(outline));
    if (steps == 6 || steps == 8 || abs(interval_quality(outline)) > 1)
        return true;
    return false;
}

// Audit (2026-07-06): mirrors tritone_in_gesture in cantus.h. Same branching
// shape (look back 2 or 3 notes, flag a tritone, then require the
// intervening motion to be stepwise), but NOT confirmed equivalent: this
// version detects the tritone via interval_pc12(...) == 6 (a generic
// 12-tone pitch-class-interval check), while cantus.h's tritone_between()
// instead compares against the mode-relative FA/MI scale degrees. As with
// ctp_dissonant_outline above, these are different tritone tests and may
// diverge in some modes. Kept separate pending test coverage.
static inline bool ctp_has_tritone(CtpState *state, int since_turn,
                                   Pitch this_note) {
    if (since_turn > 1) {
        if (abs(interval_pc12(
                interval_between(this_note, mt_ctp[state->bar - 2]))) == 6) {
            return true;
        }
    } else if (since_turn > 2) {
        if (abs(interval_pc12(
                interval_between(this_note, mt_ctp[state->bar - 3]))) == 6) {
            for (int i = 0; i < 3; i++) {
                if (abs(stepspan(interval_between(
                        mt_ctp[state->bar - i], mt_ctp[state->bar - i - 1]))) >
                    1)
                    return true;
            }
        }
    }
    return false;
}

// Audit (2026-07-06): mirrors noodling in cantus.h. Confirmed
// logic-equivalent (identical branching, using pitches_equal()/mt_ctp[]
// instead of int equality/cantus[]).
static inline bool ctp_noodling(CtpState *state, Pitch this_note) {
    if ((state->bar >= 3 && pitches_equal(this_note, mt_ctp[state->bar - 2])) &&
        (pitches_equal(mt_ctp[state->bar - 1], mt_ctp[state->bar - 3])))
        return true;
    if (state->bar >= 4 && pitches_equal(this_note, mt_ctp[state->bar - 2]) &&
        pitches_equal(this_note, mt_ctp[state->bar - 4]))
        return true;
    return false;
}

// Audit (2026-07-06): mirrors overemphasised_tone in cantus.h. NOT
// equivalent, and confirmed *intentional*: commit 88a7d62 ("relax a couple
// of counterpoint gen constraints (more to go)") deliberately raised this
// threshold from count>2 to count>3 as part of ongoing, work-in-progress
// relaxation of the counterpoint rules relative to the cantus firmus rules.
// cantus.h's version also has extra bonus-counting for scale degrees 0 and 1
// (tonic gets a stricter effective limit) that this version never had.
// Since the commit message notes "more to go", further intentional
// divergence here should be expected — do not treat this pair as a bug to
// fix.
static inline bool ctp_overemphasised_tone(CtpState *state, Pitch this_note) {
    int count = 0;
    for (int i = 0; i < state->bar; i++)
        if (pitches_equal(mt_ctp[i], this_note))
            count++;
    if (count > 3)
        return true;
    return false;
}

static inline bool is_subtonic(Pitch p) {
    return (MODE - 1) - pitch_chroma(p) == 2;
}

static inline bool is_leading_tone(Pitch p) {
    return pitch_chroma(p) - (MODE - 1) == 5;
}

static inline bool ctp_bad_penultima(CtpState *state, Pitch this_note) {
    if (MODE == 5 && !is_subtonic(this_note)) {
        return true;
    }
    if (MODE != 5 && !is_leading_tone(this_note))
        return true;
    return false;
}

static inline bool ctp_complete(CtpState *state) {
    return state->bar == BARS - 1;
}

// Audit (2026-07-06): mirrors bad_climax in cantus.h. NOT confirmed
// equivalent — cantus.h's version only flags a climax on scale degree 6
// when MODE is MAJOR or LYDIAN specifically; this version instead uses a
// mode-invariant test (interval_chroma(interval_between(tonic, climax)) ==
// 5), which would flag the equivalent climax across all modes, not just
// those two. Unclear whether cantus.h's mode restriction is intentional
// (e.g. the rule only pedagogically applies in those modes) or a narrower
// implementation that was never extended to the other modes. Kept separate
// pending test coverage.
static inline bool bad_ctp_climax(Pitch climax) {
    return interval_chroma(interval_between(tonic, climax)) == 5;
}

// Audit (2026-07-06): mirrors climax_good in cantus.h. Structurally
// identical (same 3-term boolean formula); any divergence between this pair
// is entirely inherited from bad_climax/bad_ctp_climax above.
static inline bool ctp_climax_good(CtpState *state) {
    return !state->repeated_climax && !bad_ctp_climax(state->top) &&
           !state->disconnected_climax;
}

#endif
