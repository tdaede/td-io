#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <string.h>

const uint PIN_JVS_RE = 2;
const uint PIN_JVS_DE = 3;

const uint PIN_JVS_SENSE_2_5V = 14;
const uint PIN_JVS_SENSE_0V = 13;

const uint PIN_JVS_TERMINATION = 15;
const uint PIN_JVS_SENSE_IN = 26;

const uint PIN_SR_DATA = 6;
const uint PIN_SR_CLK = 5;
const uint PIN_SR_SH = 4;

const uint PIN_METER1 = 8;
const uint PIN_METER2 = 7;
const uint PIN_LOCKOUT1 = 10;
const uint PIN_LOCKOUT2 = 9;

const uint PIN_LED_ENUMERATED = PICO_DEFAULT_LED_PIN;

const uint16_t JVS_TERMINATION_THRESHOLD = (uint16_t)(3.75/2.0/3.3*4096);
const uint16_t JVS_0V_THRESHOLD = (uint16_t)(1.25/2.0/3.3*4096);

const uint8_t JVS_STATUS_GOOD = 1;
const uint8_t JVS_STATUS_UNKNOWN_COMMAND = 2;
const uint8_t JVS_STATUS_CHECKSUM_ERROR = 3;
const uint8_t JVS_STATUS_OVERFLOW = 4;

const uint8_t JVS_REPORT_GOOD = 1;
const uint8_t JVS_REPORT_PARAMETER_INVALID = 3;

const uint8_t JVS_MAX_LEN = 253; // minus two for status and checksum

// global state
uint8_t our_address = 0;
// signed for underflow checks
int16_t coin_count_p1 = 0;
int16_t coin_count_p2 = 0;
uint8_t prev_coin_p1 = 0;
uint8_t prev_coin_p2 = 0;

#define SR_P1_3 7
#define SR_P2_3 6
#define SR_P1_4 5
#define SR_P2_4 4
#define SR_P1_5 3
#define SR_P2_5 2
#define SR_P1_6 1
#define SR_P2_6 0
#define SR_P1_LEFT 15
#define SR_P2_LEFT 14
#define SR_P1_RIGHT 13
#define SR_P2_RIGHT 12
#define SR_P1_1 11
#define SR_P2_1 10
#define SR_P1_2 9
#define SR_P2_2 8
#define SR_C1 23
#define SR_C2 22
#define SR_P1_START 21
#define SR_P2_START 20
#define SR_P1_UP 19
#define SR_P2_UP 18
#define SR_P1_DOWN 17
#define SR_P2_DOWN 16
#define SR_SERVICE 31
#define SR_TEST 30
#define SR_TILT 29

const uint8_t JVS_COMM_VER = 0x10;
const char id_str[] = "TD;TD-IO;v1.0;https://github.com/tdaede/td-io";

const uint8_t input_desc[] = {
    0x01, 2, 12, 0,
    0x02, 2, 0, 0,
    0x00
};

void jvs_sync() {
    uart_putc(uart0, 0xe0);
}

uint8_t jvs_getc() {
    uint8_t c = uart_getc(uart0);
    if (c == 0xd0) {
        c = uart_getc(uart0);
        return c + 1;
    } else {
        return c;
    }
}

void jvs_putc(uint8_t c) {
    if (c == 0xe0) {
        uart_putc(uart0, 0xd0);
        uart_putc(uart0, 0xdf);
    } else if (c == 0xd0) {
        uart_putc(uart0, 0xd0);
        uart_putc(uart0, 0xcf);
    } else {
        uart_putc(uart0, c);
    }
}

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
    jvs_sync();
    uint8_t checksum = 0;
    jvs_putc(0);
    checksum += 0;
    jvs_putc(msg_len + 2);
    checksum += msg_len + 2;
    jvs_putc(status);
    checksum += status;
    for (int i = 0; i < msg_len; i++) {
        jvs_putc(m[i]);
        checksum += m[i];
    }
    jvs_putc(checksum);
    stop_transmit();
}

uint32_t read_switches() {
    uint32_t r;
    gpio_put(PIN_SR_SH, 1);
    busy_wait_us(1);
    for (int i = 0; i < 32; i++) {
        r >>= 1;
        r |= (gpio_get(PIN_SR_DATA) ? 1 : 0) << 31;
        gpio_put(PIN_SR_CLK, 1);
        busy_wait_us(1);
        gpio_put(PIN_SR_CLK, 0);
        busy_wait_us(1);
    }
    gpio_put(PIN_SR_SH, 0);
    return ~r;
}

void update_termination() {
    uint16_t v = adc_read();
    v = 4095; // FIXME: bad adc input
    if (v >= JVS_TERMINATION_THRESHOLD) {
        gpio_put(PIN_JVS_TERMINATION, 0);
    } else {
        gpio_put(PIN_JVS_TERMINATION, 1);
    }
}

// call periodically with switch read to check level changes of coin input
void process_coin(uint32_t switches) {
    if ((switches >> SR_C1 & 1) && !prev_coin_p1) {
        coin_count_p1++;
        if (coin_count_p1 > 16383) {
            coin_count_p1 = 16383;
        }
    }
        if ((switches >> SR_C2 & 1) && !prev_coin_p2) {
        coin_count_p2++;
        if (coin_count_p2 > 16383) {
            coin_count_p2 = 16383;
        }
    }
    prev_coin_p1 = switches >> SR_C1 & 1;
    prev_coin_p2 = switches >> SR_C2 & 1;
    gpio_put(PIN_METER1, prev_coin_p1);
    gpio_put(PIN_METER2, prev_coin_p2);
    gpio_put(PIN_LOCKOUT1, coin_count_p1 >= 16383);
    gpio_put(PIN_LOCKOUT2, coin_count_p2 >= 16383);
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
    gpio_init(PIN_JVS_TERMINATION);
    gpio_put(PIN_JVS_TERMINATION, 1); // disable termination by default
    gpio_set_dir(PIN_JVS_TERMINATION, GPIO_OUT);
    adc_init();
    adc_gpio_init(PIN_JVS_SENSE_IN);
    adc_select_input(0);

    // sr
    gpio_init(PIN_SR_DATA);
    gpio_set_dir(PIN_SR_DATA, GPIO_IN);
    gpio_init(PIN_SR_CLK);
    gpio_put(PIN_SR_CLK, 0);
    gpio_set_dir(PIN_SR_CLK, GPIO_OUT);
    gpio_init(PIN_SR_SH);
    gpio_put(PIN_SR_SH, 0);
    gpio_set_dir(PIN_SR_SH, GPIO_OUT);

    // coin/lockout drivers
    gpio_init(PIN_METER1);
    gpio_put(PIN_METER1, 0);
    gpio_set_dir(PIN_METER1, GPIO_OUT);
    gpio_init(PIN_METER2);
    gpio_put(PIN_METER2, 0);
    gpio_set_dir(PIN_METER2, GPIO_OUT);
    gpio_init(PIN_LOCKOUT1);
    gpio_put(PIN_LOCKOUT1, 0);
    gpio_set_dir(PIN_LOCKOUT1, GPIO_OUT);
    gpio_init(PIN_LOCKOUT2);
    gpio_put(PIN_LOCKOUT2, 0);
    gpio_set_dir(PIN_LOCKOUT2, GPIO_OUT);

    // ui
    gpio_init(PIN_LED_ENUMERATED);
    gpio_put(PIN_LED_ENUMERATED, 0);
    gpio_set_dir(PIN_LED_ENUMERATED, GPIO_OUT);

    update_termination();

    while (true) {
        uint8_t sync = uart_getc(uart0);
        if (sync == 0xe0) {
            uint8_t our_checksum = 0;
            uint8_t node_num = jvs_getc();
            if (!((node_num == 0xff) || ((our_address == node_num) && (our_address != 0)))) {
                continue;
            }
            our_checksum += node_num;
            uint8_t length = jvs_getc();
            our_checksum += length;
            uint8_t msg_length = length - 1;
            uint8_t message[256];
            for (int i = 0; i < msg_length; i++) {
                uint8_t c = jvs_getc();
                our_checksum += c;
                message[i] = c;
            }
            uint8_t msg_send[256*2]; // a few bytes to spare for easier overflow checking
            int o = 0;
            uint8_t status = JVS_STATUS_GOOD;
            uint8_t their_checksum = jvs_getc();
            if (our_checksum == their_checksum) {
                int i = 0;
                while (i < msg_length) {
                    if ((msg_length - i) >= 2 && message[i] == 0xf0 && message[i+1] == 0xd9) {
                        printf("N: %02x Reset\n", node_num);
                        our_address = 0;
                        gpio_put(PIN_JVS_SENSE_0V, 0);
                        gpio_put(PIN_LED_ENUMERATED, 0);
                        i += 2;
                    } else if ((msg_length - i) >= 2 && message[i] == 0xf1) {
                        uint8_t node_id = message[i+1];
                        i += 2;
                        printf("Assign node id N: %02x\n", node_id);
                        uint16_t v = adc_read();
                        printf("Reading ADC: %d\n", v);
                        v = 4095; // FIXME: adc circuit is bad
                        if ((v >= JVS_TERMINATION_THRESHOLD) || (v < JVS_0V_THRESHOLD)) {
                            printf("Assigning our address\n");
                            our_address = node_id;
                            msg_send[o] = JVS_REPORT_GOOD;
                            o++;
                            gpio_put(PIN_JVS_SENSE_0V, 1);
                            gpio_put(PIN_LED_ENUMERATED, 1);
                        } else {
                            printf("We are not currently last in the chain, skipping assignment\n");
                        }
                    } else if ((msg_length - i) >= 1 && message[i] == 0x10) {
                        i++;
                        printf("Got ID code request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        strcpy((char*)&msg_send[o], id_str);
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
                        printf("Got comm revision request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        msg_send[o] = JVS_COMM_VER;
                        o++;
                    } else if ((msg_length - i) >= 1 && message[i] == 0x14) {
                        i++;
                        printf("Got input descriptor request\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        memcpy(&msg_send[o], input_desc, sizeof(input_desc));
                        o += sizeof(input_desc);
                    } else if ((msg_length - i) >= 1 && message[i] == 0x13) {
                        i++;
                        printf("Got main board ID: ");
                        while (i < msg_length) {
                            char c = message[i];
                            i++;
                            if (c == 0) break;
                            putchar(message[i]);
                        }
                        printf("\n");
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                    } else if ((msg_length - i) >= 3 && message[i] == 0x20) {
                        int num_players = message[i+1];
                        int bytes_per_player = message[i+2];
                        i += 3;
                        if (o + num_players*bytes_per_player >= JVS_MAX_LEN) {
                            printf("JVS response overflow!\n");
                            status = JVS_STATUS_OVERFLOW;
                            break;
                        }
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        uint32_t switches = read_switches();
                        process_coin(switches);
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
                        if (o + slots*2 >= JVS_MAX_LEN) {
                            printf("JVS response overflow!\n");
                            status = JVS_STATUS_OVERFLOW;
                            break;
                        }
                        msg_send[o] = JVS_REPORT_GOOD;
                        o++;
                        //printf("Got coin slot request for %02x slots\n", slots);
                        for (int slot = 0; slot < slots; slot++) {
                            if (slot == 0) {
                                msg_send[o] = coin_count_p1 >> 8;
                                msg_send[o+1] = coin_count_p1 & 0xFF;
                            } else if (slot == 1) {
                                msg_send[o] = coin_count_p2 >> 8;
                                msg_send[o+1] = coin_count_p2 & 0xFF;
                            } else {
                                msg_send[o] = 0x80;
                                msg_send[o+1] = 0x00;
                            }
                            o += 2;
                        }
                    } else if ((msg_length - i) >= 4 && message[i] == 0x30) {
                        uint8_t slot = message[i+1];
                        uint16_t amount = (message[i+2] << 8) + message[i+3];
                        i += 4;
                        printf("Decrement coin counter %d by %d\n", (int)slot, (int)amount);
                        if (amount > 16383) {
                            amount = 16383;
                            printf("Capped to 16383\n");
                        }
                        if (slot == 1) {
                            msg_send[o] = JVS_REPORT_GOOD;
                            coin_count_p1 -= amount;
                            if (coin_count_p1 < 0) {
                                coin_count_p1 = 0;
                            }
                        } else if (slot == 2) {
                            msg_send[o] = JVS_REPORT_GOOD;
                            coin_count_p2 -= amount;
                            if (coin_count_p2 < 0) {
                                coin_count_p2 = 0;
                            }
                        } else {
                            printf("Invalid coin counter slot\n");
                            msg_send[o] = JVS_REPORT_PARAMETER_INVALID;
                        }
                        o++;
                    } else if ((msg_length - i) >= 4 && message[i] == 0x35) {
                        uint8_t slot = message[i+1];
                        uint16_t amount = (message[i+2] << 8) + message[i+3];
                        i += 4;
                        printf("Increment coin counter %d by %d\n", (int)slot, (int)amount);
                        if (amount > 16383) {
                            amount = 16383;
                            printf("Capped to 16383\n");
                        }
                        if (slot == 1) {
                            msg_send[o] = JVS_REPORT_GOOD;
                            coin_count_p1 += amount;
                            if (coin_count_p1 > 16383) {
                                coin_count_p1 = 16383;
                            }
                        } else if (slot == 2) {
                            msg_send[o] = JVS_REPORT_GOOD;
                            coin_count_p2 += amount;
                            if (coin_count_p2 > 16383) {
                                coin_count_p2 = 16383;
                            }
                        } else {
                            printf("Invalid coin counter slot\n");
                            msg_send[o] = JVS_REPORT_PARAMETER_INVALID;
                        }
                        o++;
                    } else {
                        printf("Unsupported message: N: %02x L: %02x M: ", node_num, msg_length);
                        for (int j = 0; j < msg_length; j++) {
                            printf("%02x", message[j]);
                        }
                        printf("\n");
                        status = JVS_STATUS_UNKNOWN_COMMAND;
                        break;
                    }
                    if (o >= JVS_MAX_LEN) {
                        printf("JVS response overflow!\n");
                        status = JVS_STATUS_OVERFLOW;
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
            // note because of disabling the rx, we always read a 00 byte after sending here
            update_termination(); // convenient time to read adc
            //printf("Saw non-sync code %02x\n", sync);
        }
    }
    return 0;
}
