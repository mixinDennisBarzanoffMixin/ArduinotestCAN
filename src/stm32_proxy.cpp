// HAL-Includes für STM32
#include "stm32f4xx_hal.h"
#include <stdio.h>

// Pin-Definitionen
#define LED_PIN GPIO_PIN_13 // PB13 auf dem Black STM32F407VE Board
#define LED_GPIO_PORT GPIOB

// CAN-Pins (PE8 = RX, PE9 = TX)
#define CAN_RX_PIN GPIO_PIN_8
#define CAN_TX_PIN GPIO_PIN_9
#define CAN_GPIO_PORT GPIOE

// CAN-Handler
CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart1; // UART für Debug-Ausgaben

// Funktionsprototypen
void SystemClock_Config(void);
void GPIO_Init(void);
void CAN_Init(void);
void UART_Init(void);
void Error_Handler(void);
void CheckCANMessages(void);

// Puffer für serielle Ausgabe
uint8_t uart_tx_buffer[128];

// SysTick Handler für HAL_GetTick()
extern "C" void SysTick_Handler(void)
{
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}

int main(void)
{
  // HAL-Initialisierung
  HAL_Init();
  
  // System-Takt konfigurieren
  SystemClock_Config();
  
  // GPIO initialisieren (LED und andere Pins)
  GPIO_Init();
  
  // UART für Debug-Ausgaben initialisieren
  UART_Init();
  
  // Begrüßungsnachricht senden
  HAL_UART_Transmit(&huart1, (uint8_t*)"STM32 CAN-Proxy mit HAL initialisiert\r\n", 39, HAL_MAX_DELAY);
  
  // CAN initialisieren
  CAN_Init();
  
  while (1)
  {
    // LED blinken lassen
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
    
    // CAN-Nachrichten prüfen und verarbeiten
    CheckCANMessages();
    
    // Verzögerung 
    HAL_Delay(500);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Konfigurieren des Hauptoszillators (HSE = externer Quarz, 8MHz)
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  
  // Konfigurieren der Taktquellen
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  // GPIO-Takte aktivieren
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE(); // Für UART1 (PA9/PA10)
  
  // LED-Pin konfigurieren
  GPIO_InitStruct.Pin = LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
  
  // LED initial ausschalten
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
}

void UART_Init(void)
{
  // UART1-Takt aktivieren
  __HAL_RCC_USART1_CLK_ENABLE();
  
  // UART-Pins konfigurieren (PA9 = TX, PA10 = RX)
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  // UART konfigurieren
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

void CAN_Init(void)
{
  // CAN1-Takt aktivieren
  __HAL_RCC_CAN1_CLK_ENABLE();
  
  // GPIO für CAN konfigurieren (PE8 = RX, PE9 = TX)
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = CAN_RX_PIN | CAN_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStruct);
  
  // CAN konfigurieren
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;       // 42MHz/6 = 7MHz
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;  // 1+13+2 = 16TQ -> 7MHz/16 = 437.5kbps
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  
  // CAN initialisieren
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)"CAN-Initialisierung fehlgeschlagen!\r\n", 38, HAL_MAX_DELAY);
    Error_Handler();
  }
  
  // CAN-Filter konfigurieren (akzeptiere alle Nachrichten)
  CAN_FilterTypeDef filter;
  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14;
  
  if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)"CAN-Filter-Konfiguration fehlgeschlagen!\r\n", 44, HAL_MAX_DELAY);
    Error_Handler();
  }
  
  // CAN starten
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)"CAN-Start fehlgeschlagen!\r\n", 28, HAL_MAX_DELAY);
    Error_Handler();
  }
  
  // CAN-Interrupt für empfangene Nachrichten aktivieren
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)"CAN-Interrupt-Aktivierung fehlgeschlagen!\r\n", 45, HAL_MAX_DELAY);
    Error_Handler();
  }
  
  HAL_UART_Transmit(&huart1, (uint8_t*)"CAN erfolgreich initialisiert mit 437.5 kbps auf Pins PE8(RX) und PE9(TX)\r\n", 76, HAL_MAX_DELAY);
}

// CAN-Nachrichten prüfen und verarbeiten
void CheckCANMessages(void)
{
  if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0)
  {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    
    if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
    {
      // Nachrichtendetails formatieren
      int len = 0;
      
      // ID formatieren
      if (RxHeader.IDE == CAN_ID_STD)
      {
        len = sprintf((char*)uart_tx_buffer, "CAN ID: 0x%03lX", RxHeader.StdId);
      }
      else
      {
        len = sprintf((char*)uart_tx_buffer, "CAN ID: 0x%08lX (Extended)", RxHeader.ExtId);
      }
      
      // RTR Flag
      if (RxHeader.RTR == CAN_RTR_REMOTE)
      {
        len += sprintf((char*)(uart_tx_buffer + len), " (RTR)");
      }
      
      // Länge
      len += sprintf((char*)(uart_tx_buffer + len), " Länge: %d Daten: ", RxHeader.DLC);
      
      // Daten
      for (uint8_t i = 0; i < RxHeader.DLC; i++)
      {
        len += sprintf((char*)(uart_tx_buffer + len), "%02X ", RxData[i]);
      }
      
      // Zeilenumbruch
      len += sprintf((char*)(uart_tx_buffer + len), "\r\n");
      
      // Nachricht senden
      HAL_UART_Transmit(&huart1, uart_tx_buffer, len, HAL_MAX_DELAY);
    }
  }
}

// Fehlerbehandlung
void Error_Handler(void)
{
  // Bei Fehler schnelles Blinken
  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
    HAL_Delay(100);
  }
}