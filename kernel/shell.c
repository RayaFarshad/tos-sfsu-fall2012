#include <kernel.h>

static WINDOW shell_wnd = {0, 20, 40, 5, 0, 0, 0xDC};
WINDOW* shell_wnd_ptr = &shell_wnd;
static WINDOW train_wnd = {0, 0, 80, 8, 0, 0, ' '};
static WINDOW pacman_wnd = {61, 8, 0, 0, 0, 0, ' '};
static WINDOW divider_wnd = {0, 8, 80, 1, 0, 0, ' '};

void run_train_app(WINDOW* wnd) {
    static int already_run = 0;

    if (already_run) {
        wprintf(&shell_wnd, "Train application already running.\n\n");
        return;
    }

    already_run = 1;
    init_train(wnd);
}

void run_pacman_app(WINDOW* wnd) {
    static int already_run = 0;

    if (already_run) {
        wprintf(&shell_wnd, "PacMan already running.\n\n");
        return;
    }

    already_run = 1;
    init_pacman(wnd, 4);
}

void read_line(char* buffer, int max_len) {
    Keyb_Message msg;
    EM_Message * em_msg;
    PROCESS sender_proc;
    char ch;
    int i = 0;

    while (1) {
        msg.key_buffer = &ch;
        // send(keyb_port, &msg);
        // send(em_port, &em_msg);
        em_msg = (EM_Message *) receive(&sender_proc);
        switch (em_msg->key) {
            case '\b':
                if (i == 0)
                    continue;
                i--;
                break;
            case 13:
                buffer[i] = '\0';
                wprintf(&shell_wnd, "\n");
                return;
            default:
                if (i == max_len)
                    break;
                buffer[i++] = ch;
                break;
        }
        wprintf(&shell_wnd, "%c", ch);
    }
}

int is_command(char* s1, char* s2) {
    while (*s1 == *s2 && *s2 != '\0') {
        s1++;
        s2++;
    }
    return *s2 == '\0';
}

void process_command(char* command) {
    if (is_command(command, "ps")) {
        print_all_processes(&shell_wnd);
        return;
    }

    if (is_command(command, "clear")) {
        clear_window(&shell_wnd);
        return;
    }

    if (is_command(command, "pacman")) {
        run_pacman_app(&pacman_wnd);
        return;
    }

    if (is_command(command, "train")) {
        run_train_app(&train_wnd);
        return;
    }

    if (is_command(command, "go")) {
        set_train_speed("4");
        return;
    }

    if (is_command(command, "stop")) {
        set_train_speed("0");
        return;
    }

    if (is_command(command, "rev")) {
        set_train_speed("D");
        return;
    }

    if (is_command(command, "sendudp")) {
        ne_send_udp(command + 8);
        return;
    }

    if (is_command(command, "send")) {
        ne_test_transmit();
        return;
    }

    if (is_command(command, "ne")) {
        ne_config(command + 3);
        return;
    }

    if (is_command(command, "pong")) {
        init_pong();
        return;
    }
    if (is_command(command, "chat")) {
        init_chat(); //hello
        return;
    }

    if (is_command(command, "coin")) {
        pong_coin_inserted();
        return;
    }

    if (is_command(command, "help")) {
        wprintf(&shell_wnd, "Commands:\n");
        wprintf(&shell_wnd, "  - help   show this help\n");
        wprintf(&shell_wnd, "  - clear  clear window\n");
        wprintf(&shell_wnd, "  - ps     show all processes\n");
        wprintf(&shell_wnd, "  - pacman start PacMan\n");
        wprintf(&shell_wnd, "  - go     make the train go\n");
        wprintf(&shell_wnd, "  - stop   make the train stop\n");
        wprintf(&shell_wnd, "  - rev    reverse train direction\n");
        wprintf(&shell_wnd, "  - train  start train application\n");
        wprintf(&shell_wnd, "  - ne     NE2000 tools\n");
        wprintf(&shell_wnd, "  - send   sends test packet from NE2K\n\n");
        return;
    }

    /* Room for more commands! */
    wprintf(&shell_wnd, "Syntax error! Type 'help' for help.\n");
}

void print_prompt() {
    wprintf(&shell_wnd, "> ");
}

void shell_process(PROCESS self, PARAM param) {
    char buffer[80];
    int i;

    clear_window(&shell_wnd);
    clear_window(&train_wnd);
    clear_window(&divider_wnd);
    for (i = 0; i < 79; i++)
        output_char(&divider_wnd, 196);

    wprintf(&shell_wnd, "TOS Shell\n");
    wprintf(&shell_wnd, "---------\n\n");

    Keyb_Message msg;
    EM_Message * em_msg;
    PROCESS sender_proc;
    char ch;
    i = 0;
    unsigned char process_input;

    em_register_kboard_listener();

    while (1) {
        i = 0;
        process_input = 1;
        print_prompt();
        while (process_input) {
            msg.key_buffer = &ch;
            // send(keyb_port, &msg);
            // send(em_port, &em_msg);
            em_msg = (EM_Message *) receive(&sender_proc);
            ch = em_msg->key;
            switch (ch) {
                case '\b':
                    if (i == 0)
                        continue;
                    i--;
                    break;
                case 13:
                    buffer[i] = '\0';
                    wprintf(&shell_wnd, "\n");
                    process_input = 0;
                    break;
                default:
                    if (i == 80)
                        break;
                    buffer[i++] = ch;
                    break;
            }
            if (process_input)
                wprintf(&shell_wnd, "%c", ch);
        }
        process_command(buffer);
    }
}

void init_shell() {
    create_process(shell_process, 5, 0, "Shell Process");
    resign();
}
