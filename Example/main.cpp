/**
 * @file main.cpp
 * @author Evandro Teixeira
 * @brief 
 * @version 0.1
 * @date 14-02-2022
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <Arduino.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "RTCTiny.hpp"

#define COLOR_BLACK         "\e[0;30m"
#define COLOR_RED           "\e[0;31m"
#define COLOR_GREEN         "\e[0;32m"
#define COLOR_YELLOW        "\e[0;33m"
#define COLOR_BLUE          "\e[0;34m"
#define COLOR_PURPLE        "\e[0;35m"
#define COLOR_CYAN          "\e[0;36m"
#define COLOR_WRITE         "\e[0;37m"
#define COLOR_RESET         "\e[0m"
#define AT24C32_ADDRESS     0x50
#define DS1307_ADDRESS      0x68
#define BUTTON              15    // Pino do botão 
#define LED_BOARD           2     // Pino do LED
#define DEBOUNCE_BUTTON     1000  // Tempo do debounce do botão

/**
 * @brief 
 */
const char MesesDoAno[12][4] = {"Jan","Fev","Mar","Abr","Mai","Jun","Jul","Ago","Set","Out","Nov","Dez"};
const char DiasDaSemana[7][14] = {"Domingo","Segunda-feira","Terça-feira","Quarta-feira","Quinta-feira","Sexta-feira","Sabado"};

/**
 * @brief Cria objeto RTC Tiny
 * 
 * @return RtcTiny 
 */
RtcTiny ModuleRTC(AT24C32_ADDRESS, DS1307_ADDRESS);

/**
 * @brief 
 */
void Tarefa_LED(void *parameters);
void Tarefa_Relogio(void *parameters);
void Tarefa_ContadorPulso(void *parameters);
void SetVarRTC(DS1307Data_t Data);
DS1307Data_t GetVarRTC(void);

/**
 * @brief 
 */
SemaphoreHandle_t xSemaphore_Pulso = NULL;
SemaphoreHandle_t xMutex_I2C = NULL;
SemaphoreHandle_t xMutex_Var = NULL;
DS1307Data_t RTCData;

/**
 * @brief Função da interrupção botão
 */
void IRAM_ATTR Button_ISR()
{
  // tempo da ultima leitura do botão
  static uint32_t last_time = 0; 

  // Algoritmo de debounce do botão
  if( (millis() - last_time) >= DEBOUNCE_BUTTON)
  {
    last_time = millis();
    xSemaphoreGiveFromISR(xSemaphore_Pulso, (BaseType_t)(pdFALSE));
  }
}

/**
 * @brief 
 */
void setup() 
{
  // Inicializa a Serial 
  Serial.begin( 115200 );
  Serial.printf("\n\rFreeRTOS - Mutex\n\r");

  ModuleRTC.Init();

  // Inicializa pino 15 como entra e inicializa interrupção do botão
  pinMode(BUTTON, INPUT);
  attachInterrupt(BUTTON, Button_ISR, RISING);

  // Inicializa pino do LED on Board
  pinMode(LED_BOARD,OUTPUT);
  digitalWrite(LED_BOARD,LOW);

  // Cria semafaro binario xSemaphore_Pulso
  vSemaphoreCreateBinary( xSemaphore_Pulso );
  if(xSemaphore_Pulso == NULL)
  {
    Serial.printf("\n\rFalha em criar o semafaro para Contador Pulso");
  }
  // Obtem o semafaro xSemaphore_Pulso
  xSemaphoreTake(xSemaphore_Pulso,(TickType_t)100);

  // Cria Mutex para gestão do barramento I2C
  xMutex_I2C = xSemaphoreCreateMutex();
  if(xMutex_I2C == NULL)
  {
    Serial.printf("\n\rFalha em criar o Mutex para I2C");
  }

  xMutex_Var = xSemaphoreCreateMutex();
  if(xMutex_Var == NULL)
  {
    Serial.printf("\n\rFalha em criar o Mutex para variavel global");
  }

  // Cria tarefas da aplicação
  xTaskCreate(Tarefa_LED, "LED", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(Tarefa_Relogio, "Relogio", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(Tarefa_ContadorPulso, "Contador Pulso", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 3, NULL);
}

/**
 * @brief 
 */
void loop() 
{
  Serial.printf("\n\rSupende tarefa LOOP");
  vTaskSuspend(NULL);
}

/**
 * @brief 
 * 
 * @param parameters 
 */
void Tarefa_LED(void *parameters)
{
  static int valueOld = 0xFF;
  int value = 0;
  
  while (1)
  {
    // le o valor do botão 
    value = digitalRead(BUTTON);

    // detecta borda de subida
    if((value != valueOld) && (value == HIGH))
    {
      digitalWrite(LED_BOARD,LOW);
      Serial.print(COLOR_BLUE);
      Serial.printf("\n\rLED OFF");
      Serial.print(COLOR_RESET);
    }
    else 
    {
      // detecta borda de descida
      if((value != valueOld) && (value == LOW))
      {
        digitalWrite(LED_BOARD,HIGH);
        Serial.print(COLOR_BLUE);
        Serial.printf("\n\rLED ON");
        Serial.print(COLOR_RESET);
      }
    }
    // Update
    valueOld = value;

    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}

/**
 * @brief 
 * 
 * @param parameters 
 */
void Tarefa_Relogio(void *parameters)
{
  DS1307Data_t Data;

  Data.Month = 2;
  Data.Day = 3;
  Data.Date = 22;
  while (1)
  {
    
    // Obtem o Mutex-I2C
    xSemaphoreTake(xMutex_I2C,portMAX_DELAY );
    // Le os dados do RTC via barramento I2C
    ModuleRTC.ReadRTC(&Data);
    // libera o Mutex-I2C
    xSemaphoreGive(xMutex_I2C);
    

    // salva dados na memoria global
    SetVarRTC(Data);

    // Imprimi no barramento serial Data e Hora
    Serial.printf("\n\r%02d:%02d:%02d - ",Data.Hours,
                                          Data.Minutes,
                                          Data.Seconds);
    Serial.printf("%s, %02d de %s de %d ",DiasDaSemana[ (Data.Day-1) ],
                                          Data.Date ,
                                          MesesDoAno[ (Data.Month-1) ], 
                                          (2000 + Data.Year) ); 
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

/**
 * @brief 
 * 
 * @param parameters 
 */
void Tarefa_ContadorPulso(void *parameters)
{
  const uint16_t endProximo = 0x0000;
  uint16_t contadorPulso = 0;
  uint16_t endMemoriaROM = 0;
  uint8_t buffer[2] = {0};
  DS1307Data_t dataRTC;

  // Obtem o Mutex-I2C
  xSemaphoreTake(xMutex_I2C,portMAX_DELAY );
  // Salva o endereço proximo dado
  ModuleRTC.WriteROM(endProximo, 0x00 );
  ModuleRTC.WriteROM(endProximo + 1, 0x00 );
  // libera o Mutex-I2C
  xSemaphoreGive(xMutex_I2C);

  while (1)
  {
    if(xSemaphoreTake(xSemaphore_Pulso,portMAX_DELAY) == pdTRUE)
    {
      // obtem data e hora 
      dataRTC = GetVarRTC();

      // incrementa o contador de pulso
      contadorPulso++;
      Serial.print(COLOR_YELLOW);
      Serial.printf("\n\rContador de Puslo: %d",contadorPulso);

      // Obtem o Mutex-I2C
      xSemaphoreTake(xMutex_I2C,portMAX_DELAY );

      // Le dados no barramento I2C
      // Obtem o endereço do proximo dado a ser salvo
      ModuleRTC.ReadROM(endProximo, &buffer[0] );
      ModuleRTC.ReadROM(endProximo + 1, &buffer[1] );
      endMemoriaROM  = (uint16_t)(buffer[0] << 8);
      endMemoriaROM += (uint16_t)(buffer[1] );
      Serial.printf("\n\r-->endMemoriaROM: %d",endMemoriaROM);
/*     ________ ________
      |   Add  |  Data  |
      |--------|--------|
      | 0x0000 |  XXXX  | Proximo endereço
      |--------|--------|
      | 0x---- |  XXXX  | Numero de pulso MSB
      | 0x---- |  XXXX  | Numero de pulso LSB
      | 0x---- |  XXXX  | Hora
      | 0x---- |  XXXX  | Min
      | 0x---- |  XXXX  | Sec
      | 0x---- |  XXXX  | Dia 
      | 0x---- |  XXXX  | Mes
      |________|________| Ano    */
      // Escreve no barramento I2C
      // Salva os dados na memoria 
      ModuleRTC.WriteROM(endMemoriaROM++, ((contadorPulso & 0xFF00) >> 8) );
      ModuleRTC.WriteROM(endMemoriaROM++, ((contadorPulso & 0x00FF) >> 0) );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Hours );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Minutes );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Seconds );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Date );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Month );
      ModuleRTC.WriteROM(endMemoriaROM++, dataRTC.Year );

      // Salva o endereço proximo dado
      ModuleRTC.WriteROM(endProximo, ((endMemoriaROM & 0xFF00) >> 8) );
      ModuleRTC.WriteROM(endProximo + 1, ((endMemoriaROM & 0x00FF) >> 0) );

      Serial.printf("\n\r<--endMemoriaROM: %d",endMemoriaROM);
      Serial.print(COLOR_RESET);

      // libera o Mutex-I2C
      xSemaphoreGive(xMutex_I2C);

    }
  }
}

/**
 * @brief Set the Global Variable RTC object
 * 
 * @param Data 
 */
void SetVarRTC(DS1307Data_t Data)
{
  // Obtem o Mutex Variavel Global
  xSemaphoreTake(xMutex_Var,portMAX_DELAY );
  RTCData = Data;
  // libera o Mutex Variavel Global
  xSemaphoreGive(xMutex_Var);
}

/**
 * @brief Get the Global Variable RTC object
 * 
 * @return DS1307Data_t 
 */
DS1307Data_t GetVarRTC(void)
{
  DS1307Data_t Data;
  // Obtem o Mutex Variavel Global
  xSemaphoreTake(xMutex_Var,portMAX_DELAY );
  Data = RTCData;
  // libera o Mutex Variavel Global
  xSemaphoreGive(xMutex_Var);
  return Data;
}
