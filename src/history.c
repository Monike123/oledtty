#include "history.h"

#include <stdio.h>
#include <string.h>

static bool line_is_blank(const char *line, u8 len) {
    for (u8 i = 0; i < len; ++i) {
        if (line[i] != ' ' && line[i] != '\0') {
            return false;
        }
    }
    return true;
}

static void copy_term_row(char *dst, const terminal_t *term, u8 row) {
    u8 c;
    for (c = 0; c < term->cols && c < OLEDTTY_MAX_TERM_COLS; ++c) {
        char ch = terminal_char_at(term, row, c);
        if (ch < 0x20 || ch == 0x7F) {
            ch = ' ';
        }
        dst[c] = ch;
    }
    dst[c] = '\0';
}

static const hist_entry_t *history_last_entry(const history_t *h) {
    if (h == NULL || h->count == 0) {
        return NULL;
    }
    const u16 idx = (h->head == 0)
        ? (u16)(OLEDTTY_HISTORY_CAPACITY - 1U)
        : (u16)(h->head - 1U);
    return &h->entries[idx];
}

static void history_push(history_t *h, const char *text) {
    if (h == NULL || text == NULL) {
        return;
    }
    if (line_is_blank(text, (u8)strnlen(text, OLEDTTY_HISTORY_LINE_LEN))) {
        return;
    }

    const hist_entry_t *last = history_last_entry(h);
    if (last != NULL && strcmp(last->text, text) == 0) {
        return;
    }

    hist_entry_t *slot = &h->entries[h->head];
    snprintf(slot->text, sizeof(slot->text), "%s", text);

    h->head = (u16)((h->head + 1U) % OLEDTTY_HISTORY_CAPACITY);
    if (h->count < OLEDTTY_HISTORY_CAPACITY) {
        ++h->count;
    }
}

void history_init(history_t *h) {
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

void history_update(history_t *h, const terminal_t *term) {
    if (h == NULL || term == NULL || term->rows == 0) {
        return;
    }

    char row_buf[OLEDTTY_MAX_TERM_COLS + 1];

    for (u8 r = 0; r < term->rows && r < OLEDTTY_MAX_TERM_ROWS; ++r) {
        copy_term_row(row_buf, term, r);

        if (r < h->prev_rows_count &&
            strncmp(row_buf, h->prev_rows[r], sizeof(row_buf)) == 0) {
            continue;
        }

        history_push(h, row_buf);
        snprintf(h->prev_rows[r], sizeof(h->prev_rows[r]), "%s", row_buf);
    }

    h->prev_rows_count = term->rows;
}

u16 history_count(const history_t *h) {
    return h == NULL ? 0 : h->count;
}

const hist_entry_t *history_at(const history_t *h, u16 index_from_oldest) {
    if (h == NULL || index_from_oldest >= h->count) {
        return NULL;
    }

    u16 start = (h->count < OLEDTTY_HISTORY_CAPACITY)
        ? 0
        : h->head;
    u16 idx = (u16)((start + index_from_oldest) % OLEDTTY_HISTORY_CAPACITY);
    return &h->entries[idx];
}

const hist_entry_t *history_newest_at(const history_t *h, u16 index_from_newest) {
    if (h == NULL || h->count == 0 || index_from_newest >= h->count) {
        return NULL;
    }
    const u16 oldest = (u16)(h->count - 1U - index_from_newest);
    return history_at(h, oldest);
}
