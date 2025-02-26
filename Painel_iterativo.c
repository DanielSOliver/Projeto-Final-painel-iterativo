#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "animacoes_led.pio.h"


const uint BOTAO_A = 5;
const uint BOTAO_B = 6;
const uint LED_VERMELHO = 13;
const uint LED_VERDE = 11;
const uint JOYSTICK_X_PIN = 27;
const uint JOYSTICK_Y_PIN = 26;  

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define OUT_PIN 7
#define NUM_PIXELS 25 // Número de LEDs na matriz
#define BUZZER_PIN 21  // Pino do buzzer 

PIO pio;
uint sm;
static volatile uint32_t last_time = 0;
bool Led_Estado = false;
bool Estado_BUZZ = false;


/// Função para configurar e tocar o buzzer em uma frequência e duração específicas
void buzzer_tocar(uint32_t frequencia, uint32_t duracao) {
    // Configura o pino como saída PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    
    // Define a frequência do PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN); 
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t wrap_val = clock_freq / frequencia;
    pwm_set_wrap(slice_num, wrap_val);  // Define o valor do "wrap" para a frequência
    pwm_set_gpio_level(BUZZER_PIN, wrap_val / 2); // Define o ciclo de trabalho (50%)

    // Inicia o PWM
    pwm_set_enabled(slice_num, true);
    
    // Aguarda a duração do som
    sleep_ms(duracao);
    
    // Desativa o PWM para parar o som
    pwm_set_enabled(slice_num, false);
}


// Vetores de imagens – valores de 0 a 1 (representam intensidade)
double apagado[25] = {0.0, 0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0, 0.0, 
                      0.0, 0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0, 0.0};

double carinha_triste[25] = {0.1, 0.1, 0.0, 0.1, 0.1,
                             0.1, 0.1, 0.0, 0.1, 0.1, 
                             0.0, 0.0, 0.0, 0.1, 0.0,
                             0.0, 0.1, 0.1, 0.1, 0.0,
                             0.1, 0.0, 0.0, 0.0, 0.1};

double carinha_raiva[25] = {0.1, 0.0, 0.0, 0.0, 0.1,
                            0.1, 0.1, 0.0, 0.1, 0.1,
                            0.0, 0.0, 0.0, 0.0, 0.0,
                            0.0, 0.1, 0.1, 0.1, 0.0,
                            0.1, 0.0, 0.0, 0.0, 0.1};

double carinha_ansioso[25] = {0.1, 0.1, 0.0, 0.1, 0.1,
                              0.1, 0.1, 0.0, 0.1, 0.1,
                              0.0, 0.0, 0.1, 0.0, 0.0,
                              0.1, 0.1, 0.1, 0.1, 0.1,
                              0.1, 0.1, 0.1, 0.1, 0.1};

double carinha_alegre[25] = {0.1, 0.1, 0.0, 0.1, 0.1,
                             0.1, 0.1, 0.0, 0.1, 0.1,
                             0.0, 0.0, 0.1, 0.0, 0.0,
                             0.1, 0.1, 0.1, 0.1, 0.1,
                             0.0, 0.1, 0.1, 0.1, 0.0};



// Função que converte valores de 0 a 1 em uma cor RGB de 32 bits
uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R = r * 255;
    unsigned char G = g * 255;
    unsigned char B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// Função para acionar a matriz de LEDs (WS2812B) via PIO
void desenho_pio(double *desenho, int cor) {
    uint32_t valor_led;
    if (cor == 1) { // Liga todos os LEDs na cor vermelha
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(desenho[24 - i], 0.0, 0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    } else if (cor == 2) { // Liga todos os LEDs na cor amarela
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(desenho[24 - i], desenho[24 - i], 0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    } else if (cor == 3) { // Liga todos os LEDs na cor verde
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, desenho[24 - i], 0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    } else if (cor == 4) { // Liga todos os LEDs na cor azul
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, 0.0, desenho[24 - i]);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    } else if (cor == 5) { // Desliga todos os LEDs
        for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(0.0, 0.0, 0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    }

}

// Função para tocar uma melodia
void tocar_melodia(const char* emocao) {
    if (strcmp(emocao, "feliz") == 0) {
        // Melodia alegre (frequências mais altas e rápidas)
        buzzer_tocar(730, 200); 
        sleep_ms(200);
        buzzer_tocar(760, 200); 
        sleep_ms(200);
        buzzer_tocar(800, 200); 
        sleep_ms(200);
        buzzer_tocar(840, 200); 
      
    } else if (strcmp(emocao, "triste") == 0) {
        // Melodia triste (frequências baixas e longas)
        buzzer_tocar(410, 500); 
        sleep_ms(200);
        buzzer_tocar(400, 500); 
        sleep_ms(200);
        buzzer_tocar(396, 500); 
        sleep_ms(200);
        buzzer_tocar(494, 500); 
        sleep_ms(200);
       
    } else if (strcmp(emocao, "bravo") == 0) {
        // Melodia agressiva (notas rápidas e fortes)
        buzzer_tocar(800, 100);
        sleep_ms(100);
        buzzer_tocar(700, 100); 
        sleep_ms(100);
        buzzer_tocar(800, 100); 
        sleep_ms(100);
        buzzer_tocar(700, 100); 
        sleep_ms(100);
        
    } else if (strcmp(emocao, "ansioso") == 0) {
        // Melodia ansiosa (curtas variações de notas)
        buzzer_tocar(600, 150); // Dó# (600 Hz)
        sleep_ms(150);
        buzzer_tocar(700, 150); // Ré# (700 Hz)
        sleep_ms(150);
        buzzer_tocar(600, 150); // Dó# (600 Hz)
        sleep_ms(150);
        buzzer_tocar(800, 150); // Fá (800 Hz)
        sleep_ms(150);
    }
}

void debounce_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 300000) {  // Debounce de 300ms
        last_time = current_time;
        Led_Estado= !Led_Estado;

        if (gpio == BOTAO_A) {
            gpio_put(LED_VERMELHO, Led_Estado);  
            gpio_put(LED_VERDE, Led_Estado); 
            printf(" Sua criança solicitou ajuda com sua alimentação!\n\n");
            
        }
       if (gpio == BOTAO_B) {
            gpio_put(LED_VERMELHO, Led_Estado); 
            Estado_BUZZ= !Estado_BUZZ;
            printf(" Sua criança se queixou de dores. Recomendamos cuidados médicos!\n\n");
        } 
    }

}


void indicador_emocoes(ssd1306_t *ssd, uint16_t adc_x, uint16_t adc_y) {
    
    if (adc_x < 500) {
        ssd1306_fill(ssd, false); // Limpa o display
        desenho_pio(carinha_alegre, 2);
        ssd1306_rect(ssd, 3, 3, 122, 60, true, false);
        ssd1306_draw_string(ssd, "FELIZ", 49, 45);
        ssd1306_send_data(ssd);
        printf(" Sua criança está se sentindo Feliz!\n\n");
        tocar_melodia("feliz");
        desenho_pio(apagado, 5);
    } else if (adc_y > 4000) {
        ssd1306_fill(ssd, false); // Limpa o display
        desenho_pio(carinha_triste, 4);
        ssd1306_rect(ssd, 3, 3, 122, 60, true, false);
        ssd1306_draw_string(ssd, "TRISTE", 75, 30);
        ssd1306_send_data(ssd);
        printf(" Sua criança está se sentindo Triste!\n\n");
        tocar_melodia("triste");
        desenho_pio(apagado, 5);
    } else if (adc_x > 4000) {
        ssd1306_fill(ssd, false); // Limpa o display
        desenho_pio(carinha_raiva, 1);
        ssd1306_rect(ssd, 3, 3, 122, 60, true, false);
        ssd1306_draw_string(ssd, "BRAVO", 49, 10);
        ssd1306_send_data(ssd);
        printf(" Sua criança está se sentindo Brava!\n\n");
        tocar_melodia("bravo");
        desenho_pio(apagado, 5);
    } else if (adc_y < 500) {
        ssd1306_fill(ssd, false); // Limpa o display
        desenho_pio(carinha_ansioso, 3);
        ssd1306_rect(ssd, 3, 3, 122, 60, true, false);
        ssd1306_draw_string(ssd, "ANSIOSO", 8, 30);
        ssd1306_send_data(ssd);
        printf(" Sua criança está se sentindo Ansiosa!\n\n");
        tocar_melodia("ansioso");
        desenho_pio(apagado, 5);
    }
}



int main() {
    stdio_init_all();
    pio = pio0;
   
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    // Configurações da PIO para animações dos LEDs
    uint offset = pio_add_program(pio, &animacoes_led_program);
    sm = pio_claim_unused_sm(pio, true); // Usa a variável global 'sm'
    animacoes_led_program_init(pio, sm, offset, OUT_PIN);
  
    // Configuração do I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, 128, 64, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd); // Configura o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    
    
    // Configuração do ADC (joystick)
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    

    // Configuração dos botões com pull-up
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

     // Configuração do LED verde
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0); 

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0); 

    


    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &debounce_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &debounce_irq_handler);
 
    
    while (true) {
        // Atualiza as leituras do ADC a cada loop
        adc_select_input(0);
        uint16_t adc_x = adc_read();
        adc_select_input(1);
        uint16_t adc_y = adc_read();

        ssd1306_fill(&ssd, false); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
        ssd1306_draw_string(&ssd, "Mostre Como Se", 8, 20);       // Primeira linha, centrado
        ssd1306_draw_string(&ssd, "Sente Hoje", 20, 35);       // Segunda linha, centrado
        ssd1306_send_data(&ssd);  // Envia os dados para o display


        indicador_emocoes(&ssd ,adc_x, adc_y);
        
        if (Estado_BUZZ== true) {
            // Sirene (oscila entre frequências altas e baixas)
            for (int i = 0; i < 5; i++) {  // Repete a oscilação 5 vezes
                buzzer_tocar(800, 300); // Frequência alta (1 kHz)
                sleep_ms(100);
                buzzer_tocar(400, 300);  // Frequência baixa (500 Hz)
                sleep_ms(100);
            }
        }
        
        sleep_ms(100);
    }
}
