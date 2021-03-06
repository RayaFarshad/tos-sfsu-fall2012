#include <kernel.h>
#include <nll.h>
#include <keycodes.h>

PORT em_port;
PORT em_test_port;

typedef struct _Event_Listener {
    PROCESS process;
    PORT proc_port;
    unsigned int udp_port;
    unsigned short magic;
    EM_Message data;
} Event_Listener;

#define MAX_LISTENERS 20
Event_Listener UDP_LISTENERS[MAX_LISTENERS];
Event_Listener KEY_LISTENERS[MAX_LISTENERS];
static unsigned int nbr_of_udp_listeners = 0;
static unsigned int nbr_of_key_listeners = 0;

void em_new_keystroke(unsigned char key) {
    EM_Message msg;
    msg.sanity = SANITY_SHORT;
    msg.type = EM_EVENT_KEY_STROKE;
    msg.key = key;
    em_new_event(&msg);
}

void em_new_udp_packet(UDP * udp) {
    EM_Message msg;
    msg.sanity = SANITY_SHORT;
    msg.type = EM_EVENT_UDP_PACKET_RECEIVED;
    msg.port = ntohs_tos(udp->dst_port);
    msg.data = udp;
    em_new_event(&msg);
}

void em_new_event(EM_Message * msg) {
    message(em_port, msg);
}

BOOL em_register_udp_listener(unsigned int udp_port) {
    if (nbr_of_udp_listeners > MAX_LISTENERS - 1)
        return 0;
    UDP_LISTENERS[nbr_of_udp_listeners].process = active_proc;
    UDP_LISTENERS[nbr_of_udp_listeners].proc_port = active_proc->first_port;
    UDP_LISTENERS[nbr_of_udp_listeners].udp_port = udp_port;
    nbr_of_udp_listeners++;
    return 1;
}

BOOL em_register_kboard_listener() {
    if (nbr_of_key_listeners > MAX_LISTENERS - 1)
        return 0;
    KEY_LISTENERS[nbr_of_key_listeners].process = active_proc;
    KEY_LISTENERS[nbr_of_key_listeners].proc_port = active_proc->first_port;
    nbr_of_key_listeners++;
    return 1;
}

void em_process(PROCESS self, PARAM param) {
    EM_Message * msg;
    PROCESS sender_proc;
    PORT dst_port;
    unsigned int kboard_focus = 0;
    unsigned int i;
    unsigned int msg_type;

    EM_Message focus_change;
    focus_change.type = EM_EVENT_FOCUS_IN;
    focus_change.sanity = SANITY_SHORT;
    EM_Message focus_out;
    focus_change.type = EM_EVENT_FOCUS_OUT;
    focus_change.sanity = SANITY_SHORT;

    while (1) {

        msg = (EM_Message*) receive(&sender_proc);
        // assert(msg->sanity == SANITY_SHORT);

        switch (msg->type) {

            case EM_EVENT_KEY_STROKE:
                if (msg->key == KEY_TAB) {
                    // change focus to next keyboard listener

                    dst_port = KEY_LISTENERS[kboard_focus].proc_port;
                    focus_out.type = EM_EVENT_FOCUS_OUT;
                    // kprintf("%d to %d (%d) -", focus_out.type, kboard_focus, dst_port);
                    message(dst_port, &focus_out);

                    kboard_focus = (kboard_focus + 1) % nbr_of_key_listeners;
                    dst_port = KEY_LISTENERS[kboard_focus].proc_port;
                    focus_change.type = EM_EVENT_FOCUS_IN;
                    message(dst_port, &focus_change);

                } else {
                    // send keystroke to current listener
                    dst_port = KEY_LISTENERS[kboard_focus].proc_port;
                    message(dst_port, msg);
                }
                break;

            case EM_EVENT_UDP_PACKET_RECEIVED:
                // kprintf("PACKET");
                for (i = 0; i < MAX_LISTENERS; i++) {
                    if (UDP_LISTENERS[i].process == NULL) break;
                    if (UDP_LISTENERS[i].udp_port == msg->port) {
                        dst_port = UDP_LISTENERS[i].proc_port;
                        // kprintf("Sending UDP packet to: ");
                        // kprintf(UDP_LISTENERS[i].process->name);
                        message(dst_port, msg);
                    }
                }
                break;

            default:
                kprintf("EV_TYPE=%d", msg->type);
                panic("UNKNOWN EVENT TYPE");
                break;
        }
    }
}

void em_test_process(PROCESS self, PARAM param) {
    em_register_kboard_listener();
    em_register_udp_listener(8080);
    PROCESS sender_proc;
    EM_Message * msg;
    while (1) {
        msg = (EM_Message*) receive(&sender_proc);
        switch (msg->type) {
            case EM_GET_NEXT_EVENT:
                break;
            case EM_EVENT_KEY_STROKE:
                kprintf("key=%c", msg->key);
                // if got key stroke
                break;
            case EM_EVENT_UDP_PACKET_RECEIVED:
                kprintf("New packet received");
                break;
            default:
                panic("UNKNOWN MESSAGE RECIEVED");
                break;
        }
    }
}

void init_em() {
    em_port = create_process(em_process, 6, 0, "EM process");
    resign();
}

void init_em_tester() {
    em_test_port = create_process(em_test_process, 5, 0, "EM Test process");
    resign();
}