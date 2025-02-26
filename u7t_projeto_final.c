#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h" // para microfone
#include "hardware/gpio.h"

// Para LCD DISPLAY
#include "hardware/i2c.h"
#include "libs/ssd1306.h"
#include "libs/font.h"


// MIC
#define ADC_NUM 2 // Canal do ADC
#define ADC_PIN (28 + ADC_NUM) // Pino 26 + 0 = 26
#define ADC_VREF 3.3 // Especifica a tensão de referência do ADC.
#define ADC_RANGE (1 << 12) // equivale a 1 deslocado 12 vezes para esquerda o mesmo que 4096
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1)) // usada para converter o valor bruto do ADC (0 a 4095) em uma tensão real (0V a 3.3V)
#define ADC_FLOAT_CONVERT (2.0 / (ADC_RANGE - 1)) // razão entre 2.5 e o valor máximo do ADC

// CONFIGURAÇÃO DO LED (ALERTA DE SOM)
#define LED_RED_PIN 13
#define LED_BLUE_PIN 12
#define LED_GREEN_PIN 11

// Display
#define I2C_PORT i2c1 // específica o barramento I2C utilizado na comunicação com o display
#define I2C_SDA 14 // GPIO usada para o sinal SDA (Serial Data) do barramento I2C
#define I2C_SCL 15 // GPIO que será usado para o sinal SCL (Serial Clock) do barramento I2C
#define endereco 0x3C // 0x3C é usado para identificar o display OLED no barramento I2C
ssd1306_t ssd; // Inicializa a estrutura do display

// Buffer de amostras do microfone
uint8_t waveform[WIDTH];

int main()
{
    stdio_init_all();

    printf("🎙️ Monitoramento de Microfone ...\n");

    // Inicia os GPIOs para o LED
    gpio_init(LED_RED_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(LED_GREEN_PIN);
    // Seta a direção do pino para saída
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    // Inicia cada pino em nível baixo (desligado)
    gpio_put(LED_RED_PIN, false);
    gpio_put(LED_BLUE_PIN, false);
    gpio_put(LED_GREEN_PIN, false);
  
    adc_init(); // Inicializa o hardware do ADC
    adc_gpio_init(ADC_PIN); // Configura o pino GPIO especificado como uma entrada analógica
    adc_select_input(ADC_NUM);
    sleep_ms(1000);
  
    uint captura;
    float adc_value;
  
     // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);
  
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
  
    bool detected = false;
  
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 8); // Desenha uma string
    ssd1306_draw_string(&ssd, "TIC370100931", 16, 28); // Desenha uma string
    ssd1306_draw_string(&ssd, "JONATAS SILVA", 12, 48); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display
    printf("📢 Esperando Entrada!\n");
    printf("👀 Captura de SOM ATIVADA!\n");

    while (true) {
        captura = adc_read(); // raw voltage from ADC
        adc_value = captura * ADC_FLOAT_CONVERT;
        // Agudos
        if ((captura * ADC_FLOAT_CONVERT) > 1.2) {
            gpio_put(LED_RED_PIN, true);
            gpio_put(LED_BLUE_PIN, false);
            gpio_put(LED_GREEN_PIN, false);
            detected = true;

            ssd1306_fill(&ssd, false); // Limpa o display
            //ssd1306_draw_string(&ssd, "FOGO", 40, 25); // Exibe "FOGO" no centro
            // Normaliza os valores para caber na tela (entre 0 e 63)
            for (int i = 0; i < WIDTH; i++) {
                uint16_t captura = adc_read();
                float adc_value = captura * ADC_FLOAT_CONVERT;
                
                // Normaliza os valores para caber na tela (entre 0 e 63)
                waveform[i] = (uint8_t)(48 + (adc_value - 1.65) * 20);
            }
            draw_waveform(&ssd, waveform); // Atualiza o gráfico no display
        }
        // medições com baixa variação
        if ((captura * ADC_FLOAT_CONVERT) > 0.90 && (captura * ADC_FLOAT_CONVERT) < 1.10) {
            gpio_put(LED_RED_PIN, false);
            gpio_put(LED_BLUE_PIN, false);
            gpio_put(LED_GREEN_PIN, true);

            if (detected) {
                ssd1306_fill(&ssd, false); // Limpa o display
                //ssd1306_draw_string(&ssd, "FOGO", 40, 25); // Exibe "FOGO" no centro
                // Normaliza os valores para caber na tela (entre 0 e 63)
                for (int i = 0; i < WIDTH; i++) {
                    uint16_t captura = adc_read();
                    float adc_value = captura * ADC_FLOAT_CONVERT;

                    // Normaliza os valores para caber na tela (entre 0 e 63)
                    waveform[i] = (uint8_t)(48 + (adc_value - 1.65) * 20);
                }
                draw_waveform(&ssd, waveform); // Atualiza o gráfico no display
            }
        }
        // Grave
        if ((captura * ADC_FLOAT_CONVERT) < 0.90) {
            gpio_put(LED_RED_PIN, false);
            gpio_put(LED_BLUE_PIN, true);
            gpio_put(LED_GREEN_PIN, false);
            detected = true;

            ssd1306_fill(&ssd, false); // Limpa o display
            //ssd1306_draw_string(&ssd, "FOGO", 40, 25); // Exibe "FOGO" no centro
            // Normaliza os valores para caber na tela (entre 0 e 63)
            for (int i = 0; i < WIDTH; i++) {
                uint16_t captura = adc_read();
                float adc_value = captura * ADC_FLOAT_CONVERT;
                
                // Normaliza os valores para caber na tela (entre 0 e 63)
                waveform[i] = (uint8_t)(48 + (adc_value - 1.65) * 20);
            }
            draw_waveform(&ssd, waveform); // Atualiza o gráfico no display
        }
        sleep_ms(250);
    }

    return 1;
}
