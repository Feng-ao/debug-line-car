/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
int a = 0;
int menu = 0;
int arrow = 0;
int led_mode = 0;
int led_on = 0;
unsigned int led_tick = 0;
char rxbuf[20];
int rxlen = 0;
int x[8];
int pian_cha = 0;
int shang_ci = 0;
int zhi_dao = 0;
int zhi_xing_ms = 0;
int huan_zuo_ms = 0;
int guo_du_ms = 0;
int diu_cnt = 0;
int bai_cnt = 0;
int fang_xiang = 0;
int zhong_lu = 0;
unsigned int huan_leng = 0;
int pao = 0;
unsigned int kai_shi = 0;
unsigned int shang_ci_fa = 0;
unsigned int shang_ci_shua = 0;
unsigned int shang_ci_kong = 0;
int tiaoshi = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
int AnJian_Duqu(void);
void AnJian_Chuli(void);
void Zhucaidan_AnJian(int k);
void JianTou_Shang(void);
void JianTou_Xia(void);
void Jinru_Caidan(void);
void LED_AnJian(int k);
void LED_KaiGuan(void);
void Xianshi_Zhucaidan(void);
void Xianshi_LED(void);
void Xianshi_LEDZhuangtai(void);
void Xianshi_Xinxi(void);
void Xianshi_Tuozhan(void);
void LED_QuanLiang(void);
void LED_QuanMie(void);
void LED_Shanshuo(void);
void ChuanKou_Chuli(void);
void ChuanKou_Panduan(void);
void FaSong(char *s);
void FaSongShu(int n);
void FanZhuan_Fasong(char *s, int len, int f);
int ZhuanShuzi(char *s);
void Chuanganqi_Duqu(void);
int Piancha_Jisuan(void);
void Dianji_Zuo(int v);
void Dianji_You(int v);
void Dianji_Ting(void);
int QuanBai(void);
int YaLu_LuShu(void);
int Duanlu_Chuli(void);
int ZhiXing_Pao(void);
int Liubian_Chuli(void);
int Zhuanliang_Jisuan(int e);
int GuoDu_XianZhi(int zhuan);
int Sudu_Jisuan(int e);
void Jiquan_Chuli(void);
void Jilu_ZhongLu(void)
{
  if (YaLu_LuShu() == 0) { return; }
  if (x[2] == 1 || x[3] == 1 || x[4] == 1 || x[5] == 1) { zhong_lu = 1; }
  else { zhong_lu = 0; }
}

void DiuXian_Chuli(void)
{
  if (zhi_xing_ms > 0)
  {
    diu_cnt = 0;
    return;
  }
  if (QuanBai() == 1) { diu_cnt = diu_cnt + 1; }
  else { diu_cnt = 0; }
  if (diu_cnt > 100)
  {
    pao = 0;
    Dianji_Ting();
  }
}

void Jilu_ZhongLu(void);
void DiuXian_Chuli(void);
void Xunji_Run(void);
void Fengming(int ms);
void Xianshi_Chuanganqi(void);
void Xianshi_Xunji(void);
void Xianshi_Shuju(void);
void Tiaoshi_Fasong(void);
void Xunji_AnJian(int k);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  LED_QuanMie();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  Dianji_Ting();
  OLED_Init();
  kai_shi = HAL_GetTick();
  OLED_Clear();
  Xianshi_Zhucaidan();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Chuanganqi_Duqu();
    ChuanKou_Chuli();
    AnJian_Chuli();
    LED_Shanshuo();
    if (pao == 1)
    {
      if (HAL_GetTick() - shang_ci_kong >= 10)
      {
        shang_ci_kong = HAL_GetTick();
        Xunji_Run();
      }
    }
    if (menu == 3)
    {
      Xianshi_Shuju();
    }
    Tiaoshi_Fasong();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int AnJian_Duqu(void)
{
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0) { HAL_Delay(20); while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0) { } return 1; }
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == 0) { HAL_Delay(20); while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == 0) { } return 2; }
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == 0) { HAL_Delay(20); while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == 0) { } return 3; }
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == 0) { HAL_Delay(20); while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == 0) { } return 4; }
  return 0;
}

void AnJian_Chuli(void)
{
  int k = AnJian_Duqu();

  if (k == 0) { return; }
  if (menu == 0) { Zhucaidan_AnJian(k); return; }
  if (menu == 1) { LED_AnJian(k); return; }
  if (menu == 3) { Xunji_AnJian(k); return; }
  if (k == 4)
  {
    menu = 0;
    Xianshi_Zhucaidan();
  }
}

void Zhucaidan_AnJian(int k)
{
  if (k == 1)
  {
    JianTou_Shang();
  }
  if (k == 2)
  {
    JianTou_Xia();
  }
  if (k == 3)
  {
    Jinru_Caidan();
  }
}

void JianTou_Shang(void)
{
  arrow = arrow - 1;
  if (arrow < 0)
  {
    arrow = 3;
  }
  Xianshi_Zhucaidan();
}

void JianTou_Xia(void)
{
  arrow = arrow + 1;
  if (arrow > 3)
  {
    arrow = 0;
  }
  Xianshi_Zhucaidan();
}

void Jinru_Caidan(void)
{
  if (arrow == 0) { menu = 1; Xianshi_LED(); }
  if (arrow == 1) { menu = 2; Xianshi_Xinxi(); }
  if (arrow == 2) { menu = 3; Xianshi_Xunji(); }
  if (arrow == 3) { menu = 4; Xianshi_Tuozhan(); }
}

void LED_AnJian(int k)
{
  if (k == 1)
  {
    LED_KaiGuan();
  }
  if (k == 2)
  {
    led_mode = 2;
    led_tick = HAL_GetTick();
    Xianshi_LED();
  }
  if (k == 4)
  {
    led_mode = 0;
    LED_QuanMie();
    menu = 0;
    Xianshi_Zhucaidan();
  }
}

void LED_KaiGuan(void)
{
  if (led_mode == 1)
  {
    led_mode = 0;
    LED_QuanMie();
  }
  else
  {
    led_mode = 1;
    LED_QuanLiang();
  }
  Xianshi_LED();
}

void LED_QuanLiang(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
}

void LED_QuanMie(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}

void LED_Shanshuo(void)
{
  if (led_mode != 2) { return; }
  if (HAL_GetTick() - led_tick < 500) { return; }
  led_tick = HAL_GetTick();
  led_on = !led_on;
  if (led_on)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
  }
  if (menu == 1)
  {
    Xianshi_LEDZhuangtai();
  }
}

void Xianshi_Zhucaidan(void)
{
  OLED_Clear();
  if (arrow == 0) { OLED_ShowChar(20, 0, '>'); }
  if (arrow == 1) { OLED_ShowChar(20, 2, '>'); }
  if (arrow == 2) { OLED_ShowChar(20, 4, '>'); }
  if (arrow == 3) { OLED_ShowChar(20, 6, '>'); }
  OLED_ShowString(40, 0, "LED");
  OLED_ShowString(40, 2, "XINXI");
  OLED_ShowString(40, 4, "XUNJI");
  OLED_ShowString(40, 6, "TUOZHAN");
}

void Xianshi_LEDZhuangtai(void)
{
  if (led_on)
  {
    OLED_ShowString(8, 2, "LED1: ON ");
    OLED_ShowString(8, 4, "LED2: OFF");
  }
  else
  {
    OLED_ShowString(8, 2, "LED1: OFF");
    OLED_ShowString(8, 4, "LED2: ON ");
  }
}

void Xianshi_LED(void)
{
  OLED_Clear();
  if (led_mode == 0) { OLED_ShowString(8, 0, "MODE: OFF"); }
  if (led_mode == 1) { OLED_ShowString(8, 0, "MODE: ON "); }
  if (led_mode == 2) { OLED_ShowString(8, 0, "MODE: FLASH"); }
  if (led_mode == 0)
  {
    OLED_ShowString(8, 2, "LED1: OFF");
    OLED_ShowString(8, 4, "LED2: OFF");
  }
  if (led_mode == 1)
  {
    OLED_ShowString(8, 2, "LED1: ON ");
    OLED_ShowString(8, 4, "LED2: ON ");
  }
  if (led_mode == 2)
  {
    Xianshi_LEDZhuangtai();
  }
  OLED_ShowString(8, 6, "K1:ON K2:FLASH");
}

void Xianshi_Xinxi(void)
{
  OLED_Clear();
  OLED_ShowString(8, 0, "NAME: FENG AO");
  OLED_ShowString(16, 2, "ID: 32602156");
  OLED_ShowString(20, 4, "A = ");
  OLED_ShowNum(52, 4, a, 4, 16);
  OLED_ShowString(32, 6, "K4: BACK");
}

void Xianshi_Tuozhan(void)
{
  OLED_Clear();
  OLED_ShowString(36, 0, "TUOZHAN");
  OLED_ShowString(20, 2, "COMING SOON");
  OLED_ShowString(32, 6, "K4: BACK");
}

void ChuanKou_Chuli(void)
{
  char c;

  if (HAL_UART_Receive(&huart1, (uint8_t *)&c, 1, 5) != HAL_OK)
  {
    return;
  }
  if (c == '\r' || c == '\n')
  {
    if (rxlen > 0)
    {
      rxbuf[rxlen] = 0;
      ChuanKou_Panduan();
      rxlen = 0;
    }
    return;
  }
  if (rxlen < 19)
  {
    rxbuf[rxlen] = c;
    rxlen = rxlen + 1;
  }
}

void ChuanKou_Panduan(void)
{
  if (strcmp(rxbuf, "GET A") == 0)
  {
    FaSongShu(a);
    return;
  }
  a = ZhuanShuzi(rxbuf);
  if (menu == 2)
  {
    Xianshi_Xinxi();
  }
}

void FaSong(char *s)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), 50);
}

void FaSongShu(int n)
{
  char s[12];
  int i = 0;
  int f = 0;

  if (n < 0)
  {
    f = 1;
    n = -n;
  }
  if (n == 0)
  {
    s[i] = '0';
    i = i + 1;
  }
  while (n > 0)
  {
    s[i] = '0' + n % 10;
    i = i + 1;
    n = n / 10;
  }
  FanZhuan_Fasong(s, i, f);
}

void FanZhuan_Fasong(char *s, int len, int f)
{
  char out[14];
  int j = 0;

  if (f == 1)
  {
    out[j] = '-';
    j = j + 1;
  }
  while (len > 0)
  {
    len = len - 1;
    out[j] = s[len];
    j = j + 1;
  }
  out[j] = '\r';
  out[j + 1] = '\n';
  out[j + 2] = 0;
  FaSong(out);
}

int ZhuanShuzi(char *s)
{
  int i = 0;
  int f = 1;
  int n = 0;

  if (s[0] == '-')
  {
    f = -1;
    i = 1;
  }
  while (s[i] >= '0' && s[i] <= '9')
  {
    n = n * 10 + (s[i] - '0');
    i = i + 1;
  }
  return n * f;
}

void Chuanganqi_Duqu(void)
{
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == 0) { x[0] = 1; } else { x[0] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == 0) { x[1] = 1; } else { x[1] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == 0) { x[2] = 1; } else { x[2] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == 0) { x[3] = 1; } else { x[3] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == 0) { x[4] = 1; } else { x[4] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == 0) { x[5] = 1; } else { x[5] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == 0) { x[6] = 1; } else { x[6] = 0; }
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == 0) { x[7] = 1; } else { x[7] = 0; }
}

int Piancha_Jisuan(void)
{
  int i;
  int n = 0;
  int he = 0;

  for (i = 0; i < 8; i++)
  {
    if (x[i] == 1)
    {
      he = he + (i * 2 - 7);
      n = n + 1;
    }
  }
  if (n == 0)
  {
    if (fang_xiang > 0) { return 700; }
    if (fang_xiang < 0) { return -700; }
    return pian_cha;
  }
  pian_cha = he * 100 / n;
  if (pian_cha > 300) { fang_xiang = 1; }
  if (pian_cha < -300) { fang_xiang = -1; }
  return pian_cha;
}

void Dianji_Zuo(int v)
{
  if (v >= 0)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
  }
  if (v < 0) { v = -v; }
  if (v > 999) { v = 999; }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, v);
}

void Dianji_You(int v)
{
  if (v >= 0)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
  }
  if (v < 0) { v = -v; }
  if (v > 999) { v = 999; }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, v);
}

void Dianji_Ting(void)
{
  Dianji_Zuo(0);
  Dianji_You(0);
}

int QuanBai(void)
{
  int i;

  for (i = 0; i < 8; i++)
  {
    if (x[i] == 1) { return 0; }
  }
  return 1;
}

int YaLu_LuShu(void)
{
  int i;
  int n = 0;

  for (i = 0; i < 8; i++)
  {
    if (x[i] == 1) { n = n + 1; }
  }
  return n;
}

int Duanlu_Chuli(void)
{
  if (zhi_xing_ms > 0) { return ZhiXing_Pao(); }
  if (QuanBai() == 1) { bai_cnt = bai_cnt + 1; }
  else { bai_cnt = 0; }
  if (bai_cnt >= 2 && zhi_dao > 30 && zhong_lu == 1)
  {
    zhi_xing_ms = 1500;
  }
  return 0;
}

int ZhiXing_Pao(void)
{
  if (QuanBai() == 0)
  {
    zhi_xing_ms = 0;
    guo_du_ms = 250;
    return 0;
  }
  zhi_xing_ms = zhi_xing_ms - 10;
  if (zhi_xing_ms <= 0)
  {
    pao = 0;
    Dianji_Ting();
    return 1;
  }
  Dianji_Zuo(380);
  Dianji_You(380);
  return 1;
}

int Liubian_Chuli(void)
{
  int zuo;
  int you;
  int zhong;

  if (huan_zuo_ms > 0)
  {
    huan_zuo_ms = huan_zuo_ms - 10;
    Dianji_Zuo(150);
    Dianji_You(750);
    return 1;
  }
  if (HAL_GetTick() - huan_leng < 25000) { return 0; }
  zuo = x[0] + x[1] + x[2];
  you = x[5] + x[6] + x[7];
  zhong = x[2] + x[3] + x[4];
  if (zuo > 0 && you > 0 && zhong == 0)
  {
    huan_zuo_ms = 300;
    huan_leng = HAL_GetTick();
  }
  return 0;
}

int Zhuanliang_Jisuan(int e)
{
  int zhuan = (e * 110 + (e - shang_ci) * 160) / 100;

  if (e > 250 || e < -250) { zhuan = zhuan * 18 / 10; }
  else if (e > 120 || e < -120) { zhuan = zhuan * 12 / 10; }
  if (zhuan > 900) { zhuan = 900; }
  if (zhuan < -900) { zhuan = -900; }
  return zhuan;
}

int GuoDu_XianZhi(int zhuan)
{
  if (guo_du_ms <= 0) { return zhuan; }
  guo_du_ms = guo_du_ms - 10;
  if (zhuan > 350) { zhuan = 350; }
  if (zhuan < -350) { zhuan = -350; }
  return zhuan;
}

int Sudu_Jisuan(int e)
{
  int sudu = e;

  if (sudu < 0) { sudu = -sudu; }
  sudu = 260 - sudu / 4;
  if (sudu < 200) { sudu = 200; }
  return sudu;
}

void Jiquan_Chuli(void)
{
  unsigned int t = HAL_GetTick() - kai_shi;

  if (t > 204700)
  {
    pao = 0;
    Dianji_Ting();
    return;
  }
  if (t < 169100) { return; }
  if (t > 190460) { return; }
  if (YaLu_LuShu() < 7) { return; }
  pao = 0;
  Dianji_Ting();
  Fengming(200);
}

void Xunji_Run(void)
{
  int e;
  int zhuan;
  int sudu;

  if (Duanlu_Chuli() == 1) { return; }
  if (Liubian_Chuli() == 1) { return; }

  e = Piancha_Jisuan();
  zhuan = GuoDu_XianZhi(Zhuanliang_Jisuan(e));
  sudu = Sudu_Jisuan(e);
  Dianji_Zuo(sudu + zhuan);
  Dianji_You(sudu - zhuan);
  shang_ci = e;

  Jilu_ZhongLu();
  if (YaLu_LuShu() == 0) { }
  else if (e < 250 && e > -250 && YaLu_LuShu() <= 2) { zhi_dao = zhi_dao + 1; }
  else { zhi_dao = 0; }

  DiuXian_Chuli();
  Jiquan_Chuli();
}

void Fengming(int ms)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(ms);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

void Xianshi_Chuanganqi(void)
{
  int i;
  int p;

  for (i = 0; i < 8; i++)
  {
    p = 32 + i * 8;
    if (x[i] == 1) { OLED_ShowString(p, 0, "1"); }
    else { OLED_ShowString(p, 0, "0"); }
  }
}

void Xianshi_Xunji(void)
{
  OLED_Clear();
  Xianshi_Chuanganqi();
  OLED_ShowString(0, 2, "TING");
  OLED_ShowString(0, 4, "T=");
  OLED_ShowString(0, 6, "K3:GO K4:BACK");
}

void Xianshi_Shuju(void)
{
  if (HAL_GetTick() - shang_ci_shua < 200) { return; }
  shang_ci_shua = HAL_GetTick();
  Xianshi_Chuanganqi();
  if (pao == 1) { OLED_ShowString(0, 2, "PAO "); }
  else { OLED_ShowString(0, 2, "TING"); }
  OLED_ShowString(24, 4, "     ");
  OLED_ShowNum(24, 4, (HAL_GetTick() - kai_shi) / 1000, 3, 16);
}

void Tiaoshi_Fasong(void)
{
  char s[10];
  int i;

  if (tiaoshi == 0) { return; }
  if (HAL_GetTick() - shang_ci_fa < 100) { return; }
  shang_ci_fa = HAL_GetTick();
  for (i = 0; i < 8; i++)
  {
    if (x[i] == 1) { s[i] = '1'; }
    else { s[i] = '0'; }
  }
  s[8] = ' ';
  s[9] = 0;
  FaSong(s);
  FaSongShu(pian_cha);
}

void Xunji_AnJian(int k)
{
  if (k == 3)
  {
    kai_shi = HAL_GetTick();
    shang_ci = 0;
    zhi_dao = 0;
    zhi_xing_ms = 0;
    huan_zuo_ms = 0;
    guo_du_ms = 0;
    diu_cnt = 0;
    bai_cnt = 0;
    fang_xiang = 0;
    zhong_lu = 0;
    huan_leng = HAL_GetTick() - 25000;
    pao = 1;
    Fengming(200);
  }
  if (k == 4)
  {
    pao = 0;
    Dianji_Ting();
    menu = 0;
    Xianshi_Zhucaidan();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
