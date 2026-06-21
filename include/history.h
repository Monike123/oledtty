#ifndef OLEDTTY_HISTORY_H
#define OLEDTTY_HISTORY_H

#include "types.h"
#include "config.h"
#include "terminal.h"

typedef struct {
    char text[OLEDTTY_HISTORY_LINE_LEN];
} hist_entry_t;

typedef struct {
    hist_entry_t entries[OLEDTTY_HISTORY_CAPACITY];
    u16 head;
    u16 count;
    char prev_rows[OLEDTTY_MAX_TERM_ROWS][OLEDTTY_MAX_TERM_COLS + 1];
    u8   prev_rows_count;
} history_t;

void history_init(history_t *h);
void history_update(history_t *h, const terminal_t *term);

u16  history_count(const history_t *h);
const hist_entry_t *history_at(const history_t *h, u16 index_from_oldest);
const hist_entry_t *history_newest_at(const history_t *h, u16 index_from_newest);

#endif
