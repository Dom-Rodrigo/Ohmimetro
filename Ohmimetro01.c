/*
 * Por: Wilton Lacerda Silva
 *    Ohmímetro utilizando o ADC da BitDogLab
 *
 * 
 * Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 * desconhecido, utilizando um divisor de tensão com dois resistores.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "math.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A

int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.27;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main()
{
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_t ssd;                                                // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string

  bool cor = true;

  float e24[24] = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
  char cores[10][3] = {"PRT", "MAR", "VEM", "LAR", "AMA", "VED", "AZU", "VIO", "CIN", "BRA"};
  char faixa_1[3];
  char faixa_2[3];
  char faixa_3[3];
  double frac_part;
  double integer_part;
  int indice_mais_prox = 0;
  float menor_distancia = 1000;
  while (true)
  {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

      // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = ((R_conhecido * media) / (ADC_RESOLUTION - media))-55; // EM média os valores estavam dando 55 unidades a mais, por isso o -55.

    int zeros_multiplicador = 0;
    while (R_x >= 9.1){
      R_x = R_x / 10;
      zeros_multiplicador++;
    }


    for(int i =0; i < 24; i++){

      if (e24[i] <= R_x*1.05){ // 5% de tolerancia
        if ((R_x - e24[i])<= menor_distancia){
          menor_distancia = R_x - e24[i];
          indice_mais_prox = i;
        }
      }

    }
    R_x = e24[indice_mais_prox];

    frac_part = modf(R_x, &integer_part);
    frac_part = round(frac_part*10);
    for (int i =0; i < 3; i++){
      faixa_2[i] = cores[(int)(frac_part)][i];
      
    }

    for (int i =0; i < 3; i++){
      faixa_1[i] = cores[(int)(round(integer_part))][i];
    }


    

    R_x = R_x * (pow(10, zeros_multiplicador));

    for (int i =0; i < 3; i++){
      faixa_3[i] = cores[zeros_multiplicador-1][i];
    }
    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string

    menor_distancia=1000;
    indice_mais_prox=0;
    zeros_multiplicador=0; // Coloca as variaveis no padrao pra caso outro resistor tenha sido inserido

    // cor = !cor;
    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
    ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
    ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
    // ssd1306_draw_string(&ssd, faixa_3, 53, 16);  // Desenha uma string
    ssd1306_draw_char(&ssd, faixa_3[0], 80, 16);
    ssd1306_draw_char(&ssd, faixa_3[1], 88, 16);
    ssd1306_draw_char(&ssd, faixa_3[2], 96, 16);
    
    ssd1306_draw_char(&ssd, faixa_2[0], 48, 16);
    ssd1306_draw_char(&ssd, faixa_2[1], 56, 16);
    ssd1306_draw_char(&ssd, faixa_2[2], 64, 16);

        
    ssd1306_draw_char(&ssd, faixa_1[0], 16, 16);
    ssd1306_draw_char(&ssd, faixa_1[1], 24, 16);
    ssd1306_draw_char(&ssd, faixa_1[2], 32, 16);
     // NÂO FUNCIONA O DRAW STRING!!

    // ssd1306_draw_string(&ssd, faixa_1, 13, 16);  // Desenha uma string
    // ssd1306_draw_string(&ssd, faixa_2, 13, 16);  // Desenha uma string

    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(800);
  }
}