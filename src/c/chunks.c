#include "chunks.h"
#include "meantonal.h"

Interval motions[MELODIC_INDEX_COUNT] = {(Interval){-5, -2}, // -P8
                                         (Interval){-4, -1}, // -M6
                                         (Interval){-3, -2}, // -m6
                                         (Interval){-3, -1}, // -P5
                                         (Interval){-2, -1}, // -P4
                                         (Interval){-2, 0},  // -M3
                                         (Interval){-1, -1}, // -m3
                                         (Interval){-1, 0},  // -M2
                                         (Interval){0, -1},  // -m2
                                         (Interval){0, 0},   // P1
                                         (Interval){0, 1},   // m2
                                         (Interval){1, 0},   // M2
                                         (Interval){1, 1},   // m3
                                         (Interval){2, 0},   // M3
                                         (Interval){2, 1},   // P4
                                         (Interval){3, 1},   // P5
                                         (Interval){3, 2},   // m6
                                         (Interval){4, 1},   // M6
                                         (Interval){5, 2}};  // M10

Interval consonances[HARMONIC_INDEX_COUNT] = {(Interval){-7, -2}, // -M10
                                              (Interval){-6, -3}, // -m10
                                              (Interval){-5, -2}, // -P8
                                              (Interval){-4, -1}, // -M6
                                              (Interval){-3, -2}, // -m6
                                              (Interval){-3, -1}, // -P5
                                              (Interval){-2, 0},  // -M3
                                              (Interval){-1, -1}, // -m3
                                              (Interval){0, 0},   // P1
                                              (Interval){1, 1},   // m3
                                              (Interval){2, 0},   // M3
                                              (Interval){3, 1},   // P5
                                              (Interval){3, 2},   // m6
                                              (Interval){4, 1},   // M6
                                              (Interval){5, 2},   // P8
                                              (Interval){6, 3},   // m10
                                              (Interval){7, 2}};  // M10

chunk_node *data[MELODIC_INDEX_COUNT][HARMONIC_INDEX_COUNT] = {0};

chunk_node node_pool[NODE_POOL_SIZE];

chunk_node *create_chunk(void) {
    static int i = 0;
    if (i >= NODE_POOL_SIZE) {
        fprintf(stderr, "node_pool exhausted (NODE_POOL_SIZE=%d)\n",
                NODE_POOL_SIZE);
        return &node_pool[NODE_POOL_SIZE - 1];
    }
    i++;
    return node_pool + i - 1;
}

bool chunks_initialised = false;

void generate_chunks(void) {
    if (chunks_initialised)
        return;
    chunks_initialised = true;
    for (int i = 0; i < MELODIC_INDEX_COUNT; i++) {
        for (int j = 0; j < HARMONIC_INDEX_COUNT; j++) {
            for (int k = 0; k < HARMONIC_INDEX_COUNT; k++) {
                // no unisons (outlide of outer measures)
                if (stepspan(consonances[k]) == 0 ||
                    // no voice crossing
                    (stepspan(consonances[j]) > 0 &&
                     stepspan(consonances[k]) < 0) ||
                    (stepspan(consonances[j]) < 0 &&
                     stepspan(consonances[k]) > 0) ||
                    // no consecutive perfects
                    (stepspan(consonances[j]) == stepspan(consonances[k]) &&
                     (abs(stepspan(consonances[j])) == 0 ||
                      abs(stepspan(consonances[j])) == 4 ||
                      abs(stepspan(consonances[j])) == 7)))
                    continue;
                Interval ctp_motion = intervals_subtract(
                    intervals_add(consonances[k], motions[i]), consonances[j]);
                // no direct motion to perfect consonances
                if (((stepspan(motions[i]) > 0 && stepspan(ctp_motion) > 0) ||
                     (stepspan(motions[i]) < 0 && stepspan(ctp_motion) < 0)) &&
                    (abs(stepspan(consonances[k])) == 4 ||
                     abs(stepspan(consonances[k])) == 7))
                    continue;
                // no similar motion where either part is moving by more than a
                // third.
                if ((stepspan(motions[i]) > 2 && stepspan(ctp_motion) > 2) ||
                    (stepspan(motions[i]) < -2 && stepspan(ctp_motion) < -2))
                    continue;

                for (int m = 0; m < MELODIC_INDEX_COUNT; m++) {
                    if (intervals_equal(ctp_motion, motions[m])) {
                        add_chunk(motions[i], consonances[j],
                                  (chunk){consonances[k], ctp_motion});
                    }
                }
            }
        }
    }
}

int map_melodic_index(Interval m) {
    for (int i = 0; i < MELODIC_INDEX_COUNT; i++) {
        if (intervals_equal(m, motions[i]))
            return i;
    }
    return -1;
}

int map_harmonic_index(Interval m) {
    for (int i = 0; i < HARMONIC_INDEX_COUNT; i++) {
        if (intervals_equal(m, consonances[i]))
            return i;
    }
    return -1;
}

void add_chunk(Interval i, Interval j, chunk c) {
    int fi = map_melodic_index(i);
    int si = map_harmonic_index(j);
    if (fi < 0 || fi >= MELODIC_INDEX_COUNT || si < 0 ||
        si >= HARMONIC_INDEX_COUNT) {
        char iname[8], jname[8];
        interval_name(i, iname);
        interval_name(j, jname);
        fprintf(stderr, "Invalid index: (%s, %s)\n", iname, jname);
        return;
    }

    chunk_node *new_node = create_chunk();
    new_node->data = c;
    new_node->next = data[fi][si];
    data[fi][si] = new_node; // insert at head
}

void shuffle_list(chunk_node **head) {
    if (!head || !*head)
        return;

    // Step 1: copy node pointers into an array. Bucket lists are expected to
    // stay well under MAX_CHUNKS; if one ever grows past it, entries beyond
    // MAX_CHUNKS are silently dropped from the shuffle rather than
    // overflowing `arr`.
    chunk_node *arr[MAX_CHUNKS];
    int n = 0;
    chunk_node *cur = *head;
    while (cur && n < MAX_CHUNKS) {
        arr[n++] = cur;
        cur = cur->next;
    }

    // Step 2: Fisher–Yates shuffle
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        chunk_node *tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }

    // Step 3: counteract bias towards "early spending" of leaps
    // by strongly preferencing steps as first choices
    int s = rand() % 5;
    if (s > 1) {
        for (int i = 0; i < n - 1; i++) {
            if (abs(stepspan(arr[i]->data.motion)) == 1) {
                chunk_node *tmp = arr[i];
                arr[i] = arr[0];
                arr[0] = tmp;
                break;
            }
        }
        for (int i = 1; i < n - 1; i++) {
            if (abs(stepspan(arr[i]->data.motion)) == 1) {
                chunk_node *tmp = arr[i];
                arr[i] = arr[1];
                arr[1] = tmp;
                break;
            }
        }
    }

    // Step 4: re-link nodes in shuffled order
    for (int i = 0; i < n - 1; i++) {
        arr[i]->next = arr[i + 1];
    }
    arr[n - 1]->next = NULL;

    *head = arr[0];
}

chunk_node *get_chunks(Interval cons, Interval motion) {
    int fi = map_melodic_index(motion);
    int si = map_harmonic_index(cons);

    if (fi < 0 || fi >= MELODIC_INDEX_COUNT || si < 0 ||
        si >= HARMONIC_INDEX_COUNT)
        return NULL;

    return data[fi][si]; // may be NULL if no chunks
}

chunk_node *clone_list_into(const chunk_node *head,
                            chunk_node buf[MAX_CHUNKS]) {
    const chunk_node *src = head;
    chunk_node *prev = NULL;
    int count = 0;

    // As in shuffle_list, entries beyond MAX_CHUNKS are silently dropped
    // rather than overflowing `buf`.
    while (src && count < MAX_CHUNKS) {
        buf[count].data = src->data; // copy chunk by value
        if (prev) {
            prev->next = &buf[count];
        }
        prev = &buf[count];
        src = src->next;
        count++;
    }

    if (prev)
        prev->next = NULL;

    shuffle_list(&buf);

    return buf;
}
