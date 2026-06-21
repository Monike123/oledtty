#include "config.h"
#include "types.h"
#include "framebuffer.h"
#include "ssd1306.h"
#include "terminal.h"
#include "viewport.h"
#include "render.h"
#include "history.h"
#include "control.h"
#include "keyboard.h"
#include "timer.h"
#include "log.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char i2c_path[64];
    char vcsa_path[64];
    char input_path[128];
    u8   i2c_address;
    u8   contrast;
    u32  poll_ms;
    u32  blink_ms;
    bool cursor_enabled;
    bool dry_run;
    bool once;
    bool no_splash;
    bool no_keyboard;
} app_config_t;

static void app_config_set_defaults(app_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->i2c_path, sizeof(cfg->i2c_path), "%s", OLEDTTY_I2C_BUS_PATH);
    snprintf(cfg->vcsa_path, sizeof(cfg->vcsa_path), "%s", OLEDTTY_VCSA_PATH);
    cfg->i2c_address = OLEDTTY_I2C_ADDRESS;
    cfg->contrast = OLEDTTY_CONTRAST;
    cfg->poll_ms = OLEDTTY_POLL_INTERVAL_MS;
    cfg->blink_ms = OLEDTTY_CURSOR_BLINK_MS;
    cfg->cursor_enabled = true;
    cfg->dry_run = false;
    cfg->once = false;
    cfg->no_splash = false;
    cfg->no_keyboard = false;
}

static void print_usage(const char *prog) {
    printf(
        "oledtty %s - mirror the Linux console onto an SSD1306 OLED\n\n"
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --device PATH      I2C bus device (default: %s)\n"
        "  --address ADDR     I2C address (default: 0x%02X)\n"
        "  --vcsa PATH        Console device (default: %s)\n"
        "  --input-dev PATH   Keyboard evdev device (default: auto-detect)\n"
        "  --no-keyboard      Disable keyboard viewport panning\n"
        "  --poll-ms N        Poll interval ms (default: %d)\n"
        "  --blink-ms N       Cursor blink half-period ms (default: %d)\n"
        "  --contrast N       Contrast 0-255 (default: %d)\n"
        "  --no-cursor        Disable cursor highlight\n"
        "  --no-splash        Skip startup splash\n"
        "  --dry-run          ASCII preview, no hardware\n"
        "  --once             Single frame then exit\n"
        "  -v, --verbose      Debug logging\n"
        "  -q, --quiet        Errors only\n"
        "  -h, --help         Show help\n"
        "      --version      Show version\n",
        OLEDTTY_VERSION, prog,
        OLEDTTY_I2C_BUS_PATH, OLEDTTY_I2C_ADDRESS, OLEDTTY_VCSA_PATH,
        OLEDTTY_POLL_INTERVAL_MS, OLEDTTY_CURSOR_BLINK_MS, OLEDTTY_CONTRAST);
}

enum {
    OPT_DEVICE = 1000,
    OPT_ADDRESS,
    OPT_VCSA,
    OPT_INPUT_DEV,
    OPT_NO_KEYBOARD,
    OPT_POLL_MS,
    OPT_BLINK_MS,
    OPT_CONTRAST,
    OPT_NO_CURSOR,
    OPT_NO_SPLASH,
    OPT_DRY_RUN,
    OPT_ONCE,
    OPT_VERSION
};

static int parse_args(int argc, char **argv, app_config_t *cfg) {
    static const struct option long_opts[] = {
        {"device",      required_argument, NULL, OPT_DEVICE},
        {"address",     required_argument, NULL, OPT_ADDRESS},
        {"vcsa",        required_argument, NULL, OPT_VCSA},
        {"input-dev",   required_argument, NULL, OPT_INPUT_DEV},
        {"no-keyboard", no_argument,       NULL, OPT_NO_KEYBOARD},
        {"poll-ms",     required_argument, NULL, OPT_POLL_MS},
        {"blink-ms",    required_argument, NULL, OPT_BLINK_MS},
        {"contrast",    required_argument, NULL, OPT_CONTRAST},
        {"no-cursor",   no_argument,       NULL, OPT_NO_CURSOR},
        {"no-splash",   no_argument,       NULL, OPT_NO_SPLASH},
        {"dry-run",     no_argument,       NULL, OPT_DRY_RUN},
        {"once",        no_argument,       NULL, OPT_ONCE},
        {"verbose",     no_argument,       NULL, 'v'},
        {"quiet",       no_argument,       NULL, 'q'},
        {"help",        no_argument,       NULL, 'h'},
        {"version",     no_argument,       NULL, OPT_VERSION},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "vqh", long_opts, NULL)) != -1) {
        switch (opt) {
            case OPT_DEVICE:
                snprintf(cfg->i2c_path, sizeof(cfg->i2c_path), "%s", optarg);
                break;
            case OPT_ADDRESS:
                cfg->i2c_address = (u8)strtoul(optarg, NULL, 0);
                break;
            case OPT_VCSA:
                snprintf(cfg->vcsa_path, sizeof(cfg->vcsa_path), "%s", optarg);
                break;
            case OPT_INPUT_DEV:
                snprintf(cfg->input_path, sizeof(cfg->input_path), "%s", optarg);
                break;
            case OPT_NO_KEYBOARD:
                cfg->no_keyboard = true;
                break;
            case OPT_POLL_MS:
                cfg->poll_ms = (u32)strtoul(optarg, NULL, 10);
                break;
            case OPT_BLINK_MS:
                cfg->blink_ms = (u32)strtoul(optarg, NULL, 10);
                break;
            case OPT_CONTRAST:
                cfg->contrast = (u8)strtoul(optarg, NULL, 0);
                break;
            case OPT_NO_CURSOR:
                cfg->cursor_enabled = false;
                break;
            case OPT_NO_SPLASH:
                cfg->no_splash = true;
                break;
            case OPT_DRY_RUN:
                cfg->dry_run = true;
                break;
            case OPT_ONCE:
                cfg->once = true;
                break;
            case 'v':
                log_set_level(LOG_LEVEL_DEBUG);
                break;
            case 'q':
                log_set_level(LOG_LEVEL_ERROR);
                break;
            case OPT_VERSION:
                printf("oledtty %s\n", OLEDTTY_VERSION);
                exit(0);
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }
    return 0;
}

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_force_redraw = 0;

static void on_shutdown_signal(int signum) {
    (void)signum;
    g_running = 0;
}

static void on_force_redraw_signal(int signum) {
    (void)signum;
    g_force_redraw = 1;
}

static void install_signal_handlers(void) {
    struct sigaction sa_shutdown;
    memset(&sa_shutdown, 0, sizeof(sa_shutdown));
    sa_shutdown.sa_handler = on_shutdown_signal;
    sigemptyset(&sa_shutdown.sa_mask);
    sigaction(SIGINT, &sa_shutdown, NULL);
    sigaction(SIGTERM, &sa_shutdown, NULL);

    struct sigaction sa_redraw;
    memset(&sa_redraw, 0, sizeof(sa_redraw));
    sa_redraw.sa_handler = on_force_redraw_signal;
    sigemptyset(&sa_redraw.sa_mask);
    sigaction(SIGUSR1, &sa_redraw, NULL);
}

static void show_splash(ssd1306_t *display) {
    framebuffer_t fb;
    fb_init(&fb);

    fb_draw_text(&fb, 4, 0, "OLEDTTY", true);
    fb_draw_text(&fb, 4, OLEDTTY_CHAR_CELL_H, "v" OLEDTTY_VERSION, true);

    ssd1306_flush_frame(display, fb.data, (u16)sizeof(fb.data));
}

static void ascii_preview(const framebuffer_t *fb) {
    if (isatty(STDOUT_FILENO)) {
        fputs("\x1b[2J\x1b[H", stdout);
    }

    putchar('+');
    for (int x = 0; x < OLEDTTY_WIDTH; ++x) {
        putchar('-');
    }
    putchar('+');
    putchar('\n');

    for (int y = 0; y < OLEDTTY_HEIGHT; ++y) {
        putchar('|');
        for (int x = 0; x < OLEDTTY_WIDTH; ++x) {
            const u16 byte_index = (u16)(x + (y / 8) * fb->width);
            const u8 bit_mask = (u8)(1U << (y & 7));
            putchar((fb->data[byte_index] & bit_mask) ? '#' : ' ');
        }
        putchar('|');
        putchar('\n');
    }

    putchar('+');
    for (int x = 0; x < OLEDTTY_WIDTH; ++x) {
        putchar('-');
    }
    putchar('+');
    putchar('\n');
    fflush(stdout);
}

static void capture_cursor_signature(const terminal_t *term, char *buf, size_t len) {
    if (term == NULL || buf == NULL || len == 0) {
        return;
    }
    u8 c = 0;
    for (; c < term->cols && c < OLEDTTY_MAX_TERM_COLS && (size_t)c + 1U < len; ++c) {
        buf[c] = terminal_char_at(term, term->cursor_y, c);
    }
    buf[c] = '\0';
}

static void apply_pan_cmd(view_state_t *view, control_cmd_t cmd, const history_t *hist) {
    if (cmd == CTRL_CMD_NONE) {
        return;
    }
    const u16 hcount = viewport_history_count(hist);
    view_state_apply_cmd(view, cmd, hcount);
    if (cmd != CTRL_CMD_LIVE) {
        log_debug("viewport pan: cmd=%d row=%u col=%u",
                  (int)cmd, view->pan_row, view->pan_col);
    }
}

int main(int argc, char **argv) {
    app_config_t cfg;
    app_config_set_defaults(&cfg);

    if (parse_args(argc, argv, &cfg) != 0) {
        return 1;
    }

    install_signal_handlers();

    if (nice(10) < 0) {
        log_debug("nice(10) failed: %s", strerror(errno));
    }

    ssd1306_t display;
    memset(&display, 0, sizeof(display));

    control_server_t ctrl_srv;
    memset(&ctrl_srv, 0, sizeof(ctrl_srv));

    keyboard_t keyboard;
    memset(&keyboard, 0, sizeof(keyboard));

    if (!cfg.dry_run) {
        if (ssd1306_init(&display, cfg.i2c_path, cfg.i2c_address) < 0) {
            log_error("failed to initialize SSD1306: %s", strerror(errno));
            return 1;
        }

        if (ssd1306_set_contrast(&display, cfg.contrast) < 0) {
            log_warn("failed to set contrast: %s", strerror(errno));
        }

        if (control_server_start(&ctrl_srv, OLEDTTY_CONTROL_SOCKET) < 0) {
            log_warn("control socket unavailable: %s", strerror(errno));
        }

        if (!cfg.no_keyboard) {
            const char *input_dev = cfg.input_path[0] ? cfg.input_path : NULL;
            if (keyboard_open(&keyboard, input_dev) < 0) {
                log_debug("keyboard pan unavailable (use oledtty-ctl from SSH)");
            }
        }

        if (!cfg.no_splash) {
            show_splash(&display);
            timer_sleep_ms(700, &g_running);
        }
    } else {
        log_info("dry-run mode: no I2C hardware will be touched");
    }

    terminal_t term;
    if (terminal_open(&term, cfg.vcsa_path) < 0) {
        log_error("failed to open %s: %s", cfg.vcsa_path, strerror(errno));
        if (!cfg.dry_run) {
            keyboard_close(&keyboard);
            control_server_stop(&ctrl_srv);
            ssd1306_shutdown(&display);
        }
        return 1;
    }

    history_t hist;
    history_init(&hist);

    view_state_t view;
    view_state_init(&view);

    framebuffer_t fb;
    fb_init(&fb);

    viewport_t vp;
    char prev_cursor_sig[OLEDTTY_MAX_TERM_COLS + 1];
    prev_cursor_sig[0] = '\0';

    bool blink_on = true;
    u32 last_blink_toggle = timer_now_ms();
    bool have_term = false;

    log_info("oledtty %s (poll=%ums vcsa=%s rows=%d cols=%d)",
              OLEDTTY_VERSION, cfg.poll_ms, cfg.vcsa_path,
              OLEDTTY_VISIBLE_ROWS, OLEDTTY_VISIBLE_COLS);

    while (g_running) {
        apply_pan_cmd(&view, control_server_poll(&ctrl_srv), &hist);
        apply_pan_cmd(&view, keyboard_poll(&keyboard), &hist);

        const int term_rc = terminal_read(&term);
        if (term_rc < 0) {
            if (errno != EAGAIN) {
                log_warn("terminal_read failed: %s", strerror(errno));
            }
            if (!have_term) {
                timer_sleep_ms(cfg.poll_ms, &g_running);
                continue;
            }
        } else {
            have_term = true;
            history_update(&hist, &term);
        }

        char cursor_sig[OLEDTTY_MAX_TERM_COLS + 1];
        capture_cursor_signature(&term, cursor_sig, sizeof(cursor_sig));
        if (view.mode == VIEW_MODE_MANUAL &&
            strcmp(cursor_sig, prev_cursor_sig) != 0) {
            view_state_enter_live(&view);
        }
        snprintf(prev_cursor_sig, sizeof(prev_cursor_sig), "%s", cursor_sig);

        const u32 now = timer_now_ms();
        if (cfg.cursor_enabled && (now - last_blink_toggle) >= cfg.blink_ms) {
            blink_on = !blink_on;
            last_blink_toggle = now;
        }

        viewport_build(&term, &hist, &view, &vp);
        render_frame(&fb, &vp, cfg.cursor_enabled && blink_on);

        if (cfg.dry_run) {
            ascii_preview(&fb);
        } else if (ssd1306_flush_frame(&display, fb.data, (u16)sizeof(fb.data)) < 0) {
            log_error("ssd1306_flush_frame failed: %s", strerror(errno));
            break;
        }

        (void)g_force_redraw;

        if (cfg.once) {
            break;
        }

        timer_sleep_ms(cfg.poll_ms, &g_running);
    }

    terminal_close(&term);
    keyboard_close(&keyboard);
    control_server_stop(&ctrl_srv);

    if (!cfg.dry_run) {
        ssd1306_clear_display(&display);
        ssd1306_shutdown(&display);
    }

    log_info("oledtty stopped");
    return 0;
}
