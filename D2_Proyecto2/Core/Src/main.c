/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fatfs_sd.h"
#include "string.h"
#include "stdio.h"
#include "ili9341.h"
#include <stdlib.h>
#include "Bitmaps.h"
#include "imagenes2_rgb565_grouped_flat.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define SQ_SIZE 16
#define HEIGHT 15
#define WIDTH 20
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
UINT br;
uint32_t totalSpace, freeSpace;
char buffer[700];
uint8_t input[1];
char opcion = 0;
uint8_t nivel = 0;
uint8_t received = 0;
uint8_t transmitted = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_tx;

/* USER CODE BEGIN PV */


// 0 -> Arriba
// 1 -> Abajo
// 3 -> Derecha
// 2 -> Izquierda


static uint32_t lastInterruptTick = 0;

static uint32_t lastITick_orange = 0;

static uint32_t lastInterruptTick_prueba = 0;
uint8_t prueba_iniciar_conteo = 0;


extern uint8_t fondo[];

// VARIABLES DEL JUEGO
unsigned char menu_buf[320*2*2];

uint8_t infinity_mode = 1;
int speed = 150;
uint8_t input_controles[1];
uint8_t received_input = 0;

uint8_t map[HEIGHT][WIDTH];

uint8_t game_over = 0;

uint8_t stop_animations = 0;

uint8_t ciclo_prueba = 0;

uint8_t menu = 0;

uint8_t menu_window = 0;

uint8_t showed_window = 0;

uint8_t menu_option = 0;

uint8_t select_option = 0;

uint8_t update_buttons = 0;

int available_cells;
int orange_random_pos;
int cell_count;
int possible_orange_cell;
int positioned_orange;
uint8_t max_oranges = 10;

uint8_t sonido;

// VARIABLES SNAKE1
uint8_t orange_count = 0;
uint8_t snake1[100][3];
uint8_t len_snake1;
uint8_t dir_snake1 = 0;
uint8_t snake1_tail_shade;
uint8_t snake1_lost = 0;
uint8_t snake1_prev_x;
uint8_t snake1_prev_y;
uint8_t snake1_prev_dir;
uint8_t snake1_next_x;
uint8_t snake1_next_y;
uint8_t snake1_next_dir;
uint8_t snake1_next_cell_value;
uint8_t snake1_eats = 0;
uint8_t snake1_current_cell_value;
uint8_t snake1_orange_around = 0;

// VARIABLES SNAKE2
uint8_t snake2[100][3];
uint8_t len_snake2;
uint8_t dir_snake2 = 0;
uint8_t snake2_tail_shade;
uint8_t snake2_lost = 0;
uint8_t snake2_prev_x;
uint8_t snake2_prev_y;
uint8_t snake2_prev_dir;
uint8_t snake2_next_x;
uint8_t snake2_next_y;
uint8_t snake2_next_dir;
uint8_t snake2_next_cell_value;
uint8_t snake2_eats = 0;
uint8_t snake2_current_cell_value;
uint8_t snake2_orange_around = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



void CreateBackground(void){
	uint8_t shade = 0;
	// celdas de 8x8 pixeles
	for (int i = 0; i < HEIGHT; i++){
		uint16_t y = i*SQ_SIZE;
		for (int j = 0; j< WIDTH ; j++){
			uint16_t x = j*SQ_SIZE;
			map[i][j] = shade;
			if (shade){
				FillRect(x, y, SQ_SIZE, SQ_SIZE, 0x04C1);
				shade = 0;
			} else {
				FillRect(x, y, SQ_SIZE, SQ_SIZE, 0x0660);
				shade = 1;
			}

		}
		if (shade == 0){
			shade = 1;
		} else {
			shade = 0;
		}
	}
}

void init_random(void){
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	uint16_t ruido = HAL_ADC_GetValue(&hadc1);
	srand(ruido);
}

int get_random(int min, int max){
	return (rand() % (max - min + 1)) + min;
}

void restart_game(void){
	sonido = 'j';
	HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
	stop_animations = 0;
	game_over = 0;
	snake1_lost = 0;
	snake2_lost = 0;
	orange_count = 0;
	LCD_Clear(0x00);

	CreateBackground();

	//INICIALIZACION SNAKE 1
	len_snake1 = 6;
	  for (int i = 0; i < len_snake1; i++){
		  snake1[i][0] = 4;
		  snake1[i][1] = i+5;
		  snake1[i][2] = 0;
		  map[i+5][4] += 2;
		  snake1_current_cell_value = map[snake1[i][1]][snake1[i][0]]%2;
		  if (i==0){
			  if (snake1_current_cell_value == 1){
				  LCD_Bitmap((snake1[i][0]*SQ_SIZE), (snake1[i][1]*SQ_SIZE), SQ_SIZE, SQ_SIZE, head_snake1_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_0, 0, 0);
			  }
		  } else if (i == len_snake1-1){
			  if (snake1_current_cell_value == 1){
				  LCD_Bitmap(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_0, 0, 0);
			  }
		  } else {
			  //FillRect(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x0019);
			  if (snake1_current_cell_value == 1){
				  LCD_Bitmap(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_0, 0, 0);
			  }
		  }

	  }
	  dir_snake1 = 0;
	  snake1_next_x = snake1[0][0];
	  snake1_next_y = snake1[0][1];

	  //INICIALIZACIOIN SNAKE 2
	  len_snake2 = 6;
		for (int i = 0; i < len_snake2; i++){
		  snake2[i][0] = WIDTH-5;
		  snake2[i][1] = i+5;
		  snake2[i][2] = 0;
		  map[i+5][WIDTH-5] += 4;
		  snake2_current_cell_value = map[snake2[i][1]][snake2[i][0]]%2;
		  if (i==0){
			  if (snake2_current_cell_value == 1){
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_0, 0, 0);
			  }
		  } else if (i == len_snake1-1){
			  if (snake2_current_cell_value == 1){
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_0, 0, 0);
			  }
		  } else {
			  //FillRect(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x0019);
			  if (snake2_current_cell_value == 1){
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_1, 0, 0);
			  } else {
				  LCD_Bitmap(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_0, 0, 0);
			  }
		  }
		}
		dir_snake2 = 0;
		snake2_next_x = snake2[0][0];
		snake2_next_y = snake2[0][1];
}

void display_menu(void){
	menu = 1;
	game_over = 1;
	menu_window = 0;
	menu_option = 0;
	select_option = 0;
	update_buttons = 1;
	sonido = 'm';
	HAL_UART_Transmit_DMA(&huart3, &sonido, 1);

	LCD_Clear(0x00);



	//Montar el sistema de archivos
	  fres = f_mount(&fs, "", 0);
	  if (fres == FR_OK){
		  fres = f_open(&fil, "menu_0.bin", FA_READ);
		  if (fres == FR_OK) {
			  for (uint16_t row = 0; row < 240; row+=2){
				  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
				  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
					  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
				  }
			  }
			f_close(&fil);


		}
		  f_mount(NULL, "", 1);
	  }

	  showed_window = 1;

	//


	//f_mount(NULL, "", 1);
	  /*if (fres == FR_OK){
		  transmit_uart("Card Unmounted");
	  } else if (fres != FR_OK){
	      transmit_uart("Card not Unmounted");
	  }*/
}

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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500);

  HAL_UART_Receive_IT(&huart3, input_controles, 1);

  init_random();

  LCD_Init();

  //restart_game();

  display_menu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //MOVIMIENTO CON UART
	  // i -> izquierda snake 1
	  // d -> derecha snake 1
	  // a -> arriba snake 1
	  // b -> abajo  snake 1
	  // p -> botón 1
	  // I -> izquierda snake 1
	  // D -> derecha snake 1
	  // A -> arriba snake 1
	  // B -> abajo  snake 1
	  // P -> botón 1
	  if (received_input == 1){
		  received_input = 0;
		  char command = input_controles[0];
		  if (game_over == 0){
			  switch (command){
			  //Snake 1
			  case 'i':
				  if (dir_snake1 != 3){
					  dir_snake1 = 2;
				  }
				  break;
			  case 'd':
				  if (dir_snake1 != 2){
					  dir_snake1 = 3;
				  }
				  break;
			  case 'a':
				  if (dir_snake1 != 1){
					  dir_snake1 = 0;
				  }
				  break;
			  case 'b':
				  if (dir_snake1 != 0){
					  dir_snake1 = 1;
				  }
				  break;
			  //Snake 2
			  case 'I':
				  if (dir_snake2 != 3){
					  dir_snake2 = 2;
				  }
				  break;
			  case 'D':
				  if (dir_snake2 != 2){
					  dir_snake2 = 3;
				  }
				  break;
			  case 'A':
				  if (dir_snake2 != 1){
					  dir_snake2 = 0;
				  }
				  break;
			  case 'B':
				  if (dir_snake2 != 0){
					  dir_snake2 = 1;
				  }
				  break;
			  default:
				  break;
			  }
		  }

		  if (menu){
			  update_buttons = 1;
			  switch (command){
			  case 'a':
				  if (menu_option != 0){
					  menu_option -= 1;
				  }
				  break;
			  case 'b':
				  if (menu_window){
					  if (menu_option < 3){
						  menu_option += 1;
					  }
				  } else {
					  if (menu_option < 1){
						  menu_option += 1;
					  }
				  }
				  break;
			  case 'i':
				  if (menu_option == 2){
					  if (max_oranges > 1){
						  max_oranges -= 1;
					  }
				  }
				  break;
			  case 'd':
				  if (menu_option == 2){
					  if (max_oranges < 30){
						  max_oranges += 1;
					  }
				  }
				  break;
			  case 'p':
				  select_option = 1;
				  break;
			  }
		  }
	  }

	  if (menu){
		  if (menu_window){ //Ventana de ajustes
			  if (select_option){
				  select_option = 0;
				  switch (menu_option){
				  case 0:
					  if (speed == 450){
						  speed = 300;
					  } else if (speed == 300){
						  speed = 150;
					  } else {
						  speed = 450;
					  }
					  break;
				  case 1:
					  if (infinity_mode){
						  infinity_mode = 0;
					  } else {
						  infinity_mode = 1;
					  }
					  break;
				  case 2:
					  break;
				  case 3:
					  menu_window = 0;
					  showed_window = 0;
					  menu_option = 0;
					  break;
				  }
			  } else {
				  // GRAPHICS SETTINGS WINDOW
				  if (showed_window == 0){
					  fres = f_mount(&fs, "", 0);
					  if (fres == FR_OK){
						  fres = f_open(&fil, "menu_1.bin", FA_READ);
						  if (fres == FR_OK) {
							  for (uint16_t row = 0; row < 240; row+=2){
								  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
								  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
									  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
								  }
							  }
							f_close(&fil);


						}
						  f_mount(NULL, "", 1);
					  }
					  showed_window = 1;
				  }
				  if (update_buttons){
					  update_buttons = 0;
					  if (speed == 450){
						  if (menu_option == 0){
							  LCD_Bitmap(123, 45, button_speed_low_1_width, button_speed_low_1_height, button_speed_low_1_data, 0, 0);
						  } else {
							  LCD_Bitmap(123, 45, button_speed_low_0_width, button_speed_low_0_height, button_speed_low_0_data, 0, 0);
						  }
					  } else if (speed == 300){
						  if (menu_option == 0){
							  LCD_Bitmap(123, 45, button_speed_medium_1_width, button_speed_medium_1_height, button_speed_medium_1_data, 0, 0);
						  } else {
							  LCD_Bitmap(123, 45, button_speed_medium_0_width, button_speed_medium_0_height, button_speed_medium_0_data, 0, 0);
						  }
					  } else {
						  if (menu_option == 0){
							  LCD_Bitmap(123, 45, button_speed_high_1_width, button_speed_high_1_height, button_speed_high_1_data, 0, 0);
						  } else {
							  LCD_Bitmap(123, 45, button_speed_high_0_width, button_speed_high_0_height, button_speed_high_0_data, 0, 0);
						  }
					  }
					  if (infinity_mode){
						  if (menu_option == 1){
							  LCD_Bitmap(122, 104, button_mode_infinity_1_width, button_mode_infinity_1_height, button_mode_infinity_1_data, 0, 0);
						  } else {
							  LCD_Bitmap(122, 104, button_mode_infinity_0_width, button_mode_infinity_0_height, button_mode_infinity_0_data, 0, 0);
						  }
					  } else {
						  if (menu_option == 1){
							  LCD_Bitmap(122, 104, button_mode_limited_1_width, button_mode_limited_1_height, button_mode_limited_1_data, 0, 0);
						  } else {
							  LCD_Bitmap(122, 104, button_mode_limited_0_width, button_mode_limited_0_height, button_mode_limited_0_data, 0, 0);
						  }
					  }
					  // NUMBERS ORANGES
					  if (menu_option == 2){
						  if (max_oranges > 1){
							  LCD_Bitmap(137, 160, button_arrow_left_1_width, button_arrow_left_1_height, button_arrow_left_1_data, 0, 0);
						  } else {
							  FillRect(137, 160, button_arrow_left_1_width, button_arrow_left_1_height, 0xE302);
						  }
						  if (max_oranges < 30){
							  LCD_Bitmap(176, 160, button_arrow_right_1_width, button_arrow_right_1_height, button_arrow_right_1_data, 0, 0);
						  } else {
							  FillRect(176, 160, button_arrow_right_1_width, button_arrow_right_1_height, 0xE302);
						  }
					  } else {
						  if (max_oranges > 1){
							  LCD_Bitmap(137, 160, button_arrow_left_0_width, button_arrow_left_0_height, button_arrow_left_0_data, 0, 0);
						  } else {
							  FillRect(137, 160, button_arrow_left_0_width, button_arrow_left_0_height, 0xE302);
						  }
						  if (max_oranges < 30){
							  LCD_Bitmap(176, 160, button_arrow_right_0_width, button_arrow_right_0_height, button_arrow_right_0_data, 0, 0);
						  } else {
							  FillRect(176, 160, button_arrow_right_0_width, button_arrow_right_0_height, 0xE302);
						  }
					  }
					  uint8_t d_max_oranges = max_oranges / 10;
					  uint8_t u_max_oranges = max_oranges % 10;
					  LCD_Bitmap(147, 162, button_numbers_width[d_max_oranges], button_numbers_height[d_max_oranges], button_numbers_data[d_max_oranges], 0, 0);
					  LCD_Bitmap(162, 162, button_numbers_width[u_max_oranges], button_numbers_height[u_max_oranges], button_numbers_data[u_max_oranges], 0, 0);
					  // ------------------
					  if (menu_option == 3){
						  LCD_Bitmap(138, 189, button_back_1_width, button_back_1_height, button_back_1_data, 0, 0);
					  } else {
						  LCD_Bitmap(138, 189, button_back_0_width, button_back_0_height, button_back_0_data, 0, 0);
					  }
				  }
				  //---------------------------
			  }


		  } else { // Ventana principal
			  if (select_option){
				  select_option = 0;
				  if (menu_option){ // SELECT SETTINGS
					  menu_window = 1;
					  showed_window = 0;
				  } else {
					  menu = 0;
					  restart_game();
				  }
			  } else {
				  // GRAPHICS MAIN WINDOW
				  if (showed_window == 0){
					  fres = f_mount(&fs, "", 0);
					  if (fres == FR_OK){
						  fres = f_open(&fil, "menu_0.bin", FA_READ);
						  if (fres == FR_OK) {
							  for (uint16_t row = 0; row < 240; row+=2){
								  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
								  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
									  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
								  }
							  }
							f_close(&fil);


						}
						  f_mount(NULL, "", 1);
					  }
					  showed_window = 1;
				  }
				  //LCD_Bitmap(40, 40, button_arrow_left_0_width, button_arrow_left_0_height, button_arrow_left_0_data, 0, 0);
				  if (update_buttons){
					  update_buttons = 0;
					  if (menu_option){
						  LCD_Bitmap(108, 157, button_settings_1_width, button_settings_1_height, button_settings_1_data, 0, 0);
						  LCD_Bitmap(124, 131, button_start_0_width, button_start_0_height, button_start_0_data, 0, 0);
					  } else {
						  LCD_Bitmap(108, 157, button_settings_0_width, button_settings_0_height, button_settings_0_data, 0, 0);
						  LCD_Bitmap(124, 131, button_start_1_width, button_start_1_height, button_start_1_data, 0, 0);
					  }
				  }
				  //---------------------------
			  }

		  }
	  }


	  uint32_t currentTick = HAL_GetTick();
	  if ((currentTick-lastInterruptTick)>speed){
		  lastInterruptTick = currentTick;

		  if (game_over == 0){
			  //GENERACION NARANJAS
			  uint32_t currentTick = HAL_GetTick();
			  if ((currentTick - lastITick_orange)>speed*2){
				  lastITick_orange = currentTick;
				  if (orange_count < max_oranges){
					  available_cells = HEIGHT*WIDTH-len_snake1-len_snake2-orange_count;
					  orange_random_pos = get_random(0, available_cells-1);
					  cell_count = -1;
					  positioned_orange = 0;
					  for  (int i = 0; i<HEIGHT; i++){
						  for (int j = 0; j<WIDTH; j++){
							  possible_orange_cell = map[i][j] - map[i][j]%2;
							  if ((possible_orange_cell != 6)&&(possible_orange_cell != 4)&&(possible_orange_cell != 2)){
								  cell_count += 1;
							  }
							  if (cell_count == orange_random_pos){
								  map[i][j] += 6;
								  if (map[i][j]%2 == 1){
									  LCD_Bitmap(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, orange_1, 0, 0);
								  } else {
									  LCD_Bitmap(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, orange_0, 0, 0);
								  }
								  //FillRect(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0xFC00);
								  orange_count += 1;
								  positioned_orange = 1;
							  }
							  if (positioned_orange == 1){
								  break;
							  }
						  }
						  if (positioned_orange == 1){
							  break;
						  }
					  }
				  }
			  }

			  // LOGICA MOVIMIENTO SERPIENTE 1
			  snake1_prev_x = snake1[len_snake1-1][0];
			  snake1_prev_y = snake1[len_snake1-1][1];
			  //snake1_prev_dir = snake1[len_snake1-1][2];
			  if (((snake1[len_snake1-2][1] - snake1[len_snake1-3][1]) == 1)||((snake1[len_snake1-2][1] - snake1[len_snake1-3][1]) == -(HEIGHT-1))){
				  snake1_prev_dir = 0;
			  } else if (((snake1[len_snake1-2][1] - snake1[len_snake1-3][1]) == -1)||((snake1[len_snake1-2][1] - snake1[len_snake1-3][1]) == (HEIGHT-1))){
				  snake1_prev_dir = 1;
			  } else if (((snake1[len_snake1-2][0] - snake1[len_snake1-3][0]) == 1)||((snake1[len_snake1-2][0] - snake1[len_snake1-3][0]) == -(WIDTH-1))){
				  snake1_prev_dir = 2;
			  } else {
				  snake1_prev_dir = 3;
			  }
			  switch (dir_snake1){
			  case 0: // Arriba
				  snake1_next_x = snake1[0][0];
				  if (snake1[0][1] == 0){
					  if (infinity_mode){
						  snake1_next_y = HEIGHT-1;
					  } else {
						  snake1_lost = 1;
					  }
				  } else {
					  snake1_next_y -= 1;
				  }
				  if ((snake1[0][2] == 2)||(snake1[0][2] == 4)||(snake1[0][2] == 6)){
					  snake1_next_dir = 7;
				  } else if ((snake1[0][2] == 3)||(snake1[0][2] == 5)||(snake1[0][2] == 7)){
					  snake1_next_dir = 6;
				  } else {
					  snake1_next_dir = 0;
				  }
				  break;
			  case 1: // Abajo
				  snake1_next_x = snake1[0][0];
				  if (snake1[0][1] == HEIGHT-1){
					  if (infinity_mode){
						  snake1_next_y = 0;
					  } else {
						  snake1_lost = 1;
					  }
				  } else {
					  snake1_next_y += 1;
				  }
				  if ((snake1[0][2] == 2)||(snake1[0][2] == 4)||(snake1[0][2] == 6)){
					  snake1_next_dir = 5;
				  } else if ((snake1[0][2] == 3)||(snake1[0][2] == 5)||(snake1[0][2] == 7)){
					  snake1_next_dir = 4;
				  } else {
					  snake1_next_dir = 1;
				  }
				  break;
			  case 2: // Izquierda
				  snake1_next_y = snake1[0][1];
				  if (snake1[0][0] == 0){
					  if (infinity_mode){
						  snake1_next_x = WIDTH-1;
					  } else {
						  snake1_lost = 1;
					  }
				  } else {
					  snake1_next_x -= 1;
				  }
				  if ((snake1[0][2] == 0)||(snake1[0][2] == 7)||(snake1[0][2] == 6)){
					  snake1_next_dir = 4;
				  } else if ((snake1[0][2] == 1)||(snake1[0][2] == 4)||(snake1[0][2] == 5)){
					  snake1_next_dir = 6;
				  } else {
					  snake1_next_dir = 2;
				  }
				  break;
			  case 3: // Derecha
				  snake1_next_y = snake1[0][1];
				  if (snake1[0][0] == WIDTH-1){
					  if (infinity_mode){
						  snake1_next_x = 0;
					  } else {
						  snake1_lost = 1;
					  }
				  } else {
					  snake1_next_x += 1;
				  }
				  if ((snake1[0][2] == 0)||(snake1[0][2] == 7)||(snake1[0][2] == 6)){
					  snake1_next_dir = 5;
				  } else if ((snake1[0][2] == 1)||(snake1[0][2] == 4)||(snake1[0][2] == 5)){
					  snake1_next_dir = 7;
				  } else {
					  snake1_next_dir = 3;
				  }
				  break;
			  }

			  // LOGICA MOVIMIENTO SERPIENTE 2

			  snake2_prev_x = snake2[len_snake2-1][0];
			  snake2_prev_y = snake2[len_snake2-1][1];
			  //snake2_prev_dir = snake2[len_snake2-1][2];
			  if (((snake2[len_snake2-2][1] - snake2[len_snake2-3][1]) == 1)||((snake2[len_snake2-2][1] - snake2[len_snake2-3][1]) == -(HEIGHT-1))){
				  snake2_prev_dir = 0;
			  } else if (((snake2[len_snake2-2][1] - snake2[len_snake2-3][1]) == -1)||((snake2[len_snake2-2][1] - snake2[len_snake2-3][1]) == (HEIGHT-1))){
				  snake2_prev_dir = 1;
			  } else if (((snake2[len_snake2-2][0] - snake2[len_snake2-3][0]) == 1)||((snake2[len_snake2-2][0] - snake2[len_snake2-3][0]) == -(WIDTH-1))){
				  snake2_prev_dir = 2;
			  } else {
				  snake2_prev_dir = 3;
			  }
			  switch (dir_snake2){
			  case 0: // Arriba
				  snake2_next_x = snake2[0][0];
				  if (snake2[0][1] == 0){
					  if (infinity_mode){
						  snake2_next_y = HEIGHT-1;
					  } else {
						  snake2_lost = 1;
					  }
				  } else {
					  snake2_next_y -= 1;
				  }
				  if ((snake2[0][2] == 2)||(snake2[0][2] == 4)||(snake2[0][2] == 6)){
					  snake2_next_dir = 7;
				  } else if ((snake2[0][2] == 3)||(snake2[0][2] == 5)||(snake2[0][2] == 7)){
					  snake2_next_dir = 6;
				  } else {
					  snake2_next_dir = 0;
				  }
				  break;
			  case 1: // Abajo
				  snake2_next_x = snake2[0][0];
				  if (snake2[0][1] == HEIGHT-1){
					  if (infinity_mode){
						  snake2_next_y = 0;
					  } else {
						  snake2_lost = 1;
					  }
				  } else {
					  snake2_next_y += 1;
				  }
				  if ((snake2[0][2] == 2)||(snake2[0][2] == 4)||(snake2[0][2] == 6)){
					  snake2_next_dir = 5;
				  } else if ((snake2[0][2] == 3)||(snake2[0][2] == 5)||(snake2[0][2] == 7)){
					  snake2_next_dir = 4;
				  } else {
					  snake2_next_dir = 1;
				  }
				  break;
			  case 2: // Izquierda
				  snake2_next_y = snake2[0][1];
				  if (snake2[0][0] == 0){
					  if (infinity_mode){
						  snake2_next_x = WIDTH-1;
					  } else {
						  snake2_lost = 1;
					  }
				  } else {
					  snake2_next_x -= 1;
				  }
				  if ((snake2[0][2] == 0)||(snake2[0][2] == 7)||(snake2[0][2] == 6)){
					  snake2_next_dir = 4;
				  } else if ((snake2[0][2] == 1)||(snake2[0][2] == 4)||(snake2[0][2] == 5)){
					  snake2_next_dir = 6;
				  } else {
					  snake2_next_dir = 2;
				  }
				  break;
			  case 3: // Derecha
				  snake2_next_y = snake2[0][1];
				  if (snake2[0][0] == WIDTH-1){
					  if (infinity_mode){
						  snake2_next_x = 0;
					  } else {
						  snake2_lost = 1;
					  }
				  } else {
					  snake2_next_x += 1;
				  }
				  if ((snake2[0][2] == 0)||(snake2[0][2] == 7)||(snake2[0][2] == 6)){
					  snake2_next_dir = 5;
				  } else if ((snake2[0][2] == 1)||(snake2[0][2] == 4)||(snake2[0][2] == 5)){
					  snake2_next_dir = 7;
				  } else {
					  snake2_next_dir = 3;
				  }
				  break;
			  }

			  // VERIFICAR COLISION SNAKES
			  snake1_next_cell_value = map[snake1_next_y][snake1_next_x] - map[snake1_next_y][snake1_next_x]%2;
			  snake2_next_cell_value = map[snake2_next_y][snake2_next_x] - map[snake2_next_y][snake2_next_x]%2;

			  if ((snake1_next_y != snake2_prev_y)||(snake1_next_x != snake2_prev_x)){
				  if ((snake1_next_cell_value == 2) || (snake1_next_cell_value == 4)){
					  //game_over = 1;
					  snake1_lost = 1;
				  }
			  }
			  if ((snake2_next_y != snake1_prev_y)||(snake2_next_x != snake1_prev_x)){
				  if ((snake2_next_cell_value == 2) || (snake2_next_cell_value == 4)){
					  //game_over = 1;
					  snake2_lost = 1;
				  }
			  }
			  if ((snake1_next_x == snake2_next_x)&&(snake1_next_y == snake2_next_y)){
				  snake1_lost = 1;
				  snake2_lost = 1;
			  }
			  if (snake1_next_cell_value == 6){
				  snake1_eats = 1;
			  }
			  if (snake2_next_cell_value == 6){
				  snake2_eats = 1;
			  }

		  }




		  if (game_over == 0){

			  // ACTUALIZAR MAPA
			  if (snake1_lost == 0){
				  if (snake1_eats == 0){
					  map[snake1_prev_y][snake1_prev_x] -= 2;
				  }
				  if (snake1_eats == 1){
					  map[snake1_next_y][snake1_next_x] -= 6;
				  }
				  map[snake1_next_y][snake1_next_x] += 2;
			  }
			  if (snake2_lost == 0){
				  if (snake2_eats == 0){
					  map[snake2_prev_y][snake2_prev_x] -= 4;
				  }
				  if (snake2_eats == 1){
					  map[snake2_next_y][snake2_next_x] -= 6;
				  }
				  map[snake2_next_y][snake2_next_x] += 4;
			  }

			  // MOVER SERPIENTES
			  // SERPIENTE 1
			  if (snake1_lost == 0){
				  if (snake1_eats == 1){
					  len_snake1 += 1;
				  }
				  for (int i = len_snake1-1; i > 0; i--){
					  snake1[i][0] = snake1[i-1][0];
					  snake1[i][1] = snake1[i-1][1];
					  snake1[i][2] = snake1[i-1][2];
				  }
				  snake1[0][0] = snake1_next_x;
				  snake1[0][1] = snake1_next_y;
				  snake1[0][2] = dir_snake1;
				  snake1[1][2] = snake1_next_dir;
			  }

			  // SERPIENTE 2
			  if (snake2_lost == 0){
				  if (snake2_eats == 1){
					  len_snake2 += 1;
				  }
				  for (int i = len_snake2-1; i > 0; i--){
					  snake2[i][0] = snake2[i-1][0];
					  snake2[i][1] = snake2[i-1][1];
					  snake2[i][2] = snake2[i-1][2];
				  }
				  snake2[0][0] = snake2_next_x;
				  snake2[0][1] = snake2_next_y;
				  snake2[0][2] = dir_snake2;
				  snake2[1][2] = snake2_next_dir;
			  }

			  // PINTAR COLAS
			  // SERPIENTE 1
			  if ((snake1_lost == 0)&&(snake1_eats == 0)){
				  snake1_tail_shade = map[snake1_prev_y][snake1_prev_x] % 2;
				  if (snake1_tail_shade == 1){
					  FillRect(snake1_prev_x*SQ_SIZE, snake1_prev_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x04C1);
				  } else {
					  FillRect(snake1_prev_x*SQ_SIZE, snake1_prev_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x0660);
				  }
				  snake1_current_cell_value = map[snake1[len_snake1-1][1]][snake1[len_snake1-1][0]] % 2;
				  switch (snake1_prev_dir){
				  case 0:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_0, 0, 0);
					  }
					  break;
				  case 1:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_1, 1, 0);
					  } else {
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_up_0, 1, 0);
					  }
					  break;
				  case 2:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_right_1, 0, 1);
					  } else {
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_right_0, 0, 1);
					  }
					  break;
				  case 3:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[len_snake1-1][0]*SQ_SIZE, snake1[len_snake1-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake1_right_0, 0, 0);
					  }
					  break;
				  }
			  }

			  // SERPIENTE 2
			  if ((snake2_lost == 0)&&(snake2_eats == 0)){
				  snake2_tail_shade = map[snake2_prev_y][snake2_prev_x] % 2;
				  if (snake2_tail_shade == 1){
					  FillRect(snake2_prev_x*SQ_SIZE, snake2_prev_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x04C1);
				  } else {
					  FillRect(snake2_prev_x*SQ_SIZE, snake2_prev_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x0660);
				  }
				  snake2_current_cell_value = map[snake2[len_snake2-1][1]][snake2[len_snake2-1][0]] % 2;
				  switch (snake2_prev_dir){
				  case 0:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_0, 0, 0);
					  }
					  break;
				  case 1:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_1, 1, 0);
					  } else {
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_up_0, 1, 0);
					  }
					  break;
				  case 2:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_right_1, 0, 1);
					  } else {
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_right_0, 0, 1);
					  }
					  break;
				  case 3:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[len_snake2-1][0]*SQ_SIZE, snake2[len_snake2-1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, tail_snake2_right_0, 0, 0);
					  }
					  break;
				  }
			  }
			  if ((snake1_eats == 1)||(snake2_eats == 1)){
				  sonido = 'c';
				  HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
			  }
			  if (snake1_eats == 1){
				  orange_count -= 1;
				  snake1_eats = 0;
			  }
			  if (snake2_eats == 1){
				  orange_count -= 1;
				  snake2_eats = 0;
			  }


			  //ANALIZAR ZONA VECINA A LAS CABEZAS POR NARANJAS
			  //SERPIENTE 1
			  uint8_t break_check_orange_snake1 = 0;
			  uint8_t check_orange_around_snake1;
			  /*for (int j = 0; j < 3; j++){
				  uint8_t pos_y_snake1;
				  if (snake1_next_y-1+j < 0) {
					  pos_y_snake1 = HEIGHT-1;
				  } else if (snake1_next_y-1+j > HEIGHT-1) {
					  pos_y_snake1 = 0;
				  } else {
					  pos_y_snake1 = snake1_next_y-1+j;
				  }
				  for (int i = 0; i < 3; i++){
					  uint8_t pos_x_snake1;
					  if (snake1_next_x-1+i < 0) {
						  pos_x_snake1 = WIDTH-1;
					  } else if (snake1_next_x-1+i > WIDTH-1) {
						  pos_x_snake1 = 0;
					  } else {
						  pos_x_snake1 = snake1_next_x-1+i;
					  }
					  check_orange_around_snake1 = map[pos_y_snake1][pos_x_snake1] - map[pos_y_snake1][pos_x_snake1]%2;
					  if (check_orange_around_snake1 == 6){
						  break_check_orange_snake1 = 1;
						  break;
					  }
				  }
				  if (break_check_orange_snake1 == 1){
					  break;
				  }
			  }*/
			  for (int i = 0; i < 3; i+=2){
				  uint8_t pos_y_snake1;
				  if (snake1_next_y-1+i < 0) {
					  if (infinity_mode){
						  pos_y_snake1 = HEIGHT-1;
					  } else {
						  pos_y_snake1 = snake1_next_y;
					  }
				  } else if (snake1_next_y-1+i > HEIGHT-1) {
					  if (infinity_mode){
						  pos_y_snake1 = 0;
					  } else {
						  pos_y_snake1 = snake1_next_y;
					  }
				  } else {
					  pos_y_snake1 = snake1_next_y-1+i;
				  }
				  uint8_t pos_x_snake1;
				  if (snake1_next_x-1+i < 0) {
					  if (infinity_mode){
						  pos_x_snake1 = WIDTH-1;
					  } else {
						  pos_x_snake1 = snake1_next_x;
					  }
				  } else if (snake1_next_x-1+i > WIDTH-1) {
					  if (infinity_mode){
						  pos_x_snake1 = 0;
					  } else {
						  pos_x_snake1 = snake1_next_x;
					  }
				  } else {
					  pos_x_snake1 = snake1_next_x-1+i;
				  }
				  check_orange_around_snake1 = map[snake1_next_y][pos_x_snake1] - map[snake1_next_y][pos_x_snake1]%2;
				  if (check_orange_around_snake1 == 6){
					  break_check_orange_snake1 = 1;
					  break;
				  }
				  check_orange_around_snake1 = map[pos_y_snake1][snake1_next_x] - map[pos_y_snake1][snake1_next_x]%2;
				  if (check_orange_around_snake1 == 6){
					  break_check_orange_snake1 = 1;
					  break;
				  }
			  }
			  snake1_orange_around = break_check_orange_snake1 ? 1 : 0;

			  //SERPIENTE 2
			  uint8_t break_check_orange_snake2 = 0;
			  uint8_t check_orange_around_snake2;
			  /*for (int j = 0; j < 3; j++){
				  uint8_t pos_y_snake2;
				  if (snake2_next_y-1+j < 0) {
					  pos_y_snake2 = HEIGHT-1;
				  } else if (snake2_next_y-1+j > HEIGHT-1) {
					  pos_y_snake2 = 0;
				  } else {
					  pos_y_snake2 = snake2_next_y-1+j;
				  }
				  for (int i = 0; i < 3; i++){
					  uint8_t pos_x_snake2;
					  if (snake2_next_x-1+i < 0) {
						  pos_x_snake2 = WIDTH-1;
					  } else if (snake2_next_x-1+i > WIDTH-1) {
						  pos_x_snake2 = 0;
					  } else {
						  pos_x_snake2 = snake2_next_x-1+i;
					  }
					  check_orange_around_snake2 = map[pos_y_snake2][pos_x_snake2] - map[pos_y_snake2][pos_x_snake2]%2;
					  if (check_orange_around_snake2 == 6){
						  break_check_orange_snake2 = 1;
						  break;
					  }
				  }
				  if (break_check_orange_snake2 == 1){
					  break;
				  }
			  }*/
			  for (int i = 0; i < 3; i+=2){
			  uint8_t pos_y_snake2;
			  if (snake2_next_y-1+i < 0) {
				  if (infinity_mode){
					  pos_y_snake2 = HEIGHT-1;
				  } else {
					  pos_y_snake2 = snake2_next_y;
				  }
			  } else if (snake2_next_y-1+i > HEIGHT-1) {
				  if (infinity_mode){
					  pos_y_snake2 = 0;
				  } else {
					  pos_y_snake2 = snake2_next_y;
				  }
			  } else {
				  pos_y_snake2 = snake2_next_y-1+i;
			  }
			  uint8_t pos_x_snake2;
			  if (snake2_next_x-1+i < 0) {
				  if (infinity_mode){
					  pos_x_snake2 = WIDTH-1;
				  } else {
					  pos_x_snake2 = snake2_next_x;
				  }
			  } else if (snake2_next_x-1+i > WIDTH-1) {
				  if (infinity_mode){
					  pos_x_snake2 = 0;
				  } else {
					  pos_x_snake2 = snake2_next_x;
				  }
			  } else {
				  pos_x_snake2 = snake2_next_x-1+i;
			  }
			  check_orange_around_snake2 = map[snake2_next_y][pos_x_snake2] - map[snake2_next_y][pos_x_snake2]%2;
			  if (check_orange_around_snake2 == 6){
				  break_check_orange_snake2 = 1;
				  break;
			  }
			  check_orange_around_snake2 = map[pos_y_snake2][snake2_next_x] - map[pos_y_snake2][snake2_next_x]%2;
			  if (check_orange_around_snake2 == 6){
				  break_check_orange_snake2 = 1;
				  break;
			  }
		  }
			  snake2_orange_around = break_check_orange_snake2 ? 1 : 0;

			  // PINTAR CABEZAS
			  if (snake1_lost == 0){
				  //FillRect(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0x0019);
				  snake1_current_cell_value = map[snake1_next_y][snake1_next_x] % 2;
				  switch (dir_snake1){
				  case 0:
					  if (snake1_current_cell_value == 1){
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_eats_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_1, 0, 0);
						  }
					  } else {
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_eats_0, 0, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_0, 0, 0);
						  }
					  }
					  break;
				  case 1:
					  if (snake1_current_cell_value == 1){
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_eats_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_1, 1, 0);
						  }
					  } else {
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_eats_0, 1, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_0, 1, 0);
						  }
					  }
					  break;
				  case 2:
					  if (snake1_current_cell_value == 1){
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_eats_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_1, 0, 1);
						  }
					  } else {
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_eats_0, 0, 1);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_0, 0, 1);
						  }
					  }
					  break;
				  case 3:
					  if (snake1_current_cell_value == 1){
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_eats_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_1, 0, 0);
						  }
					  } else {
						  if (snake1_orange_around == 1){
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_eats_0, 0, 0);
						  } else {
							  LCD_Bitmap(snake1_next_x*SQ_SIZE, snake1_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_0, 0, 0);
						  }
					  }
					  break;
				  }
				  //PINTAR CUERPO SIGUIENTE A LA CABEZA
				  snake1_current_cell_value = map[snake1[1][1]][snake1[1][0]] % 2;
				  switch (snake1[1][2]){
				  case 0:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_0, 0, 0);
					  }
					  break;
				  case 1:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_up_0, 0, 0);
					  }
					  break;
				  case 2:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_right_0, 0, 0);
					  }
					  break;
				  case 3:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake1_right_0, 0, 0);
					  }
					  break;
				  case 4:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_1, 1, 1);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_0, 1, 1);
					  }
					  break;
				  case 5:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_1, 1, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_0, 1, 0);
					  }
					  break;
				  case 6:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_1, 0, 1);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_0, 0, 1);
					  }
					  break;
				  case 7:
					  if (snake1_current_cell_value == 1){
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake1[1][0]*SQ_SIZE, snake1[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_0, 0, 0);
					  }
					  break;
				  }
			  }
			  if (snake2_lost == 0){
				  //FillRect(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0xF800);
				  snake2_current_cell_value = map[snake2_next_y][snake2_next_x] % 2;
				  switch (dir_snake2){
				  case 0:
					  if (snake2_current_cell_value == 1){
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_eats_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_1, 0, 0);
						  }
					  } else {
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_eats_0, 0, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_0, 0, 0);
						  }
					  }
					  break;
				  case 1:
					  if (snake2_current_cell_value == 1){
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_eats_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_1, 1, 0);
						  }
					  } else {
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_eats_0, 1, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_0, 1, 0);
						  }
					  }
					  break;
				  case 2:
					  if (snake2_current_cell_value == 1){
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_eats_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_1, 0, 1);
						  }
					  } else {
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_eats_0, 0, 1);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_0, 0, 1);
						  }
					  }
					  break;
				  case 3:
					  if (snake2_current_cell_value == 1){
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_eats_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_1, 0, 0);
						  }
					  } else {
						  if (snake2_orange_around == 1){
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_eats_0, 0, 0);
						  } else {
							  LCD_Bitmap(snake2_next_x*SQ_SIZE, snake2_next_y*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_0, 0, 0);
						  }
					  }
					  break;
				  }
				  //PINTAR CUERPO SIGUIENTE A LA CABEZA
				  snake2_current_cell_value = map[snake2[1][1]][snake2[1][0]] % 2;
				  switch (snake2[1][2]){
				  case 0:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_0, 0, 0);
					  }
					  break;
				  case 1:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_up_0, 0, 0);
					  }
					  break;
				  case 2:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_right_0, 0, 0);
					  }
					  break;
				  case 3:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_right_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, body_snake2_right_0, 0, 0);
					  }
					  break;
				  case 4:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_1, 1, 1);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_0, 1, 1);
					  }
					  break;
				  case 5:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_1, 1, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_0, 1, 0);
					  }
					  break;
				  case 6:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_1, 0, 1);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_0, 0, 1);
					  }
					  break;
				  case 7:
					  if (snake2_current_cell_value == 1){
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_1, 0, 0);
					  } else {
						  LCD_Bitmap(snake2[1][0]*SQ_SIZE, snake2[1][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_0, 0, 0);
					  }
					  break;
				  }
			  }

			  // DEFINIR JUEGO PERDIDO
			  if (snake1_lost || snake2_lost){
				  game_over = 1;
				  sonido = 'p';
				  HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
			  }

		  }
		  if ((game_over == 1)&&(menu == 0)&&(stop_animations == 0)){
			  stop_animations = 1;
			  if (snake1_lost == 1){
				  /*for (int i = 0; i < len_snake1; i++){
					  FillRect(snake1[i][0]*SQ_SIZE, snake1[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0xFFE0);
				  }*/
				  snake1_current_cell_value = map[snake1[0][1]][snake1[0][0]] % 2;
				  switch (dir_snake1){
				  case 0:
					  if ((snake1[0][0] - snake1[1][0] == 1)||(snake1[0][0] - snake1[1][0] == -(WIDTH-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_1, 1, 1);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_0, 1, 1);
						  }
					  } else if ((snake1[0][0] - snake1[1][0] == -1)||(snake1[0][0] - snake1[1][0] == (WIDTH-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_0, 1, 0);
						  }
					  } else {
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_crash_0, 0, 0);
						  }
					  }
					  break;
				  case 1:
					  if ((snake1[0][0] - snake1[1][0] == 1)||(snake1[0][0] - snake1[1][0] == -(WIDTH-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_0, 0, 1);
						  }
					  } else if ((snake1[0][0] - snake1[1][0] == -1)||(snake1[0][0] - snake1[1][0] == (WIDTH-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_right_bottom_crash_0, 0, 0);
						  }
					  } else {
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_up_crash_0, 1, 0);
						  }
					  }
					  break;
				  case 2:
					  if ((snake1[0][1] - snake1[1][1] == 1)||(snake1[0][1] - snake1[1][1] == -(HEIGHT-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_0, 0, 1);
						  }
					  } else if ((snake1[0][1] - snake1[1][1] == -1)||(snake1[0][1] - snake1[1][1] == (HEIGHT-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_1, 1, 1);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_0, 1, 1);
						  }
					  } else {
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_crash_0, 0, 1);
						  }
					  }
					  break;
				  case 3:
					  if ((snake1[0][1] - snake1[1][1] == 1)||(snake1[0][1] - snake1[1][1] == -(HEIGHT-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_0, 0, 0);
						  }
					  } else if ((snake1[0][1] - snake1[1][1] == -1)||(snake1[0][1] - snake1[1][1] == (HEIGHT-1))){
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake1_top_right_crash_0, 1, 0);
						  }
					  } else {
						  if (snake1_current_cell_value == 1){
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake1[0][0]*SQ_SIZE, snake1[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake1_right_crash_0, 0, 0);
						  }
					  }
					  break;
				  }
			  }
			  if (snake2_lost == 1){
				  /*for (int i = 0; i < len_snake2; i++){
					  FillRect(snake2[i][0]*SQ_SIZE, snake2[i][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0xFFE0);
				  }*/
				  snake2_current_cell_value = map[snake2[0][1]][snake2[0][0]] % 2;
				  switch (dir_snake2){
				  case 0:
					  if ((snake2[0][0] - snake2[1][0] == 1)||(snake2[0][0] - snake2[1][0] == -(WIDTH-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_1, 1, 1);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_0, 1, 1);
						  }
					  } else if ((snake2[0][0] - snake2[1][0] == -1)||(snake2[0][0] - snake2[1][0] == (WIDTH-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_0, 1, 0);
						  }
					  } else {
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_crash_0, 0, 0);
						  }
					  }
					  break;
				  case 1:
					  if ((snake2[0][0] - snake2[1][0] == 1)||(snake2[0][0] - snake2[1][0] == -(WIDTH-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_0, 0, 1);
						  }
					  } else if ((snake2[0][0] - snake2[1][0] == -1)||(snake2[0][0] - snake2[1][0] == (WIDTH-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_right_bottom_crash_0, 0, 0);
						  }
					  } else {
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_up_crash_0, 1, 0);
						  }
					  }
					  break;
				  case 2:
					  if ((snake2[0][1] - snake2[1][1] == 1)||(snake2[0][1] - snake2[1][1] == -(HEIGHT-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_0, 0, 1);
						  }
					  } else if ((snake2[0][1] - snake2[1][1] == -1)||(snake2[0][1] - snake2[1][1] == (HEIGHT-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_1, 1, 1);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_0, 1, 1);
						  }
					  } else {
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_crash_1, 0, 1);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_crash_0, 0, 1);
						  }
					  }
					  break;
				  case 3:
					  if ((snake2[0][1] - snake2[1][1] == 1)||(snake2[0][1] - snake2[1][1] == -(HEIGHT-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_0, 0, 0);
						  }
					  } else if ((snake2[0][1] - snake2[1][1] == -1)||(snake2[0][1] - snake2[1][1] == (HEIGHT-1))){
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_1, 1, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, corner_snake2_top_right_crash_0, 1, 0);
						  }
					  } else {
						  if (snake2_current_cell_value == 1){
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_crash_1, 0, 0);
						  } else {
							  LCD_Bitmap(snake2[0][0]*SQ_SIZE, snake2[0][1]*SQ_SIZE, SQ_SIZE, SQ_SIZE, head_snake2_right_crash_0, 0, 0);
						  }
					  }
					  break;
				  }
			  }
		  }

		  // GENERACIÓN NARANJAS
		  //Primer test
		  /*int orange_random_x = get_random(0, WIDTH-1);
		  int orange_random_y = get_random(0, HEIGHT-1);
		  int orange_random_diag_dir = get_random(0, 3);
		  int orange_position_valid = 0;
		  int orange_possible_cell;
		  while (orange_position_valid == 0){
			  orange_possible_cell = map[orange_random_y][orange_random_x] - map[orange_random_y][orange_random_x]%2;
			  if ((orange_possible_cell == 2)||(orange_possible_cell == 4)||(orange_possible_cell == 6)){
				  switch (orange_random_diag_dir  )
			  }
		  }*/
		  //Segundo Test
		  /*uint32_t currentTick = HAL_GetTick();
		  if ((currentTick - lastITick_orange)>600){
			  lastITick_orange = currentTick;
			  if (orange_count < 40){
				  available_cells = HEIGHT*WIDTH-len_snake1-len_snake2-orange_count;
				  orange_random_pos = get_random(0, available_cells-1);
				  cell_count = -1;
				  positioned_orange = 0;
				  for  (int i = 0; i<HEIGHT; i++){
					  for (int j = 0; j<WIDTH; j++){
						  possible_orange_cell = map[i][j] - map[i][j]%2;
						  if ((possible_orange_cell != 6)&&(possible_orange_cell != 4)&&(possible_orange_cell != 2)){
							  cell_count += 1;
						  }
						  if (cell_count == orange_random_pos){
							  map[i][j] += 6;
							  if (map[i][j]%2 == 1){
								  LCD_Bitmap(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, orange_1, 0, 0);
							  } else {
								  LCD_Bitmap(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, orange_0, 0, 0);
							  }
							  //FillRect(j*SQ_SIZE, i*SQ_SIZE, SQ_SIZE, SQ_SIZE, 0xFC00);
							  orange_count += 1;
							  positioned_orange = 1;
						  }
						  if (positioned_orange == 1){
							  break;
						  }
					  }
					  if (positioned_orange == 1){
						  break;
					  }
				  }
			  }
		  }*/









		  //PRUEBA DE MOVIMIENTO: ALGORITMO DE MOVIMIENTO ALEATORIO
		  /*if (game_over == 0){
			  int mover_snake1 = get_random(0, 3);
			  if (mover_snake1 == 0){
				  //SNAKE 1
				  int random_dir_snake1 = get_random(0, 3);

				  switch (dir_snake1){
				  case 0: // Arriba
					  if (random_dir_snake1 == 1){
						  random_dir_snake1 += 1;
					  }

					  break;
				  case 1: // Abajo
					  if (random_dir_snake1 == 0){
						  random_dir_snake1 += 1;
					  }
					  break;
				  case 2: // Izquierda
					  if (random_dir_snake1 == 3){
						  random_dir_snake1 -= 1;
					  }
					  break;
				  case 3: // Derecha
					  if (random_dir_snake1 == 2){
						  random_dir_snake1 += 1;
					  }
					  break;
				  }

				  dir_snake1 = random_dir_snake1;
			  }
			  int mover_snake2 = get_random(0, 3);
			  if (mover_snake2 == 0){
				  //SNAKE 2
				  int random_dir_snake2 = get_random(0, 3);

				  switch (dir_snake2){
				  case 0:
					  if (random_dir_snake2 == 1){
						  random_dir_snake2 += 1;
					  }
					  break;
				  case 1:
					  if (random_dir_snake2 == 0){
						  random_dir_snake2 += 1;
					  }
					  break;
				  case 2:
					  if (random_dir_snake2 == 3){
						  random_dir_snake2 -= 1;
					  }
					  break;
				  case 3:
					  if (random_dir_snake2 == 2){
						  random_dir_snake2 += 1;
					  }
					  break;
				  }

				  dir_snake2 = random_dir_snake2;
			  }
		  }*/

		  if ((game_over == 1)&&(menu == 0)){
			  if (prueba_iniciar_conteo == 0){
				  prueba_iniciar_conteo = 1;

				  if ((snake1_lost)&&(snake2_lost)){
					  fres = f_mount(&fs, "", 0);
					  if (fres == FR_OK){
						  fres = f_open(&fil, "snakes_lose.bin", FA_READ);
						  if (fres == FR_OK) {
							  for (uint16_t row = 0; row < 240; row+=2){
								  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
								  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
									  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
								  }
							  }
							f_close(&fil);


						}
						  f_mount(NULL, "", 1);
					  }
					  sonido = 'l';
					  HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
				  } else if (snake2_lost){
					  fres = f_mount(&fs, "", 0);
					  if (fres == FR_OK){
						  fres = f_open(&fil, "snake1_wins.bin", FA_READ);
						  if (fres == FR_OK) {
							  for (uint16_t row = 0; row < 240; row+=2){
								  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
								  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
									  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
								  }
							  }
							f_close(&fil);


						}
						  f_mount(NULL, "", 1);
					  }
					  sonido = 'w';
					  HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
				  } else {
					  fres = f_mount(&fs, "", 0);
					  if (fres == FR_OK){
						  fres = f_open(&fil, "snake2_wins.bin", FA_READ);
						  if (fres == FR_OK) {
							  for (uint16_t row = 0; row < 240; row+=2){
								  fres = f_read(&fil, menu_buf, sizeof(menu_buf), &br);
								  if ((fres == FR_OK)&&(br == sizeof(menu_buf))){
									  LCD_Bitmap(0, row, 320, 2, menu_buf, 0, 0);
								  }
							  }
							f_close(&fil);


						}
						  f_mount(NULL, "", 1);
					  }
					  sonido = 'w';
					  HAL_UART_Transmit_DMA(&huart3, &sonido, 1);
				  }

				  lastInterruptTick_prueba = HAL_GetTick();
			  }
		  }
		  if (prueba_iniciar_conteo == 1){
			  uint32_t currentmillis_prueba = HAL_GetTick();
			  if ((currentmillis_prueba - lastInterruptTick_prueba)>2000){
				  prueba_iniciar_conteo = 0;
				  display_menu();

			  }
		  }
		  //PRUEBA JUEGO INFINITO
		  /*if (game_over == 1){
			  if (prueba_iniciar_conteo == 0){
				  prueba_iniciar_conteo = 1;
				  lastInterruptTick_prueba = HAL_GetTick();
			  }
		  }
		  if (prueba_iniciar_conteo == 1){
			  uint32_t currentmillis_prueba = HAL_GetTick();
			  if ((currentmillis_prueba - lastInterruptTick_prueba)>1500){
				  prueba_iniciar_conteo = 0;
				  restart_game();

			  }
		  }*/

		  //PRUEBA DE CRECIMIENTO: ALEATORIO PARA AMBAS SERPIENTES
		  /*int crecer_snake1 = get_random(0, 4);
		  if (crecer_snake1 == 0){
			  len_snake1 += 1;
		  }
		  int crecer_snake2 = get_random(0, 4);
		  if (crecer_snake2 == 0){
			  len_snake2 += 1;
		  }*/
		  //PRUEBA DE MOVIMIENTO: SERPIENTE 1 Y 2 GIRA
		  /*if (ciclo_prueba == 3){
			  ciclo_prueba = 0;
			  switch (dir_snake1){
			  case 0:
				  dir_snake1 = 3;
				  break;
			  case 1:
				  dir_snake1 = 2;
				  break;
			  case 2:
				  dir_snake1 = 0;
				  break;
			  case 3:
				  dir_snake1 = 1;
				  break;
			  }
			  switch (dir_snake2){
			  case 0:
				  dir_snake2 = 2;
				  break;
			  case 1:
				  dir_snake2 = 3;
				  break;
			  case 2:
				  dir_snake2 = 1;
				  break;
			  case 3:
				  dir_snake2 = 0;
				  break;
			  }
		  }
		  ciclo_prueba += 1;*/

	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_SS_Pin */
  GPIO_InitStruct.Pin = SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(SD_SS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	//HAL_UART_Transmit(&huart1, buffer, sizeof(buffer), 1000);
	received_input = 1;
	HAL_UART_Receive_IT(&huart3, input_controles, 1);
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
