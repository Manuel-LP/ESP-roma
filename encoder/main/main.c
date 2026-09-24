#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"

// Pines de conexión (Asegurate de usar el divisor resistivo a 3.3V)
#define ENCODER_PIN_A 17
#define ENCODER_PIN_B 16

// Límites del hardware del PCNT (máximo de 16 bits)
#define PCNT_HIGH_LIMIT 100
#define PCNT_LOW_LIMIT  -100

static const char *TAG = "MOTOR_PID";

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando hardware PCNT en modo 2X...");

    // 1. Configurar la unidad principal
    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // 2. Configurar SOLO el Canal A para modo 2X
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ENCODER_PIN_A,   // Pin que dispara la interrupción
        .level_gpio_num = ENCODER_PIN_B,  // Pin que indica la dirección
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    // 3. Configurar acciones lógicas de los flancos
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, 
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,  // Al bajar la señal de A
        PCNT_CHANNEL_EDGE_ACTION_INCREASE   // Al subir la señal de A
    ));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, 
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,     // Si B es ALTO, mantiene sentido
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE   // Si B es BAJO, invierte cuenta
    ));

    // 4. Habilitar, limpiar y arrancar el módulo
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    ESP_LOGI(TAG, "Encoder listo. Iniciando lazo de control...");

    int pulse_count = 0;
    float rpm = 0.0;
    
    // Constante precalculada para la matemática del encoder
    const float RPM_MULTIPLIER = 0.3;

    while (1) {
        // 1. Leer pulsos acumulados en esta ventana de tiempo
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
        
        // 2. Reiniciar el hardware inmediatamente para la próxima ventana
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
        
        // 3. Calcular RPM
        rpm = (float)pulse_count * RPM_MULTIPLIER;
        
        // 4. Imprimir en consola (Luego acá mandaremos el dato a ROS 2)
        ESP_LOGI(TAG, "Pulsos (100ms): %d | Velocidad: %.1f RPM", pulse_count, rpm);
        
        // 5. Ventana de tiempo estricta de 100ms usando la macro segura
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}