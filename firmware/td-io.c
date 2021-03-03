#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>

const uint PIN_JVS_RE = 2;
const uint PIN_JVS_DE = 3;

const uint PIN_JVS_SENSE_2_5V = 14;
const uint PIN_JVS_SENSE_0V = 13;

const uint PIN_SR_DATA = 6;
const uint PIN_SR_CLK = 5;
const uint PIN_SR_SH = 4;

const uint8_t JVS_STATUS_GOOD = 1;
const uint8_t JVS_STATUS_UNKNOWN_COMMAND = 2;
const uint8_t JVS_STATUS_CHECKSUM_ERROR = 3;

const uint8_t JVS_REPORT_GOOD = 1;

const uint8_t JVS_MAX_LEN = 253; // minus two for status and checksum

uint8_t our_address = 0;

#define SR_P1_3 0
#define SR_P2_3 1
#define SR_P1_4 2
#define SR_P2_4 3
#define SR_P1_5 4
#define SR_P2_5 5
#define SR_P1_6 6
#define SR_P2_6 7
#define SR_P1_LEFT 8
#define SR_P2_LEFT 9
#define SR_P1_RIGHT 10
#define SR_P2_RIGHT 11
#define SR_P1_1 12
#define SR_P2_1 13
#define SR_P1_2 14
#define SR_P2_2 15
#define SR_C1 16
#define SR_C2 17
#define SR_P1_START 18
#define SR_P2_START 19
#define SR_P1_UP 20
#define SR_P2_UP 21
#define SR_P1_DOWN 22
#define SR_P2_DOWN 23
#define SR_SERVICE 24
#define SR_TEST 25
#define SR_TILT 26

const char id_str[] = "TD;TD-IO;v1.0;https://github.com/tdaede/td-io";

const uint8_t input_desc[] = {
    0x01, 2, 12, 0,
    0x02, 2, 0, 0,
    0x00
};

void start_transmit() {
    gpio_put(PIN_JVS_RE, 1); // disable receive
    gpio_put(PIN_JVS_DE, 1); // enable transmitter
}

void stop_transmit() {
    uart_tx_wait_blocking(uart0);
    gpio_put(PIN_JVS_DE, 0); // disable transmitter
    gpio_put(PIN_JVS_RE, 0); // enable receiver
}

void send_message(uint8_t status, uint8_t* m, uint8_t msg_len) {
    start_transmit();
    uart_putc(uart0, 0xe0);
    uint8_t checksum = 0;
    uart_putc(uart0, 0);
    checksum += 0;
    uart_putc(uart0, msg_len + 2);
    checksum += msg_len + 2;
    uart_putc(uart0, status);
    checksum += status;
    for (int i = 0; i < msg_len; i++) {
        uart_putc(uart0, m[i]);
        checksum += m[i];
    }
    uart_putc(uart0, checksum);
    stop_transmit();
}

uint32_t read_switches() {
    uint32_t r;
    gpio_put(PIN_SR_SH, 1);
    busy_wait_us(1);
    for (int i = 0; i < 32; i++) {
        r <<= 1;
        r |= gpio_get(PIN_SR_DATA);
        gpio_put(PIN_SR_CLK, 1);
        busy_wait_us(1);
        gpio_put(PIN_SR_CLK, 0);
        busy_wait_us(1);
    }
    gpio_put(PIN_SR_SH, 0);
    return ~r;
}

int main() {
    stdio_init_all();
    uart_init(uart0, 115200);
    uart_set_translate_crlf(uart0, false);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    gpio_init(PIN_JVS_RE);
    gpio_put(PIN_JVS_RE, 0); // enable receiver
    gpio_set_dir(PIN_JVS_RE, GPIO_OUT);
    gpio_init(PIN_JVS_DE);
    gpio_put(PIN_JVS_DE, 0); //disable transmitter
    gpio_set_dir(PIN_JVS_DE, GPIO_OUT);
    gpio_init(PIN_JVS_SENSE_2_5V);
    gpio_put(PIN_JVS_SENSE_2_5V, 1); // always appear present
    gpio_set_dir(PIN_JVS_SENSE_2_5V, GPIO_OUT);
    gpio_init(PIN_JVS_SENSE_0V);
    gpio_put(PIN_JVS_SENSE_0V, 0);
    gpio_set_dir(PIN_JVS_SENSE_0V, GPIO_OUT);

    // sr
    gpio_init(PIN_SR_DATA);
    gpio_set_dir(PIN_SR_DATA, GPIO_IN);
    gpio_init(PIN_SR_CLK);
    gpio_put(PIN_SR_CLK, 0);
    gpio_set_dir(PIN_SR_CLK, GPIO_OUT);
    gpio_init(PIN_SR_SH);
    gpio_put(PIN_SR_SH, 0);
    gpio_set_dir(PIN_SR_SH, GPIO_OUT);

    while (true) {
        uint8_t sync = uart_getc(uart0);
        if (sync == 0xe0) {
            uint8_t our_checksum = 0;
            uint8_t node_num = uart_getc(uart0);
            our_checksum += node_num;
            uint8_t length = uart_getc(uart0);
            our_checksum += length;
            uint8_t msg_length = length - 1;
            uint8_t message[256];
            for (int i = 0; i < msg_length; i++) {
                uint8_t c = uart_getc(uart0);
                our_checksum += c;
                message[i] = c;
            }
            uint8_t msg_send[256];
            int o = 0;
            uint8_t status = JVS_STATUS_GOOD;
            uint8_t their_checksum = uart_getc(uart0);
            if (our_checksum == their_checksum) {
                int i = 0;
                while (i < msg_length) {
                    if ((msg_length - i) >= 2 && message[i] == 0xf0 && message[i+1] == 0xd9) {
                        printf("N: %02x Reset\n", node_num);
                        our_address = 0;
                        gpio_put(PIN_JVS_SENSE_0V, 0);
                        i += 2;
                    } else if ((msg_length - i) >= 2 && message[i] == 0xf1) {
                        printf("Assign node id N: %02x\n", message[i+1]);
                        printf("Assume it is us\n");
                        our_address = message[i+1];
                        i += 2;
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        gpio_put(PIN_JVS_SENSE_0V, 1);
                    } else if ((msg_length - i) >= 1 && message[i] == 0x10) {
                        i++;
                        printf("Got ID code request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        strcpy(&msg_send[o], id_str);
                        o += strlen(id_str) + 1;
                    } else if ((msg_length - i) >= 1 && message[i] == 0x11) {
                        i++;
                        printf("Got revision request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        msg_send[o] = 0x13;
                        o++;
                    } else if ((msg_length - i) >= 1 && message[i] == 0x12) {
                        i++;
                        printf("Got video revision request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        msg_send[o] = 0x30;
                        o++;
                    } else if ((msg_length - i) >= 1 && message[i] == 0x13) {
                        i++;
                        printf("Got io revision request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        msg_send[o] = 0x10;
                        o++;
                    } else if ((msg_length - i) >= 1 && message[i] == 0x14) {
                        i++;
                        printf("Got input descriptor request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        memcpy(&msg_send[o], input_desc, sizeof(input_desc));
                        o += sizeof(input_desc);
                    } else if ((msg_length - i) >= 3 && message[i] == 0x20) {
                        int num_players = message[i+1];
                        int bytes_per_player = message[i+2];
                        i += 3;
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        uint32_t switches = read_switches();
                        msg_send[o] = ((switches >> SR_TEST) & 1) << 7
                            | ((switches >> SR_TILT) & 1) << 7;
                        o++;
                        //printf("Got coin slot request for %02x slots\n", slots);
                        for (int player = 0; player < num_players; player++) {
                            for (int byte = 0; byte < bytes_per_player; byte++) {
                                uint8_t b = 0;
                                if ((player == 0) && (byte == 0)) {
                                    b = ((switches >> SR_P1_2) & 1) << 0
                                        | ((switches >> SR_P1_1) & 1) << 1
                                        | ((switches >> SR_P1_RIGHT) & 1) << 2
                                        | ((switches >> SR_P1_LEFT) & 1) << 3
                                        | ((switches >> SR_P1_DOWN) & 1) << 4
                                        | ((switches >> SR_P1_UP) & 1) << 5
                                        | ((switches >> SR_SERVICE) & 1) << 6
                                        | ((switches >> SR_P1_START) & 1) << 7;
                                } else if ((player == 0) && (byte == 1)) {
                                    b = ((switches >> SR_P1_6) & 1) << 4
                                        | ((switches >> SR_P1_5) & 1) << 5
                                        | ((switches >> SR_P1_4) & 1) << 6
                                        | ((switches >> SR_P1_3) & 1) << 7;
                                } else if ((player == 1) && (byte == 0)) {
                                    b = ((switches >> SR_P2_2) & 1) << 0
                                        | ((switches >> SR_P2_1) & 1) << 1
                                        | ((switches >> SR_P2_RIGHT) & 1) << 2
                                        | ((switches >> SR_P2_LEFT) & 1) << 3
                                        | ((switches >> SR_P2_DOWN) & 1) << 4
                                        | ((switches >> SR_P2_UP) & 1) << 5
                                        | ((switches >> SR_P2_START) & 1) << 7;
                                } else if ((player == 1) && (byte == 1)) {
                                    b = ((switches >> SR_P2_6) & 1) << 4
                                        | ((switches >> SR_P2_5) & 1) << 5
                                        | ((switches >> SR_P2_4) & 1) << 6
                                        | ((switches >> SR_P2_3) & 1) << 7;
                                }
                                msg_send[o] = b;
                                o++;
                            }
                        }
                    } else if ((msg_length - i) >= 2 && message[i] == 0x21) {
                        int slots = message[i+1];
                        i += 2;
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        //printf("Got coin slot request for %02x slots\n", slots);
                        for (int slot = 0; slot < slots; slot++) {
                            msg_send[o] = 0x00;
                            msg_send[o+1] = 0x00;
                            o += 2;
                        }
                    } else {
                        printf("Unsupported message: N: %02x L: %02x M: ", node_num, msg_length);
                        for (int j = 0; j < msg_length; j++) {
                            printf("%02x", message[j]);
                        }
                        printf("\n");
                        status = JVS_STATUS_UNKNOWN_COMMAND;
                        break;
                    }
                }
                if ((o > 0) || (status != JVS_STATUS_GOOD)) {
                    send_message(status, msg_send, o);
                }
            } else {
                printf("Checksum mismatch: theirs: %02x ours: %02x\n", their_checksum, our_checksum);
                status = JVS_STATUS_CHECKSUM_ERROR;
                send_message(status, msg_send, 0);
            }
        } else {
            //printf("Saw non-sync code %02x\n", sync);
        }
    }
    return 0;
}
