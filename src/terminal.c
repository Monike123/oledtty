#include "terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.h"

/* One vcsa row (char+attr pairs); supports wide consoles up to 255 cols */
#define TERM_ROW_BUF_SIZE (256U * 2U)

static u8 g_row_buf[TERM_ROW_BUF_SIZE];
static bool g_warned_clamped = false;

static int read_full(int fd, void *buf, size_t count) {
    u8 *p = (u8 *)buf;
    size_t done = 0;

    while (done < count) {
        const ssize_t n = read(fd, p + done, count - done);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static int discard_bytes(int fd, size_t count) {
    u8 scratch[256];

    while (count > 0) {
        const size_t chunk = count < sizeof(scratch) ? count : sizeof(scratch);
        if (read_full(fd, scratch, chunk) < 0) {
            return -1;
        }
        count -= chunk;
    }
    return 0;
}

static char sanitize_cell(char ch) {
    if (ch < 0x20 || ch == 0x7F) {
        return ' ';
    }
    return ch;
}

int terminal_open(terminal_t *term, const char *vcsa_path) {
    if (term == NULL || vcsa_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(term, 0, sizeof(*term));
    snprintf(term->path, sizeof(term->path), "%s", vcsa_path);

    term->fd = open(vcsa_path, O_RDONLY | O_CLOEXEC);
    if (term->fd < 0) {
        return -1;
    }

    return 0;
}

void terminal_close(terminal_t *term) {
    if (term == NULL) {
        return;
    }
    if (term->fd >= 0) {
        close(term->fd);
        term->fd = -1;
    }
}

int terminal_read(terminal_t *term) {
    if (term == NULL || term->fd < 0) {
        errno = EINVAL;
        return -1;
    }

    if (lseek(term->fd, 0, SEEK_SET) < 0) {
        return -1;
    }

    u8 header[4];
    if (read_full(term->fd, header, sizeof(header)) < 0) {
        return -1;
    }

    const u8 raw_rows = header[0];
    const u8 raw_cols = header[1];
    const u8 raw_cursor_x = header[2];
    const u8 raw_cursor_y = header[3];

    if (raw_rows == 0 || raw_cols == 0) {
        errno = EAGAIN;
        return -1;
    }

    const u8 store_rows = (raw_rows < OLEDTTY_MAX_TERM_ROWS)
        ? raw_rows
        : OLEDTTY_MAX_TERM_ROWS;
    const u8 store_cols = (raw_cols < OLEDTTY_MAX_TERM_COLS)
        ? raw_cols
        : OLEDTTY_MAX_TERM_COLS;

    if (raw_rows > OLEDTTY_MAX_TERM_ROWS || raw_cols > OLEDTTY_MAX_TERM_COLS) {
        if (!g_warned_clamped) {
            log_warn("console grid %ux%u clamped to %ux%u window",
                      raw_rows, raw_cols,
                      OLEDTTY_MAX_TERM_ROWS, OLEDTTY_MAX_TERM_COLS);
            g_warned_clamped = true;
        }
    }

    const size_t row_bytes = (size_t)raw_cols * 2U;
    if (row_bytes > TERM_ROW_BUF_SIZE) {
        log_error("console row width %u too wide", raw_cols);
        errno = EIO;
        return -1;
    }

    /* Keep a store_rows window that includes the cursor row (Kali can use 80+ rows). */
    u8 start_row = 0;
    if (raw_cursor_y >= store_rows) {
        start_row = (u8)(raw_cursor_y + 1U - store_rows);
    }
    if ((u16)start_row + (u16)store_rows > (u16)raw_rows) {
        start_row = (u8)(raw_rows - store_rows);
    }

    if (discard_bytes(term->fd, (size_t)start_row * row_bytes) < 0) {
        return -1;
    }

    for (u8 r = 0; r < store_rows; ++r) {
        memset(term->cells[r], ' ', store_cols);

        if ((u16)start_row + (u16)r < (u16)raw_rows) {
            if (read_full(term->fd, g_row_buf, row_bytes) < 0) {
                return -1;
            }
            for (u8 c = 0; c < store_cols; ++c) {
                term->cells[r][c] = sanitize_cell((char)g_row_buf[(size_t)c * 2U]);
            }
        }
    }

    /* Discard any rows below our window. */
    const u8 rows_after = (u8)(raw_rows - start_row - store_rows);
    if (rows_after > 0 && discard_bytes(term->fd, (size_t)rows_after * row_bytes) < 0) {
        return -1;
    }

    term->rows = store_rows;
    term->cols = store_cols;
    term->cursor_x = (raw_cursor_x < store_cols) ? raw_cursor_x : (u8)(store_cols - 1);
    term->cursor_y = (raw_cursor_y >= start_row)
        ? (u8)(raw_cursor_y - start_row)
        : 0;

    return 0;
}

char terminal_char_at(const terminal_t *term, u8 row, u8 col) {
    if (term == NULL || row >= term->rows || col >= term->cols) {
        return ' ';
    }
    if (row >= OLEDTTY_MAX_TERM_ROWS || col >= OLEDTTY_MAX_TERM_COLS) {
        return ' ';
    }
    return term->cells[row][col];
}
