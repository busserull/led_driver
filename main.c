#include "ubit.h"
#include "bluetooth.h"

int main(){
    ubit_uart_init();
    ubit_led_matrix_init();

    uint32_t error = 0;

    ubit_led_matrix_turn_on();

    error = bluetooth_init();
    ubit_uart_print("Init: %d\n\r", error);

    error = bluetooth_gap_advertise_start();
    ubit_uart_print("GAP: %d\n\r", error);

    error = bluetooth_gatts_start();
    ubit_uart_print("GATT: %d\n\r", error);

    bluetooth_serve_forever();

	return 0;
}
