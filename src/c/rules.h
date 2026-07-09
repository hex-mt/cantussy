#ifndef RULES_H
#define RULES_H

#include <stdbool.h>

// Runtime-tunable cantus-firmus generation constraints, exposed to the JS
// side via set_rule_param/get_rule_param/reset_rule_params (cantus.c) so a
// UI panel can toggle/tune them without a rebuild. Toggle-type rules are
// enforced by an early-return guard inside the corresponding predicate in
// cantus.h itself (not at the call site in cantus.c), since some predicates
// (e.g. bad_climax) have more than one call site that all need to agree.
//
// IDs are hand-mirrored in src/ts/rules.ts's RULE_* constants — keep both
// lists in sync when adding/removing a rule.
typedef struct {
    bool registral_break;
    bool large_unrecovered_leap;
    bool climax_disconnected;
    bool repeated_note;
    bool dissonant_leap;
    int max_leap_size;
    bool tritone_melodic;
    int leaps_divisor;
    int max_large_leaps;
    bool steps_past_arpeggio;
    bool arpeggio_past_step;
    bool large_leap_past_step;
    int max_leaps_in_row;
    bool bad_consecutive_leaps;
    int max_range;
    bool bad_climax;
    int max_same_direction;
    bool dissonant_outline;
    bool tritone_in_gesture;
    bool noodling;
    int max_tone_repetition;
    bool bad_cadence_approach;
    bool step_bias_correction;
} RuleParams;

#define RULE_REGISTRAL_BREAK 0
#define RULE_LARGE_UNRECOVERED_LEAP 1
#define RULE_CLIMAX_DISCONNECTED 2
#define RULE_REPEATED_NOTE 3
#define RULE_DISSONANT_LEAP 4
#define RULE_MAX_LEAP_SIZE 5
#define RULE_TRITONE_MELODIC 6
#define RULE_LEAPS_DIVISOR 7
#define RULE_MAX_LARGE_LEAPS 8
#define RULE_STEPS_PAST_ARPEGGIO 9
#define RULE_ARPEGGIO_PAST_STEP 10
#define RULE_LARGE_LEAP_PAST_STEP 11
#define RULE_MAX_LEAPS_IN_ROW 12
#define RULE_BAD_CONSECUTIVE_LEAPS 13
#define RULE_MAX_RANGE 14
#define RULE_BAD_CLIMAX 15
#define RULE_MAX_SAME_DIRECTION 16
#define RULE_DISSONANT_OUTLINE 17
#define RULE_TRITONE_IN_GESTURE 18
#define RULE_NOODLING 19
#define RULE_MAX_TONE_REPETITION 20
#define RULE_BAD_CADENCE_APPROACH 21
#define RULE_STEP_BIAS_CORRECTION 22
#define RULE_PARAM_COUNT 23

extern RuleParams rules;
extern const RuleParams RULE_DEFAULTS;

void set_rule_param(int rule_id, int value);
int get_rule_param(int rule_id);
void reset_rule_params(void);

#endif
