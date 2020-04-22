#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include "noise.h"

#define MAX_BUTTONS_NUM 3
#define uint8_t uint16_t

//using std::cout;			// - не ленивый неймспейс
//using std::endl;
//using std::cin;
using namespace std;		//ленивый
typedef struct buttons_state_t
{
	uint64_t time_last;
	uint16_t pin_state;
	//uint16_t pin_prev_state;
	uint16_t prev_state;
	uint16_t current_state;
	uint16_t changed;
	uint16_t delay_act;					//!!!!!
	uint16_t cnt;

} buttons_state_t;

enum
{
	BUTTON_NORMAL = 0,
	BUTTON_INVERTED,
	BUTTON_TOGGLE,
	TOGGLE_SWITCH,
	TOGGLE_SWITCH_ON,
	TOGGLE_SWITCH_OFF,

	POV1_UP,
	POV1_RIGHT,
	POV1_DOWN,
	POV1_LEFT,
	POV2_UP,
	POV2_RIGHT,
	POV2_DOWN,
	POV2_LEFT,
	POV3_UP,
	POV3_RIGHT,
	POV3_DOWN,
	POV3_LEFT,
	POV4_UP,
	POV4_RIGHT,
	POV4_DOWN,
	POV4_LEFT,

	ENCODER_INPUT_A,
	ENCODER_INPUT_B,

	RADIO_BUTTON1,
	RADIO_BUTTON2,
	RADIO_BUTTON3,
	RADIO_BUTTON4,

	SEQUENTIAL_TOGGLE,


};
typedef uint16_t button_type_t;

typedef struct button_t
{
	int8_t					physical_num;
	button_type_t 	type : 5;
	uint16_t					shift_modificator : 3;
	uint16_t					button_delay_number;				// :2
	//uint16_t					:0;

}	button_t;

typedef struct
{
	uint16_t						button_debounce_ms;					//uint16_t?
	uint16_t						toggle_press_time_ms;
	uint16_t						button_delay1_ms;						// config 6				
	uint16_t						button_delay2_ms;						// config 7
	uint16_t						button_delay3_ms;						// config 8
	button_t 						buttons[MAX_BUTTONS_NUM];
}dev_config_t;

button_t buttons[MAX_BUTTONS_NUM];
buttons_state_t 	buttons_state[MAX_BUTTONS_NUM];
dev_config_t p_dev_config;
uint16_t pov_buf;
void LogicalButtonProcessState(buttons_state_t * p_button_state, uint8_t * pov_buf, dev_config_t * p_dev_config, uint8_t num, uint32_t millis);

uint32_t GetTick()
{
	return clock();
}

void SequentialButtons_Init(dev_config_t * p_dev_config)
{
	for (uint8_t physical_num = 0; physical_num < MAX_BUTTONS_NUM; physical_num++)
	{
		for (uint8_t i = 0; i < MAX_BUTTONS_NUM; i++)
		{
			if (p_dev_config->buttons[i].type == SEQUENTIAL_TOGGLE &&
				p_dev_config->buttons[i].physical_num == physical_num)
			{
				buttons_state[i].current_state = 1;
				break;
			}
		}
	}
}

int main()
{
	setlocale(LC_ALL, "rus");
	clock_t delay = CLOCKS_PER_SEC * 10;
	p_dev_config.button_debounce_ms = 50;
	p_dev_config.button_delay1_ms = 100;
	p_dev_config.button_delay2_ms = 500;
	p_dev_config.button_delay3_ms = 2000;
	p_dev_config.toggle_press_time_ms = 200;

	p_dev_config.buttons[0].button_delay_number = 3;
	p_dev_config.buttons[0].physical_num = 0;
	p_dev_config.buttons[0].type = TOGGLE_SWITCH;
	p_dev_config.buttons[1].button_delay_number = 3;
	p_dev_config.buttons[1].physical_num = 1;
	p_dev_config.buttons[1].type = SEQUENTIAL_TOGGLE;
	p_dev_config.buttons[2].button_delay_number = 3;
	p_dev_config.buttons[2].physical_num = 2;
	p_dev_config.buttons[2].type = SEQUENTIAL_TOGGLE;
	//p_dev_config.buttons[3].button_delay_number = 3;
	//p_dev_config.buttons[3].physical_num = 0;
	//p_dev_config.buttons[3].type = SEQUENTIAL_BUTTON;
	SequentialButtons_Init(&p_dev_config);

	uint32_t millis = 0;
	while (1)
	{
		cout << "Enter millis: ";
		cin >> millis;
		cin.ignore();
		cout << "Enter pin_state: ";
		cin >> buttons_state[0].pin_state;
		cin.ignore();

		//millis += 100;

		buttons_state[2].pin_state = buttons_state[1].pin_state = buttons_state[0].pin_state;
		cout << millis << " - befor logic process  " << buttons_state[0].current_state << "  " << buttons_state[1].current_state << "  " << buttons_state[2].current_state << endl;
		LogicalButtonProcessState(&buttons_state[0], &pov_buf, &p_dev_config, 0, millis);
		//LogicalButtonProcessState(&buttons_state[1], &pov_buf, &p_dev_config, 1, millis);
		//LogicalButtonProcessState(&buttons_state[2], &pov_buf, &p_dev_config, 2, millis);
		//this_thread::sleep_for(chrono::milliseconds(700));
		cout << millis << " - AFTER logic process  " << buttons_state[0].current_state << "  " << buttons_state[1].current_state << "  " << buttons_state[2].current_state << endl;
	}


	return 0;
}





void LogicalButtonProcessState(buttons_state_t * p_button_state, uint8_t * pov_buf, dev_config_t * p_dev_config, uint8_t num, uint32_t millis)
{

	//get delay
	uint16_t tmp_button_delay;
	if (p_dev_config->buttons[num].button_delay_number)
	{
		switch (p_dev_config->buttons[num].button_delay_number)
		{
		case 1:
			tmp_button_delay = p_dev_config->button_delay1_ms;
			break;
		case 2:
			tmp_button_delay = p_dev_config->button_delay2_ms;
			break;
		case 3:
			tmp_button_delay = p_dev_config->button_delay3_ms;
			break;
		}

		// get max delay for sequential buttons
		if (p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE)
		{
			if (p_dev_config->button_delay1_ms > p_dev_config->button_delay2_ms && p_dev_config->button_delay1_ms > p_dev_config->button_delay3_ms)
				tmp_button_delay = p_dev_config->button_delay1_ms;
			else if (p_dev_config->button_delay2_ms > p_dev_config->button_delay1_ms && p_dev_config->button_delay2_ms > p_dev_config->button_delay3_ms)
				tmp_button_delay = p_dev_config->button_delay2_ms;
			else
				tmp_button_delay = p_dev_config->button_delay3_ms;
		}
	}

	// delay activated after debounce
	if ((p_dev_config->buttons[num].button_delay_number && !p_button_state->delay_act) || p_button_state->delay_act == 3)
	{
		// set timestamp if state changed
		if (!p_button_state->changed &&
			(p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE ? p_button_state->pin_state > p_button_state->prev_state : p_button_state->pin_state != p_button_state->prev_state))
		{
			p_button_state->time_last = millis;
			p_button_state->changed = 1;
		}
		// set delay activated after debounce if state have not changed
		else if (p_button_state->changed &&
			(p_dev_config->buttons[num].type == BUTTON_TOGGLE ? p_button_state->pin_state :
				p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE ? (p_button_state->pin_state && p_button_state->delay_act < 3) :
				p_button_state->pin_state != p_button_state->prev_state) &&
			millis - p_button_state->time_last > p_dev_config->button_debounce_ms && p_button_state->delay_act < 3) // <<< sequent 
		{
			p_button_state->delay_act = 1;
			//p_button_state->changed = 0;				//sequent
		}
		// reset if state changed during debounce period
		else if (p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE && !p_button_state->pin_state &&
			millis - p_button_state->time_last > p_dev_config->button_debounce_ms)
		{
			p_button_state->changed = 0;
			p_button_state->prev_state = 0;
			if (p_button_state->delay_act == 3) p_button_state->delay_act = 0;
		}


		else if (p_button_state->changed && p_button_state->delay_act != 1 && p_dev_config->buttons[num].type != SEQUENTIAL_TOGGLE &&
			millis - p_button_state->time_last > p_dev_config->button_debounce_ms)
		{
			p_button_state->changed = 0;
		}


		else if (p_dev_config->buttons[num].type == TOGGLE_SWITCH_ON && !p_button_state->pin_state && p_button_state->prev_state)
		{
			p_button_state->prev_state = 0;
			p_button_state->changed = 0;
		}
		else if (p_dev_config->buttons[num].type == TOGGLE_SWITCH_OFF && p_button_state->pin_state && !p_button_state->prev_state)
		{
			p_button_state->prev_state = 1;
			p_button_state->changed = 0;
		}
	}

	else if (!p_dev_config->buttons[num].button_delay_number || (p_button_state->delay_act == 1 && millis - p_button_state->time_last > tmp_button_delay) || p_button_state->delay_act == 2)
	{
		uint16_t tmp_debounce = p_dev_config->button_debounce_ms;

		// enable button after delay
		if (p_button_state->delay_act == 1 && millis - p_button_state->time_last < p_dev_config->toggle_press_time_ms + tmp_button_delay)
		{
			if (p_dev_config->buttons[num].type == TOGGLE_SWITCH_OFF)
			{
				p_button_state->pin_state = 0;
			}
			else 																			// SNIZU !!!!!!!!!!!!!
			{
				//p_button_state->pin_state = 1;
				if (p_dev_config->buttons[num].type == TOGGLE_SWITCH && (p_button_state->prev_state != p_button_state->pin_state ||
					(!p_button_state->pin_state && !p_button_state->current_state)))
				{
					p_button_state->prev_state ? (p_button_state->pin_state = 0) : (p_button_state->pin_state = 1);					// !!!!!!!!!!!!!!!
					//p_button_state->delay_act++;
				}
				else if (p_dev_config->buttons[num].type != TOGGLE_SWITCH) p_button_state->pin_state = 1;							// IF IF NAH

				if (p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE) p_button_state->delay_act++;
			}
			tmp_debounce = 0;
			p_button_state->changed = 1;
		}
		// disable delay
		else if (p_button_state->delay_act &&
			((p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE || p_dev_config->buttons[num].type == TOGGLE_SWITCH ||
				p_dev_config->buttons[num].type == TOGGLE_SWITCH_ON || p_dev_config->buttons[num].type == TOGGLE_SWITCH_OFF) ? 1 :
				p_button_state->pin_state != p_button_state->prev_state))
		{
			p_button_state->delay_act = 0;
			tmp_debounce = 0;
			p_button_state->time_last = millis - 1;
			//p_button_state->changed = 1;
			if (p_dev_config->buttons[num].type == SEQUENTIAL_TOGGLE)
			{
				p_button_state->changed = 1;			//!!!!
				p_button_state->delay_act = 3;
				//if(!p_button_state->pin_state) p_button_state->prev_state = 0;
				//p_button_state->pin_state = 0;
			}
			else if (p_dev_config->buttons[num].type == TOGGLE_SWITCH)
			{
				//p_button_state->prev_state = 1;
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			else if (p_dev_config->buttons[num].type == TOGGLE_SWITCH_ON)
			{
				p_button_state->prev_state = 1;
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			else if (p_dev_config->buttons[num].type == TOGGLE_SWITCH_OFF)
			{
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			else
				p_button_state->changed = 1;
		}


		switch (p_dev_config->buttons[num].type)
		{
		case BUTTON_INVERTED:
			// invert state for inverted button
			p_button_state->pin_state = !p_button_state->pin_state;
		case BUTTON_NORMAL:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->current_state = p_button_state->pin_state;
				p_button_state->prev_state = p_button_state->current_state;
				p_button_state->cnt += p_button_state->current_state;
			}
			// reset if state changed during debounce period
			else if (p_button_state->changed &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
			}
			break;

		case BUTTON_TOGGLE:
			// set timestamp if state changed to HIGH
			if (!p_button_state->changed &&
				p_button_state->pin_state > p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->prev_state = 1;
				p_button_state->current_state = !p_button_state->current_state;
				p_button_state->cnt++;

				p_button_state->delay_act = 0;				//????
			}
			// reset if state changed during debounce period
			else if (!p_button_state->pin_state && millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->prev_state = 0;
			}
			break;

		case TOGGLE_SWITCH:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->current_state = 1;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt++;
			}
			// release button after push time
			else if (millis - p_button_state->time_last > p_dev_config->toggle_press_time_ms + tmp_button_delay)
			{
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			break;

		case TOGGLE_SWITCH_ON:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state > p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->current_state = 1;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt++;
			}
			// release button after push time
			else if (!p_button_state->pin_state && p_button_state->prev_state)
			{
				p_button_state->prev_state = 0;
				p_button_state->changed = 0;
			}
			else if (millis - p_button_state->time_last > p_dev_config->toggle_press_time_ms + tmp_button_delay)			//toggle_press_time_ms + tmp_button_delay
			{
				p_button_state->prev_state = 1;
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			break;

		case TOGGLE_SWITCH_OFF:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state < p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->current_state = 1;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt++;
			}
			// release button after push time
			else if (p_button_state->pin_state && !p_button_state->prev_state)
			{
				p_button_state->prev_state = 1;
				p_button_state->changed = 0;
			}
			else if (millis - p_button_state->time_last > p_dev_config->toggle_press_time_ms + tmp_button_delay)
			{
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->current_state = 0;
				p_button_state->changed = 0;
			}
			break;

		case POV1_UP:
		case POV1_RIGHT:
		case POV1_DOWN:
		case POV1_LEFT:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				//p_button_state->current_state = p_button_state->pin_state;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt += p_button_state->pin_state;

				// set bit in povs data
				if ((p_dev_config->buttons[num].type) == POV1_UP)
				{
					pov_buf[0] &= ~(1 << 3);
					pov_buf[0] |= (p_button_state->pin_state << 3);
				}
				else if ((p_dev_config->buttons[num].type) == POV1_RIGHT)
				{
					pov_buf[0] &= ~(1 << 2);
					pov_buf[0] |= (p_button_state->pin_state << 2);
				}
				else if ((p_dev_config->buttons[num].type) == POV1_DOWN)
				{
					pov_buf[0] &= ~(1 << 1);
					pov_buf[0] |= (p_button_state->pin_state << 1);
				}
				else
				{
					pov_buf[0] &= ~(1 << 0);
					pov_buf[0] |= (p_button_state->pin_state << 0);
				}
			}
			// reset if state changed during debounce period
			else if (p_button_state->changed &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
			}
			break;

		case POV2_UP:
		case POV2_RIGHT:
		case POV2_DOWN:
		case POV2_LEFT:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				//p_button_state->current_state = p_button_state->pin_state;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt += p_button_state->pin_state;

				// set bit in povs data
				if ((p_dev_config->buttons[num].type) == POV2_UP)
				{
					pov_buf[1] &= ~(1 << 3);
					pov_buf[1] |= (p_button_state->pin_state << 3);
				}
				else if ((p_dev_config->buttons[num].type) == POV2_RIGHT)
				{
					pov_buf[1] &= ~(1 << 2);
					pov_buf[1] |= (p_button_state->pin_state << 2);
				}
				else if ((p_dev_config->buttons[num].type) == POV2_DOWN)
				{
					pov_buf[1] &= ~(1 << 1);
					pov_buf[1] |= (p_button_state->pin_state << 1);
				}
				else
				{
					pov_buf[1] &= ~(1 << 0);
					pov_buf[1] |= (p_button_state->pin_state << 0);
				}
			}
			// reset if state changed during debounce period
			else if (p_button_state->changed &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
			}
			break;

		case POV3_UP:
		case POV3_RIGHT:
		case POV3_DOWN:
		case POV3_LEFT:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				//p_button_state->current_state = p_button_state->pin_state;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt += p_button_state->pin_state;

				// set bit in povs data
				if ((p_dev_config->buttons[num].type) == POV3_UP)
				{
					pov_buf[2] &= ~(1 << 3);
					pov_buf[2] |= (p_button_state->pin_state << 3);
				}
				else if ((p_dev_config->buttons[num].type) == POV3_RIGHT)
				{
					pov_buf[2] &= ~(1 << 2);
					pov_buf[2] |= (p_button_state->pin_state << 2);
				}
				else if ((p_dev_config->buttons[num].type) == POV3_DOWN)
				{
					pov_buf[2] &= ~(1 << 1);
					pov_buf[2] |= (p_button_state->pin_state << 1);
				}
				else
				{
					pov_buf[2] &= ~(1 << 0);
					pov_buf[2] |= (p_button_state->pin_state << 0);
				}
			}
			// reset if state changed during debounce period
			else if (p_button_state->changed &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
			}
			break;

		case POV4_UP:
		case POV4_RIGHT:
		case POV4_DOWN:
		case POV4_LEFT:
			// set timestamp if state changed
			if (!p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state != p_button_state->prev_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				//p_button_state->current_state = p_button_state->pin_state;
				p_button_state->prev_state = p_button_state->pin_state;
				p_button_state->cnt += p_button_state->pin_state;

				// set bit in povs data
				if ((p_dev_config->buttons[num].type) == POV4_UP)
				{
					pov_buf[3] &= ~(1 << 3);
					pov_buf[3] |= (p_button_state->pin_state << 3);
				}
				else if ((p_dev_config->buttons[num].type) == POV4_RIGHT)
				{
					pov_buf[3] &= ~(1 << 2);
					pov_buf[3] |= (p_button_state->pin_state << 2);
				}
				else if ((p_dev_config->buttons[num].type) == POV4_DOWN)
				{
					pov_buf[3] &= ~(1 << 1);
					pov_buf[3] |= (p_button_state->pin_state << 1);
				}
				else
				{
					pov_buf[3] &= ~(1 << 0);
					pov_buf[3] |= (p_button_state->pin_state << 0);
				}
			}
			// reset if state changed during debounce period
			else if (p_button_state->changed &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
			}
			break;

		case RADIO_BUTTON1:
		case RADIO_BUTTON2:
		case RADIO_BUTTON3:
		case RADIO_BUTTON4:
			// set timestamp if state changed to HIGH
			if (!p_button_state->changed &&
				p_button_state->pin_state > p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->prev_state = 1;
				p_button_state->current_state = 1;
				p_button_state->cnt++;

				for (uint8_t i = 0; i < MAX_BUTTONS_NUM; i++)
				{
					if (p_dev_config->buttons[i].type == p_dev_config->buttons[num].type && i != num)
					{
						buttons_state[i].current_state = 0;
					}
				}
			}
			// reset if state changed during debounce period
			else if (!p_button_state->pin_state && millis - p_button_state->time_last > tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->prev_state = 0;
			}
			break;

		case SEQUENTIAL_TOGGLE:
			// set timestamp if state changed to HIGH
			if (!p_button_state->changed &&
				p_button_state->pin_state > p_button_state->prev_state)
			{
				p_button_state->time_last = millis;
				p_button_state->changed = 1;
			}
			// set state after debounce if state have not changed
			else if (p_button_state->changed && p_button_state->pin_state &&
				millis - p_button_state->time_last > tmp_debounce)
			{
				uint8_t is_first = 1;
				p_button_state->changed = 0;
				//p_button_state->cnt++;

				for (int16_t i = num - 1; i >= 0; i--)
				{
					if (p_dev_config->buttons[i].physical_num == p_dev_config->buttons[num].physical_num &&
						p_dev_config->buttons[i].type == SEQUENTIAL_TOGGLE)
					{
						is_first = 0;
						if (buttons_state[i].current_state && !buttons_state[i].prev_state)
						{
							buttons_state[i].current_state = 0;
							p_button_state->current_state = 1;
							p_button_state->prev_state = p_button_state->pin_state;
							p_button_state->time_last = millis;// - p_dev_config->button_delay3_ms;				//!!!!!
							break;
						}
					}
				}

				if (is_first && !p_button_state->current_state) // first in list and not pressed
				{
					// search for last in list
					for (int16_t i = MAX_BUTTONS_NUM; i > num; i--)
					{
						// check last
						if (p_dev_config->buttons[i].physical_num == p_dev_config->buttons[num].physical_num &&
							p_dev_config->buttons[i].type == SEQUENTIAL_TOGGLE)
						{
							if (!buttons_state[i].current_state) break;
							else if (!buttons_state[i].prev_state)
							{
								buttons_state[i].current_state = 0;
								p_button_state->current_state = 1;
								p_button_state->prev_state = p_button_state->pin_state;
								p_button_state->time_last = millis;// - p_dev_config->button_delay3_ms;				//!!!!!;
								break;
							}
						}
					}
				}

			}
			// reset if state changed during debounce period
			else if (!p_button_state->pin_state && millis - p_button_state->time_last > tmp_debounce)//tmp_debounce)
			{
				p_button_state->changed = 0;
				p_button_state->prev_state = 0;
			}
			break;

		default:
			break;
		}
	}
}




//class PC {																									//enum
//public:
//	enum PCState {
//		OFF,
//		ON,
//		SLEEP
//	};
//
//	PCState GetState() { 
//		return State; 
//	}
//	void SetState(PCState State) {
//		this->State = State;
//	}
//private:
//	PCState State;
//};
//
//enum Speed {
//	MIN=150,
//	RECOMEND=600,
//	MAX=800};
//
//int main() {
//	PC pc;
//	pc.SetState(PC::PCState::ON);
//
//	switch (pc.GetState())
//	{
//		case PC::PCState::OFF: 
//			cout << "PC OFF." << endl;
//			break;
//		case PC::PCState::ON:
//			cout << "PC ON." << endl;
//			break;
//		case PC::PCState::SLEEP:
//			cout << "PC SLEEP." << endl;
//			break;
//	}
//	/////
//
//	Speed sp = Speed::MAX;
//	cout << sp << endl;
//	int y = MIN;
//	/*PCState asd;
//	cout << "Enter 1 to shut down PC, 2 to sleep:_\b";
//	cin >> asd;
//	pc.SetState();*/
//
//}



//std::ifstream       file("resources/Database/Blocks/Blocks.csv");																	//DRAW FROM FILE
//char row;
//int col;
//std::vector<std::string>    m_data;
//std::string cell;
//std::string cell2;
//
//std::vector<std::string> GetItemStats(char row, int length)
//{
//	if (std::getline(file, cell, '\n') && std::getline(file, cell, row) && std::getline(file, cell, ';'))
//	{
//		std::getline(file, cell);
//		std::stringstream   lineStream(cell);
//
//		for (int i = 0; i < length; i++)
//		{
//			std::getline(lineStream, cell2, ';');
//			m_data.push_back(cell2);
//		}
//
//		//while (std::getline(lineStream, cell2, ';'))
//		//{
//		//	m_data.push_back(cell2);
//		//}
//	}
//
//	return m_data;
//}
//
//int main() {
//	std::vector<std::string> data;
//
//	data = GetItemStats('1', 4);
//
//}


//class Car {
//public:
//	string str = "car line";
//	void Drive() {
//		cout << "I drive" << endl;
//	}
//};
//
//class Airplain {
//public:
//	string str = "airplain line";
//	void Fly() {
//		cout << "i fly" << endl;
//	}
//};
//
//class FlyingCar :public Car, public Airplain {							//множественное наследование
//
//};
//
//int main() {
//	FlyingCar fc;
//	Car *ptrC = &fc;
//	Airplain *ptrA = &fc;
//}


//class Msg {																								//вызов виртуального метода базового класса
//public:
//	Msg(string msg) {
//		this->msg = msg;
//	}
//	virtual string GetMsg() {
//		return msg;
//	}
//private:
//	string msg;
//};
//
//class BraketsMsg :public Msg {
//public:
//	BraketsMsg(string msg) :Msg(msg) {
//
//	}
//	string GetMsg()override {
//		return "[" + Msg::GetMsg() + "]";			//метод базового класса Msg::GetMsg(), без мсг рекурсия
//	}
//};
//class Printer {
//public:
//	void Print(Msg *msg) {
//		cout << msg->GetMsg() << endl;
//	}
//};
//int main() {
//	BraketsMsg m("Hello");
//	Printer p;
//	p.Print(&m);
//}


//class Human {																							//делегирующие конструкторы
//public:
//	Human(string Name) {
//		this->Name = Name + "!";
//		this->Age = 0;
//		this->Weight = 0;
//	}
//	Human(string Name, int Age):Human(Name) {
//		this->Age = Age;
//	}
//	Human(string Name, int Age, int Weight):Human(Name,Age) {
//		this->Weight = Weight;
//	}
//private:
//	string Name;
//	int Age;
//	int Weight;
//};
//int main() {
//	Human h("Aristarch",30,100);
//	return 0;
//}


//class A {																								//чисто виртуальный деструктор
//public:																									//
//	A() {																								//
//		
//	}
//	virtual ~A() = 0;																					//чисто виртуальный деструктор
//};
//A::~A() {};																								//чисто виртуальный деструктор
//class B : public A {
//public:
//	B() {
//		
//	}
//	~B() override{
//		
//	}
//};
//int main() {
//	A *bptr = new B;
//	delete bptr;
//}


//class A {																								//виртуальный деструктор нужен чтобы правильно высвобождать ресурсы 
//public:																									//в классах наследников если используется динамическая память
//	A() {																								//(указатель базового класса для хранения ссылок на классы наследников)
//		cout << "Dynamic memory of object A" << endl;
//	}
//	virtual ~A() {
//		cout << "Delete dynamic memyry of object A" << endl;
//	}
//};
//
//class B : public A {
//public:
//	B() {
//		cout << "Dynamic memory of object B" << endl;
//	}
//	~B() override{
//		cout << "Delete dynamic memyry of object B" << endl;
//	}
//};
//
//int main() {
//	A *bptr = new B;
//	delete bptr;
//}


//class Weapon {																							//абстрактный класс и чисто виртуальные функции
//public:
//	virtual void Shoot() = 0;							//чисто виртуальная функции
//	void Foo() {
//		cout << "Foo()" << endl;
//	}
//};
//class Gun : public Weapon{																										
//public:
//	void Shoot() override {								//при наследовании обязательно реализовать метод абстрактного класса
//		cout << "BANG!" << endl;
//	}
//};
//class SubmachineGun : public Gun {
//public:
//	void Shoot() override {									//override - контроль, чтобы метод не отличался от наследоваемого
//		cout << "BANG! BANG! BANG!" << endl;
//	}
//};
//class Bazooka : public Weapon {
//public:
//	void Shoot() override {
//		cout << "BOOOOOOOOOOOOM!!11" << endl;
//	}
//};
//class Knife : public Weapon{
//public:
//	void Shoot() override {
//		cout << "STRIKE!" << endl;
//	}
//};
//class Player {
//public:
//	void Shoot(Weapon *weapon) {							//"даём игроку пушку"
//		weapon->Shoot();
//	}
//};
//int main() {
//	Bazooka rpg;
//	Gun pistol;
//	SubmachineGun machinegun;
//	Knife knife;
//	Gun *weapon = &machinegun;						//полиморфизм. вызовится метод класса SubmachineGun 3 шота(без виртуал выстрелит 1 раз)
//													
//	Player player;
//	player.Shoot(&knife);							//"даём игроку пушку"
//	knife.Foo();
//}


//class Gun {																										//виртуальные функции
//public:
//	virtual void Shoot() {
//		cout << "BANG!" << endl;
//	}
//};
//
//class SubmachineGun :public Gun{
//public:
//	void Shoot() override {									//override - контроль, чтобы метод не отличался от наследоваемого
//		cout << "BANG! BANG! BANG!" << endl;
//	}
//};
//int main() {
//	Gun pistol;
//	SubmachineGun machinegun;
//	Gun *weapon = &machinegun;						//полиморфизм. вызовится метод класса SubmachineGun 3 шота(без виртуал выстрелит 1 раз)
//													//указатель на базовый класс хранит имена своих наследников
//}


//class A {																										//конструктор при наследовании и передача параметров
//public:
//	A() {
//		msg = "NULL";						
//	}
//	A(string str) {
//		this->msg = str;
//	}
//	A(string str,int a) {
//		this->msg = str;
//		this->a = a;
//	}
//	void PrintMsg() {
//		cout << msg << endl;
//		cout << a << endl;
//	}
//private:
//	string msg;
//	int a = 0;
//};
//class B : public A {
//public:
//	B():A("NEW MESSAGE") {										//явное указание нужного конструктора
//																//B():A() - вызов конструктора по умолчанию
//	}															//
//	B(int b) :A("NEW MESSAGE",b) {								//параметр идёт к конструктору А
//																
//	}
//private:
//};
//int main() {													//
//	A a("ADASD SA");
//	a.PrintMsg();
//	B b(99);
//	b.PrintMsg();
//}


//class A {																										//конструктор и деструктор при наследовании
//public:														
//	A() {
//		cout << "constructor A" << endl;						//конструктор сначала этот
//	}
//	~A() {
//		cout << "\tdestructor A" << endl;						//последний деструктор -стек(первый вошёл последний вышел)
//	}
//};
//class B : public A {										
//public:
//	B() {
//		cout << "constructor B" << endl;						//конструктор потом этот
//	}
//	~B() {
//		cout << "\tdestructor B" << endl;
//	}
//};
//class C : public B {
//public:
//	C() {
//		cout << "constructor C" << endl;						//конструктор в конце этот
//	}
//	~C() {
//		cout << "\tdestructor C" << endl;						//первый
//	}
//};
//int main() {													//собираем A->B->C разбираем C->B->A
//	A aaa;
//	B bbb;
//	C ccc;
//}


//class A {
//public:														//доступно объекту и другому классу при наследовании
//	string msgOne = "message one";
//private:													//доступно только своему классу
//	string msgTwo = "message two";
//protected:													//доступно при наследовании другому классу, но не объекту
//	string msgThree = "message three";
//};
//
//class B : public A {										// private - все модификаторы при наследовании приват. protected - паблик протектед
//public:
//	void PrintMsg() {
//		cout << msgThree << endl;
//	}
//};
//
//int main() {
//	B b;
//	b.PrintMsg();
//}


//class Human {																									//наследование
//private:
//	string name;
//public:
//	string GetName() {
//		return name;
//	}
//	void SetName(string name) {
//		this->name = name;
//	}
//};
//
//class Student : public Human {						//наследование
//public:
//	string group;
//	void Learn() {
//		cout << "LEARN" << endl;
//	}
//};
//
//class ExtramuralStudent : public Student {				//множественное наследование (наследует класс хуман и студент)
//public:
//	void Learn() {
//		cout << "NO LEARN" << endl;
//	}
//};
//
//class Professor : public Human {
//public:
//	string subject;
//};
//
//int main() {
//	Student st;
//	st.SetName("ABERAT");
//	cout << st.GetName() << endl;
//	st.Learn();
//	ExtramuralStudent extrSt;
//	extrSt.SetName("EBANTII");
//	cout<<extrSt.GetName()<<endl;
//	extrSt.Learn();
//}


//class Cap {																							//агрегация и композиция
//public:
//	string GetColot() {
//		return color;
//	}
//private:
//	string color = "red";
//};
//
//class Model {								//агрегация
//public:
//	void InspectModel() {
//		cout<<"Cap is "<< cap.GetColot() << endl;
//	}
//private:
//	Cap cap;
//};
//
//class Human {
//public:
//	void Think() {
//		brain.Think();						//делегирование вместе с композицией
//	}
//
//	void InspectTheCap() {										//агрегация - добавлен метод из независимого класса
//		cout << "My cap is" << cap.GetColot() << endl;
//	}
//
//private:
//
//	class Brain {							//композиция - жесткая привязка одного класса к другому
//	public:
//		void Think() {
//			cout << "i think" << endl;
//		}
//	};
//
//	Brain brain;
//	Cap cap;
//};
//
//int main() {
//	Human human;
//	human.Think();
//	human.InspectTheCap();
//	Model model;
//	model.InspectModel();
//}


//class Image {																				//вложенный класс(inner)
//public:
//	void GetImageInfo() {
//		for (int i = 0; i < LENGTH; i++)
//		{
//			cout <<"#"<<i<<" "<< pixel[i].GetInfo() << endl;
//		}
//	}
//
//	class Pixel {													//вложенный класс
//	public:
//		Pixel(int r,int g, int b) {
//			this->r = r;
//			this->g = g;
//			this->b = b;
//		}
//		string GetInfo() {
//			return "Pixel: r = " + to_string(r) + " g = " + to_string(g) + " b = " + to_string(b);	//to_string - конвертация в строку
//		}
//	private:
//		int r, g, b;
//	};
//
//private:
//	static const int LENGTH = 5;
//
////Pixel *arr = new Pixel[LENGTH];								//динамический массив классов			
////delete[]arr;
//
//	Pixel pixel[5]{										//массив классов
//		Pixel(2,53,1),
//		Pixel(30,13,7),
//		Pixel(211,98,2),
//		Pixel(71,73,87),
//		Pixel(87,33,33)
//	};
//};
//
//int main() {
//	//Image img;
//	//img.GetImageInfo();
//	Image::Pixel pixel(22, 66, 19);
//	cout<<pixel.GetInfo();
//}


//class Apple {																			//static
//public:
//	Apple(int weight, string color) {
//		this->weight = weight;
//		this->color = color;
//		Count++;
//		id = Count;
//	}
//	void Print() {
//		cout << id << "\t" << color << "\t" << weight << endl;
//	}
//	int GetId() {
//		return id;
//	}
//	static int GetCount() {					//гетер
//		return Count;
//	}
//	static void ChangeColor(Apple & apple, string color) {			//статичяеский метод (
//		apple.color = color;
//	}
//	void ChangeColorNoStatic(string color) {			//обращение через объект (не статик)
//		this->color = color;
//	}
//	//1) В статик методах класса можно работать только с статик переменными этого класса, в не статик методах класса можно работать со статик переменными класса.
//	//2) Вы объявили private static переменную в классе и потом после его описания  инициализировали переменную.Я работаю в Qt Creator и хотелось бы добавить что 
//	//компилятор таким способом выдаёт ошибку что переменная приватная.Нужно было объявить отдельно класс Aplle с двумя файлами Apple.h и Apple.cpp, заголовочный 
//	//	файл и файл реализации.Так вот инициализация приватной статик переменной происходит в файле реализации.
//private:
//	int weight;
//	string color;
//	static int Count;				//static - общее значение для всех объектов класса
//	int id;
//};
//
//int Apple::Count = 0;				//инициализация static(в Qt не пашет? надо объявлять в хедере класс и инициализировать в срр?)
////Статический метод - функция, которую можно использовать, не создавая объект(переменную / константу и т.д.) и которая вообще не привязывается к объекту, через который её так же можно вызвать.
////Дружественная функция - функция, которая видит приватные / защищённые поля класса.
////Дружественный метод - функция, принадлежащая другому классу, которая видит поля первого класса.
//
//int main() {
//	Apple apple(200, "Green");
//	Apple apple2(100, "Red");
//	Apple::ChangeColor(apple, "Blue");			//обращение через класс (статик)
//	apple2.ChangeColorNoStatic("Yellow");		//обращение через объект
//	cout << apple.GetId() << endl;;			//обращение к статик
//	cout << apple2.GetId() << endl;;
//	cout << Apple::GetCount() << endl;
//}


//class Gopnik {																					//простейший ID
//public:
//	static int ID;
//	Gopnik(string name, string pivo, int litersInDay, int iq, string sidel) {
//		this->name = name;
//		this->pivo = pivo;
//		this->litersInDay = litersInDay;
//		this->iq = iq;
//		this->sidel = sidel;
//		ID++;
//		this->number = ID;
//	}
//	~Gopnik() {
//		--ID;
//	}
//	void Print() {
//		cout << number<<"\t\t - ID\n"; 
//		cout << name << "\t\t - name\n";
//		cout << pivo << "\t\t - pivo\n";
//		cout << litersInDay << "\t\t - liters in day\n";
//		cout << iq << "\t\t - IQ\n";
//		cout << sidel << "\t\t - sidel\n\n\n";
//	}
//private:
//	string name;
//	string pivo;
//	int litersInDay;
//	int iq;
//	string sidel;
//	int number;
//};
//
//int Gopnik::ID = 0;
//int main() {
//	int number;
//	Gopnik vasya("Vasya","baltika 9", 3, 87, "no");
//	Gopnik petya("Petya", "ohota strong", 2, 102, "3 years");
//	Gopnik pavel("Pavel", "bernard", 1, 119, "no");
//	vasya.Print();
//	petya.Print();
//	pavel.Print();
//}


//class String2 {																					//СВОЙ КЛАСС СТРИНГ
//public:
//	String2() {
//		str = nullptr;
//		length = 0;
//	}
//
//	String2(const char *str) {								//конструктор
//		length = strlen(str);
//		this->str = new char[length + 1];
//		for (int i = 0; i < length; i++)
//		{
//			this->str[i] = str[i];
//		}
//		this->str[length] = '\0';
//	}
//	~String2() {												//деструктор
//		delete[] this->str;
//	}
//
//	String2 (const String2 &other) {						//конструктор копирования
//		if (this->str != nullptr)
//			delete[] str;
//
//		length = strlen(other.str);
//		this->str = new char[length + 1];
//		for (int i = 0; i < length; i++)
//		{
//			this->str[i] = other.str[i];
//		}
//		this->str[length] = '\0';
//	}
//
//	String2(String2 &&other) {								//конструктор перемещения	&& - ссылка на ссылку
//		this->length = other.length;
//		this->str = other.str;
//		other.str = nullptr;			//"запретить удалять деструктору"
//	}
//
//	String2 & operator = (const String2 &other) {			//оператор присваивания
//		if (this->str != nullptr)
//			delete[] str;
//
//		length = strlen(other.str);
//		str = new char[length+1];
//		for (int i = 0; i < length; i++)
//		{
//			str[i] = other.str[i];
//		}
//		str[length] = '\0';
//		return *this;
//	}
//
//	String2 operator + (const String2 &other) {					//оператор сложения
//		String2 newStr;											//для динамической памяти нужен конструктор копирования
//		int thisLength = strlen(this->str);
//		int otherLength = strlen(other.str);
//		newStr.length = thisLength + otherLength;
//		//length = strlen(this->str) + strlen(other.str);
//		newStr.str = new char[length+1];
//
//		int i = 0;
//		for (; i < thisLength; i++)
//		{
//			newStr.str[i] = this->str[i];
//		}
//
//		for (int j = 0; j < otherLength; j++,i++)
//		{
//			newStr.str[i] = other.str[j];
//		}
//
//		newStr.str[thisLength+ otherLength] = '\0';
//		return newStr;			// return newStr.str (может без конструктора копирования)
//	}
//
//	bool operator ==(const String2 &other) {					//Оператор равно ==
//		if (this->length != other.length)
//			return false;
//		for (int i = 0; i < length; i++)
//		{
//			if (this->str[i] != other.str[i])
//				return false;
//		}
//		return true;
//	}
//	
//	char & operator[](int index) {				//без & ссылки можно только посмотреть, с ней изменить
//		return this->str[index];
//	}
//
//	bool operator !=(const String2 &other) {					//Оператор не равно !=
//		return !(this->operator==(other));
//	}
//		
//	int Length() {								//длина строки
//		return length;										// затратно	-	return strlen(this->str);
//	}
//
//	void Print() {
//		cout << str;
//	}
//private:
//	char *str;
//	int length;
//};
//
//int main() {
//	String2 str("TEST");
//	String2 str2("TEST!!");
//	String2 str3 = str;
//	String2 result = str + str2;
//	str[0] = 'V';
//	cout << result.Length() << endl;
//	cout << (str==str2) << endl;
//	cout << (str != str2) << endl;
//}


//class Human;																			// Дружественный метод класса
//class Apple;
//class Human {
//public:
//	void TakeApple(Apple &apple);
//};
//
//class Apple {
//public:
//	Apple(int weight111, string color111) {
//		weight = weight111;
//		this->color = color111;
//	}
//private:
//	int weight;
//	string color;
//
//	friend Human;												//дружественный класс
//	//friend void Human::TakeApple(Apple &apple);					// Дружественный метод класса	
//};
//
//int main() {
//	Apple apple(200, "Red");
//	Human human;
//	human.TakeApple(apple);
//}
//
//void Human::TakeApple(Apple & apple)
//{
//	cout << "Take apple " << "weight = " << apple.weight << "\tcolor = " << apple.color << endl;
//}


//class Test;			//"прототип" класса									
//class Point {																			//Перегрузка операторов + и - (сложения)
//	int x, y;
//public:
//	Point() {
//		x = 0; y = 0;
//		cout << this << " constructor" << endl;
//	}
//
//	Point(int valueX, int valueY) {
//		x = valueX; y = valueY;
//		cout << this << " constructor" << endl;
//	}
//
//	bool operator ==(const Point &other) {					//Оператор равно ==
//		return x == other.x && y == other.y;
//	}
//	bool operator !=(const Point &other) {					//!=
//		return x != other.x || y != other.y;
//	}
//
//	Point operator+(const Point& other) {					//оператор +(альтернатива)			
//		this->x += other.x;
//		this->y += other.y;
//		return *this;
//	}
//	Point operator -(const Point &other) {					//оператор -
//		Point temp;
//		temp.x = x - other.x;
//		temp.y = y - other.y;
//		return temp;
//	}
//	Point & operator ++() {									//префиксный инкремент
//		this->x++;
//		this->y++;
//		return *this;
//	}
//
//	Point operator ++(int value) {							//постфиксный инкремент (надо без &  ??)
//		Point temp(*this);
//		this->x++;
//		this->y++;
//		return temp;
//	}
//
//	int GetY() {
//		return y;
//	}
//	void SetY(int y) {
//		this->y = y;										//this - указатель
//	}
//
//	int GetX() {
//		return x;
//	}
//	void SetX(int valueX) {
//		x = valueX;
//	}
//	void Print() {
//		cout << "x = " << x << "\t y = " << y << endl;
//	}
//	friend void ChangeX(Point &value, Test &testValue);						//дружественная функция(можно указать в любом модификаторе паблик,приват,протект)
//};
//
//class Test {				//дружественная функция
//private:
//	int Data = 0;
//	friend void ChangeX(Point &value, Test &testValue);
//};
//
//void ChangeX(Point &value, Test &testValue) {								//дружественная функция
//	value.x = -11;
//}
//
//int main() {
//	Test test;
//	Point a(1, 4);
//	a.Print();
//	ChangeX(a, test);
//	a.Print();
//}



//class TestClass {																		//перегрузка оператора индексирования
//
//public:
//	int & operator[](int index) {				//с & сылкой можно менять значение
//		return arr[index];
//	}
//
//private:
//	int arr[5]{ 1,4,78,11,7 };
//};
//
//int main() {
//	TestClass a;
//	cout << a[2] << endl;
//	a[2] = 111;
//	cout << a[2];
//
//}


//class Point {																			//Перегрузка операторов + и - (сложения)
//	int x, y;
//public:
//	Point() {
//		x = 0; y = 0;
//		cout << this << " constructor" << endl;
//	}
//
//	Point(int valueX, int valueY) {
//		x = valueX; y = valueY;
//		cout << this << " constructor" << endl;
//	}
//
////Point operator+(const Point& other) {				//альтернатива
////	this->x += other.x;
////	this->y += other.y;
////	return *this;
////}
//	Point operator +(const Point &other) {
//		Point Sum;
//		Sum.x = x + other.x;
//		Sum.y = y + other.y;
//		return Sum;
//	}
//
//	Point operator -(const Point &other) {
//		Point temp;
//		temp.x = x - other.x;
//		temp.y = y - other.y;
//		return temp;
//	}
//Point & operator ++() {							//префиксный инкремент
//	this->x++;
//	this->y++;
//	return *this;
//}
//
//Point operator ++(int value) {				//постфиксный инкремент (надо без &  ??)
//	Point temp(*this);
//	this->x++;
//	this->y++;
//	return temp;
//}
//};
//
//int main() {
//	Point a(1, 4);
//	Point b(6, 1);
//	Point c = a + b;
//}


//int main() {
//
//	return 0;
//}


//class Point {																			//Перегрузка операторов равно и не равно
//	int x, y;
//public:
//	Point() {
//		x = 0; y = 0;
//		cout << this << " constructor" << endl;
//	}
//
//	Point(int valueX, int valueY) {
//		x = valueX; y = valueY;
//		cout << this << " constructor" << endl;
//	}
//
//	bool operator ==(const Point &other) {					//Оператор равно ==
//		return x == other.x && y == other.y;
//	}
//	bool operator !=(const Point &other) {					//!=
//		return x != other.x || y != other.y;
//	}
//};
//
//int main() {
//	Point a(1, 4);
//	Point b(4, 1);
//
//	bool result = a == b;
//	bool re2 = a != b;
//	cout << result << endl;
//	cout << re2 << endl;
//}


//class MyClass {																		//оператор присваивания перегрузка
//	int *data;		
//	int Size;
//public:
//	MyClass(int size) {
//		this->Size = size;
//		this->data = new int [size];
//		for (int i = 0; i < size; i++)
//		{
//			data[i] = i;
//		}
//		cout << "CONSTRUCTOR - " << this << endl;
//	}
//	MyClass(const MyClass &pother) {								//КОНСТРУКТОР КОПИРОВАНИЯ
//		cout << "CONSTRUCTOR COPY " << this << endl;
//		this->Size = pother.Size;
//		this->data = new int[pother.Size];
//		for (int i = 0; i < pother.Size; i++)
//		{
//			this->data[i] = pother.data[i];
//		}
//	}
//
//	MyClass & operator = (const MyClass &pother) {					//оператор присваивания
//		cout << "OPERATOR = " << this << endl;
//		this->Size = pother.Size;
//		if (this->data!=nullptr)
//		{
//			delete[] this->data;
//		}
//
//		this->data = new int[pother.Size];
//		for (int i = 0; i < pother.Size; i++)
//		{
//			this->data[i] = pother.data[i];
//		}
//		return *this;
//	}
//
//	~MyClass() {						
//		cout << "DESTRUCTOR - " << this << endl;
//		delete[] data;
//	}
//};
//
//int main()
//{
//	MyClass a(10);
//	MyClass b(2);
//	MyClass c(8);
//	c = a = b;
//
//}


//class MyClass {																		//КОНСТРУКТОР КОПИРОВАНИЯ
//	int *data;		
//	int Size;
//public:
//	MyClass(int size) {
//		this->Size = size;
//		this->data = new int [size];
//		for (int i = 0; i < size; i++)
//		{
//			data[i] = i;
//		}
//		cout << "CONSTRUCTOR - " << this << endl;
//	}
//	MyClass(const MyClass &pother) {								//КОНСТРУКТОР КОПИРОВАНИЯ
//		this->Size = pother.Size;
//		this->data = new int[pother.Size];
//		for (int i = 0; i < pother.Size; i++)
//		{
//			this->data[i] = pother.data[i];
//		}
//		cout << "CONSTRUCTOR COPY " << this << endl;
//	}
//
//	~MyClass() {						
//		cout << "DESTRUCTOR - " << this << endl;
//		delete data;
//	}
//};
//
//int main()
//{
//	MyClass a(10);
//	MyClass b(a);
//	//Foo2();
//	//Foo(a);
//}


//class Point {																				// ПЕРЕГРУЗКА КОНСТРУКТОРА
//private:
//	int x;
//	int y;
//public:
//	Point() {									
//		x = 0; y = 0;
//		cout << this << " constructor" << endl;			//адрес объекта
//
//	}
//	Point(int valueX, int valueY) {				// ПЕРЕГРУЗКА КОНСТРУКТОРА
//		x = valueX;
//		y = valueY;
//		cout << this << " constructor" << endl;
//	}
//	int GetY() {
//		return y;
//	}
//	void SetY(int y) {
//		this->y = y;				//this - указатель
//	}
//
//	int GetX() {
//		return x;
//	}
//	void SetX(int valueX) {
//		x = valueX;
//	}
//	void Print() {
//		cout << "x = " << x << "\t y = " << y << endl;
//	}
//};
//
//int main()
//{
//	Point a;
//	a.SetY(5);
//	a.Print();
//}


//class MyClass {																			 //деструктор
//	int* data;		//снаружи не получить, очищать в деструкторе
//public:
//	MyClass(int size) {
//		data = new int[size];		//динамич память
//		for (int i = 0; i < size; i++)
//		{
//			data[i] = i;
//		}
//		cout << "Object " << data << " CONSTRUCTOR" << endl;
//	}
//	~MyClass() {						
//		cout << "Object " << data << " DESTRUCTOR" << endl;
//		delete data;		//очистка памяти 
//	}
//};
//
//void Foo() {
//	cout << "Foo started" << endl;
//	MyClass a(1);
//	cout << "Foo end" << endl;
//}		//деструктор
//
//int main()
//{
//	Foo();
//	/*MyClass a(5);
//	MyClass b(11);
//	MyClass c(222);*/
//
//				// Деструктор как стек(первый вошёл-последний вышел)
//}				//Деструктор сработает здесь(конец области видимости)


//class Point {																				// ПЕРЕГРУЗКА КОНСТРУКТОРА
//private:
//	int x;
//	int y;
//public:
//	Point() {									
//		x = 0; y = 0;
//	}
//	Point(int valueX, int valueY) {				// ПЕРЕГРУЗКА КОНСТРУКТОРА
//		x = valueX;
//		y = valueY;
//	}
//	Point(int valueX, bool t) {					// ПЕРЕГРУЗКА КОНСТРУКТОРА
//		x = valueX;
//		if (t) {
//			y = 1;
//		}
//		else {
//			y = -1;
//		}
//	}
//	int GetY() {
//		return y;
//	}
//	void SetY(int valueY) {
//		y = valueY*3;
//	}
//
//	int GetX() {
//		return x;
//	}
//	void SetX(int valueX) {
//		x = valueX;
//	}
//	void Print() {
//		cout << "x = " << x << "\t y = " << y << endl;
//	}
//};
//
//int main()
//{
//	Point a;
//	a.Print();
//
//	Point b(5, 16);
//	b.Print();
//
//	Point c(22,false);
//	c.Print();
//}



//class CoffeGrinder {																		//ПРИМЕР ИНКАПСУЛЯЦИИ
//private:
//	bool CheckVoltage() {
//		return 1;
//	}
//public:
//	void Start() {
//		CheckVoltage() ? cout << "Start coffe" << endl: cout << "Beep Beep" << endl;
//	}
//};
//
//int main() {
//	CoffeGrinder a;
//	a.Start();
//}


//class Point {																				// ГЕТТЕРЫ И СЕТТЕРЫ
//private:
//	int x;
//	int y;
//public:
//Point(int valueX, int valueY) {										//КОНСТРУКТОР КЛАССА
//	x = valueX;
//	y = valueY;
//}
//	int GetY() {
//		return y;
//	}
//	void SetY(int valueY) {
//		y = valueY*3;
//	}
//
//	int GetX() {
//		return x;
//	}
//	void SetX(int valueX) {
//		x = valueX;
//	}
//	void Print() {
//		cout << "x = " << x << "\t y = " << y << endl;
//	}
//};
//
//int main()
//{
//	Point a(11,44);				//(11,44) конструктор
//	a.SetY(11);
//	a.SetX(9);
//	a.Print();
//}


//class Point {																				//МОДИФИКАТОРЫ ДОСТУПА КЛАССОВ
//public:
//	int x;
//	void Print() {
//		cout << "x - " << x << "\ny - " << y << "\nz - " << z << endl;
//		PrintY();
//	}
//
//private:					// НЕВИДНО СНАРУЖИ, ВИДНО ВНУТРИ
//	int y=sin(x);
//	int z=cos(x);
//	void PrintY() {
//		cout << y << endl;
//		cout << "PrintY" << endl;
//	}
//};
//
//int main()
//{
//	Point a;
//	a.x = 10;
//	a.Print();
//}



//class Human {																				// МЕТОДЫ КЛАССА
//public:
//	int age;
//	int weight;
//	string name;
//
//	void Print() {
//		cout << "Name - " << name << "\nWeight - " << weight << "\nAge - " << age << endl;
//	}
//};
//
//int main()
//{
//	Human firstHuman;
//	firstHuman.age = 30;
//	firstHuman.name = "Noob Dnichev";
//	firstHuman.weight = 77;
//	firstHuman.Print();
//}


//class Human {																				// КЛАСС
//public:
//	int age;
//	int weight;
//	string name;
//};
//
//int main()
//{
//	Human firstHuman;
//
//	firstHuman.age = 30;
//	firstHuman.name = "Noob Dnichev";
//	firstHuman.weight = 77;
//
//	cout << firstHuman.age << endl;
//	cout << firstHuman.name << endl;
//	cout << firstHuman.weight << endl;
//
//	cout << "\n###############################\n" << endl;
//	Human secondHuman;
//	secondHuman.age = 24;
//	secondHuman.name = "Detrov";
//	secondHuman.weight = 154;
//
//	cout << secondHuman.age << endl;
//	cout << secondHuman.name << endl;
//	cout << secondHuman.weight << endl;
//}

//int main()																				// ТЕРНАРНЫЙ ОПЕРАТОР
//{
//	setlocale(0, "ru");
//	int a;
//	cout << "enter the value of variable a: ";
//	cin >> a;
//	a < 10 ? cout << "a less 10" << endl : a>10 ? cout << "a greater 10" << endl : cout << "a equal to 10" << endl;
//	system("pause");
//}

//#define DEBUG 5																		// ДИРЕКТИВЫ #DEFINE #IFDEF #IF #ELIF #ELSE
//using namespace std;
//
//int main()
//{
//	setlocale(0, "ru");
//
//	// #ifndef - не определён(наоборот). #ifdef - определён
//
//#ifdef DEBUG
//	cout << "DEBUG DETECTED" << endl;
//#else
//	cout << "DEBUG NOT DETECTED" << endl;
//#endif // DEBUG
//
//
//#if DEBUG>4
//	cout << "DEBUG > 4" << endl;
//#elif DEBUG == 5
//	cout << "DEBUG = 5" << endl;
//#endif // DEBUG>4
//}


//#define FOO(x,y)((x)+(y))																// МАКРОС
//using namespace std;
//int main()
//{
//	cout << FOO(5+5, 6+4);
//}


//#define PI 3.14																		//ДИРЕКТИВА DEFINE (Заменяеет значения)
//#define tab "\t"
//#define begin {
//#define end }
//
//using namespace std;
//
//void main()
//{
//	for (int i = 0; i < 5; i++)
//	begin
//		cout << i << endl;
//	end
//}

//string GetDataFromDB()																//УКАЗАТЕЛЬ НА ФУНКЦИЮ
//{
//	return "Data From DB";
//}
//
//string GetDataFromWeb()
//{
//	return "Data From Web";
//}
//
//string GetDataFromAstral()
//{
//	return "DATA BLA";
//}
//
//void ShowInfo(string (*foo)())
//{
//	cout << foo() << endl;
//}
//
//int main()
//{
//	ShowInfo(GetDataFromAstral);
//}


//int main()																			//ОБЪЕДИНЕНИЕ СТРОК C++	КОНКАТЕНАЦИЯ
//{
//	string str1 = "Abdul";
//	string str2 = "Zebrat";
//	string str3 = "Lohovic";
//	string result;
//
//	result = "Name:\t\t"+ str1 + "\nPatronymic:\t" + str2 +"\nSurname:\t"+ str3;
//	cout << result;
//}


//int main()																			//ОБЪЕДИНЕНИЕ СТРОК C(си) КОНКАТЕНАЦИЯ
//{																						//(malloc / mcopy)
//	char result[255]{};
//	char str1[255] = "Hello";
//	char str2[255] = "world";
//
//	cout << str1 << endl;
//	strcat_s(result, str1);
//	strcat_s(result, str2);
//	cout << result << endl;
//}


//int Foo(const char* str)																// Указатели символьные строки и функции(расчёт длины строки)
//{
//	int i = 0;
//	while (str[i] != '\0')
//		i++;
//	return i;
//}
//
//int main()
//{
//	const char *str = "Hello!";
//	cout << Foo(str) << endl;
//}


//float a = 1.35499;																	// цифр после запятой
//printf("%.2f \n", a);  //  результат 1.35


//void FillArray(int* const arr, const int size)										//ДОБАВЛЕНИЕ И УДАЛЕНИЕ ОПРЕДЕЛЁННОГО ЭЛЕМЕНТА ИЗ МАССИВА
//{
//	for (int i = 0; i < size; i++)
//	{
//		arr[i] = rand() % 10;
//	}
//}					
//
//void ShowArray(const int* const arr, const int size)
//{
//	for (int i = 0; i < size; i++)
//	{
//		cout << arr[i] << "\t";
//	}
//	cout << endl;
//}
//
//void push_back(int *&arr, int &size, int index, const int value)		
//{
//	index--;
//	size++;
//	int *newArray = new int[size];
//	for (int i = 0; i < size; i++)
//	{
//		if (i < index)
//		{
//			newArray[i] = arr[i];
//		} else newArray[i+1] = arr[i];
//	}
//	newArray[index] = value;
//	delete arr;
//	arr = newArray;
//}
//
//void pop_back(int *&arr, int &size, int indexD)
//{
//	size--;
//	int *newArray = new int[size];
//	for (int i = 0; i < size; i++)
//	{
//		if (i < indexD)
//		{
//			newArray[i] = arr[i];
//		}
//		else newArray[i] = arr[i+1];
//	}
//	delete arr;
//	arr = newArray;
//}
//
//int main()
//{
//	int size = 5;
//	int *arr = new int[size];
//	int index, indexD, value;
//	cout << "Enter add value: ";
//	cin >> value;
//	cout << "Enter add array index: ";
//	cin >> index;
//	cout << "Enter delete array index: ";
//	cin >> indexD;
//
//	FillArray(arr, size);
//	ShowArray(arr, size);
//
//	push_back(arr, size, index, value);
//	ShowArray(arr, size);
//
//	pop_back(arr, size, indexD);
//	ShowArray(arr, size);
//
//	delete arr;
//}


//void FillArray(int* const arr, const int size)										//ДОБАВЛЕНИЕ И УДАЛЕНИЕ ЭЛЕМЕНТА ИЗ МАССИВА
//{
//	for (int i = 0; i < size; i++)
//	{
//		arr[i] = rand() % 10;
//	}
//}					
//
//void ShowArray(const int* const arr, const int size)
//{
//	for (int i = 0; i < size; i++)
//	{
//		cout << arr[i] << "\t";
//	}
//	cout << endl;
//}
//
//void push_back(int *&arr, int &size,const int value)				// добавление элемента в массив
//{
//	int *newArray = new int[size + 1];
//	for (int i = 0; i < size; i++)
//	{
//		newArray[i] = arr[i];
//	}
//	newArray[size++] = value;
//	delete arr;
//	arr = newArray;
//}
//
//void pop_back(int *&arr, int &size)								// удаление элемента
//{
//	size--;
//	int *newArray = new int[size];
//	for (int i = 0; i < size; i++)
//	{
//		newArray[i] = arr[i];
//	}
//	delete arr;
//	arr = newArray;
//}
//
//int main()
//{
//	int size = 5;
//	int *arr = new int[size];
//	FillArray(arr, size);
//	ShowArray(arr, size);
//
//	push_back(arr, size, 111);
//	ShowArray(arr, size);
//
//	pop_back(arr, size);
//	ShowArray(arr, size);
//
//	delete arr;
//}


//void FillArray(int* const arr, const int size)										// КОПИРОВАНИЕ МАССИВОВ
//{
//	for (int i = 0; i < size; i++)
//	{
//		arr[i] = rand() % 10;
//	}
//}
//
//void ShowArray(const int* const arr, const int size)
//{
//	for (int i = 0; i < size; i++)
//	{
//		cout << arr[i] << "\t";
//	}
//	cout << endl;
//}
//
//int main()
//{
//	int size = 10;
//	int *arr1 = new int[size];
//	int *arr2 = new int[size];
//
//	FillArray(arr1, size);
//	FillArray(arr2, size);
//
//	cout << "First array = \t";
//	ShowArray(arr1, size);
//	cout << "Secons array = \t";
//	ShowArray(arr2, size);
//
//	delete arr1;
//	arr1 = new int[size];
//	for (int i = 0; i < size; i++)
//	{
//		arr1[i] = arr2[i];
//	}
//
//	cout << "=====================\n";
//	cout << "First array = \t";
//	ShowArray(arr1, size);
//	cout << "Secons array = \t";
//	ShowArray(arr2, size);
//
//	delete arr1;
//	delete arr2;
//}


//void DayCalc(int &d, int &m)														// KOROVA КОРОВА
//{
//	if (m == 2 && d > 28)
//	{
//		d = 1;
//	}
//	if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 30)
//	{
//		d = 1;
//	}
//	if (d > 31)
//	{
//		d = 1;
//	}
//}
//
//void Foo(int &d, int &m, int &d2)
//{
//	if (m == 2 && d > 25)
//	{
//		cout << "Выходные дни: ";
//		for (int i = 0; i < (d - 25); i++)
//		{
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//	if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 28)
//	{
//		cout << "Выходные дни: ";
//		for (int i = 0; i < (d - 28); i++)
//		{
//			//DayCalc(d, m);
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//	else {
//		cout << "Выходные дни: ";
//		for (int i = 4; i > d2; i--)
//		{
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//}
//
//void Foo2(int &d, int &m, int &d2)
//{
//	if (m == 2 && d > 25)
//	{
//		cout << "Рабочие дни: ";
//		for (int i = 0; i < (d - 25); i++)
//		{
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//	if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 28)
//	{
//		cout << "Рабочие дни: ";
//		for (int i = 0; i < (d - 28); i++)
//		{
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//	else {
//		cout << "Рабочие дни: ";
//		for (int i = 8; i > d2; i--)
//		{
//			cout << d << " ";
//			d++;
//			DayCalc(d, m);
//		}
//	}
//}
//
//int main()
//{
//	setlocale(0, "ru");
//	int arr[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
//	int d,d2;
//	int m;
//	int exit;
//	//int y;
//	//bool x = false;
//	cout << "Введи день:  ";
//	cin >> d;
//	cout << "Введи месяц: ";
//	cin >> m;
//
//	if (m > 12 || d > 31)
//	{
//		cout << "АЛЁ, КАЛЕНДАРЬ ИЗУЧИ" << endl;
//		goto link;
//	}
//
//	if (m == 2 && d > 28)
//	{
//		cout << "АЛЁ, КАЛЕНДАРЬ ИЗУЧИ" << endl;
//		goto link;
//	}
//
//	if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 30)
//	{
//		cout << "АЛЁ, КАЛЕНДАРЬ ИЗУЧИ" << endl;
//		goto link;
//	}
//
//	d2 = d;
//	for (int i = 0; i < (m-1); i++)
//	{
//		d2 += arr[i];
//	}
//	d2 = (d2 - 2) % 8;
//	if (d2<4)
//	{
//		Foo(d, m, d2);
//	} else {							
//		Foo2(d, m, d2);
//	}
//
//	cout << "\n\n     Повторить?\n1 - да // 2 - выход: \n";
//	cin >> exit;
//	if (exit == 1)
//	{
//		goto link;
//	}
//}


//int main()																// Двумерный динамический массив
//{
//	int rows = 4;
//	int cols = 5;
//	cout << "Enter rows: ";
//	cin >> rows;
//	cout << "Enter cols: ";
//	cin >> cols;
//	int **arr = new int*[rows];
//	for (int i = 0; i < rows; i++)
//	{
//		arr[i] = new int[cols];
//	}
//	//
//	for (int i = 0; i < rows; i++)		// заполнение
//	{
//		for (int j = 0; j < cols; j++)
//		{
//			arr[i][j] = rand() % 20;
//		}
//	}
//	//
//	for (int i = 0; i < rows; i++)		// вывод
//	{
//		for (int j = 0; j < cols; j++)
//		{
//			cout << arr[i][j] << "\t";
//		}
//		cout << endl;
//	}
//	//////////////		ОЧИСТКА ПАМЯТИ
//	for (int i = 0; i < rows; i++)
//	{
//		delete arr[i];					//очистка вторых массивов
//	}
//
//	delete[] arr;						// очистка основного
//}


//int main()															// Динамический массив и его заполнение
//{
//	int size = 0;
//	cout << "Enter array size: ";
//	cin >> size;
//	int *arr = new int[size];
//
//	for (int i = 0; i < size; i++)
//	{
//		arr[i] = rand() % 10;
//	}
//
//	for (int i = 0; i < size; i++)
//	{
//		cout << arr[i] << "\t";
//		cout << arr + i << "\t" << endl;
//	}
//	delete [] arr;
//}


//void main()															// динамическая память и очистка
//{
//	int *pa = new int;			
//	*pa = 10;
//	cout << *pa << endl;
//	delete pa;					// СНАЧАЛА DELETE ПОТОМ NULLPTR !!!!
//	pa = nullptr;				// полная очистка памяти 
//	cout << pa << endl;
//}


//template <typename T1, typename T2>
//void foo(T1 &a, T2 &b)												//ДЗ - шаблонная замена местами значений переменных через ссылки
//{																		// swap(var1, var2); //готовая функция
//	T1 x = a;
//	a = b;
//	b = x;
//}
//
//void main()
//{
//	char a = 'a';
//	int b = 6;
//	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
//	foo(a, b);
//	cout << "foo: " << endl;
//	cout << "a = " << int (a) << endl;			//явная перемена типа данных
//	cout << "b = " << char (b) << endl;
//}


//void Foo(int &a, int &b, int &c)										//"возврат" нескольких значений из функции через ссылки
//{
//	a = 19;
//	b *= 2;
//	c -= 100;
//}
//
//void main()
//{
//	int a = 0, b = 4, c = 34;
//	cout << "a = " << a << endl;
//	cout << "a = " << b << endl;
//	cout << "a = " << c << endl;
//
//	cout << "Foo" << endl;
//	Foo(a, b, c);
//	cout << "a = " << a << endl;
//	cout << "a = " << b << endl;
//	cout << "a = " << c << endl;
//}


//void Foo(int a)														//Ссылка, указатель(примеры)
//{
//	a = 1;
//}
//
//void Foo2(int &a)
//{
//	a = 2;
//}
//
//void Foo3(int *a)
//{
//	*a = 3;
//}
//
//void main()
//{
//	int value = 5;
//	cout << "value = " << value << endl << endl;
//	cout << "Foo" << endl;
//	Foo(value);
//	cout << "value = " << value << endl << endl;
//
//	cout << "Foo2" << endl;
//	Foo2(value);
//	cout << "value = " << value << endl << endl;
//
//	cout << "Foo3" << endl;
//	Foo3(&value);
//	cout << "value = " << value << endl << endl;
//
//}

//void main()															//Ссылка
//{
//	int a = 5;
//	int*pa = &a;
//
//	int &aRef = a;	//ссылка
//	int *ppa = &aRef;
//
//	cout << "a\t" << pa << endl;
//	cout << "pa\t" << pa << endl;
//	cout << "*pa\t" << *pa << endl;
//	cout << "&aRef\t" << aRef << endl;
//	*ppa = 12;
//	cout << "a\t" << a << endl;
//	// int *pp=null;
//	// int &pRef;	//требуется инициализация
//}

//void ch(int *a, int *b)												//ДЗ из камента без 3 переменной
//{
//	*a = *a + *b;
//	*b = *a - *b;
//	*a = *a - *b;
//}


//void foo(int *pa, int *pb)											//ДЗ - замена местами значений переменных
//{																		// swap(var1, var2); //готовая функция
//	int x = *pa;
//	*pa = *pb;
//	*pb = x;
//}
//
//void main()
//{
//	int a = 5;
//	int b = 6;
//	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
//	foo(&a, &b);
//	cout << "foo: " << endl;
//	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
//}


//void Foo(int *pa, int *pb, int *pc)									// Передача указателя в функцию
//{
//	(*pa)++;
//	(*pb)+=44;
//	(*pc)--;
//}
//
//void main()
//{
//	int a = 0, b = 0, c = 1;
//	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
//	cout << "c = " << c << endl;
//
//	cout << "foo" << endl;
//	Foo(&a,&b,&c);
//
//	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
//	cout << "c = " << c << endl;
//}



//void main()															//арифметика указателей. указатель и массив
//{
//	const int SIZE = 5;
//	int arr[SIZE]{ 34,3,1,7.9 };	//a[i]= *(a+i) = *(i+a) = i[a] .. Имя массива является указателем на адрес его первого элемента.
//
//	for (int i = 0; i < SIZE; i++)
//	{
//		cout << *(arr+i) << endl;	//*(arr+i) = arr[i]
//	}
//	cout << "================" << endl;
//	int *pArr = arr;
//
//	cout << "arr\t" << arr << endl;
//	cout << "pArr\t" << pArr << endl;
//	cout << "================" << endl;
//
//	for (int i = 0; i < SIZE; i++)
//	{
//		cout << *(pArr+i) << endl;
//	}
//}



//void main()														// УКАЗАТЕЛЬ
//{
//	int a = 5;
//	cout << "a - " << a << endl;
//	int *pb = &a;		//& - амперсанд (в данном случае оператор взятия адреса)
//						//p - поинтер (так принято для указателей)
//	int *pb2 = &a;
//
//	cout << "pb  - " << pb << endl;
//	cout << "pb2 - " << pb2 << endl;
//
//	*pb2 = 2;
//
//	cout << "a - " << a << endl;
//}



//long double Foo(int a)											//ФАКТОРИАЛ РЕКУРСИЕЙ
//{
//	if (a < 0) return 0;
//	if (a <= 1) return 1;
//	return a*Foo(a-1);
//}
//
//void main()
//{
//	int a;
//	setlocale(LC_ALL, "rus");
//	cout << "\t\t\tРасчёт факториала" << endl;
//	cout << "Введите цисло: ";
//	cin >> a;
//	cout << Foo(a);
//}



//int Foo(int a)													//РЕКУРСИЯ
//{
//	if (a < 1) return 0;
//	a--;
//	cout << a << endl;
//	return Foo(a);
//}
//
//int main()
//{
//	Foo(5);
//}



//template <typename T1, typename T2>								//ШАБЛОН ФУНКЦИИ
//T1 Sum(T1 a, T2 b)
//{
//	cout << a << endl;
//	cout << b << endl;
//}
//
//int main()
//{
//
//}


//int Sum(int a, int b, int c)															//ПЕРЕГРУЗКА ФУНКЦЙИИ
//{
//	return (a + b) / c;
//}
//int Sum(int a, int b)
//{
//	return a + b;
//}
//int Sum(double a, double b)
//{
//	return a + b;
//}
//int main()
//{
//	cout << Sum(1.2, 6.8);
//}


//void foo(int q, int a = 5, double b = 2.8)						//ПАРАМЕТРЫ ПО УМОЛЧАНИЮ
//{
//	for (int i = 0; i < a; i++)
//	{
//		cout << "#" << endl;
//	}
//}
//
//int main()
//{
//	foo(4);
//}


//int main()
//{
//	setlocale(LC_ALL, "rus");
//	int num = rand() % 100;
//	int nick = -1;
//	int trycount = 0;
//
//	for (int i = 0; i < 8; i++)	
//	{
//		cout << "Угадайте число: ";
//		cin >> nick;
//
//		if(nick < num)
//			cout << "меньше" << endl;
//
//		if(nick > num)
//			cout << "больше" << endl;
//		if (nick == num)
//			break;
//		continue;
//	}
//
//	if (nick == num)
//		cout << "Вы угадали число";
//	else
//		cout << "Извините, вы не смогли угадать число: " << num;
//	return 0;
//}


//int a;
//
//void foo(int a)
//{
//	a++;
//	cout << a << endl;
//}
//
//int main()
//{
//	foo(2);
//	a++;
//	cout << a << endl;
//}



//void foo(int,int);								//ПРОТОТИП ФУНКЦИИ
//void foo(int a, int b)
//{
//	cout << "IM FUNCTION!!" << endl;
//}
//int main()
//{
//	foo(1,6);
//}


//void FillArray(int arr[], const int size)			//ФУНКЦИЯ МАССИВ
//{
//	for (int i = 0; i < size; i++)
//	{
//		arr[i] = rand() % 10;
//	}
//}
//void PrintArray(int arr[], const int size)
//{
//	for (int i = 0; i < size; i++)
//	{
//		cout << arr[i] << endl;
//	}
//}
//int main()
//{
//	const int SIZE = 10;
//	int arr[SIZE];
//	FillArray(arr, SIZE);
//	PrintArray(arr, SIZE);
//}


//int Foo(int a)			// ФУНКЦИЯ
//{
//	return ++a;
//}
//int main()
//{
//	int b = 10;
//	b = Foo(b);
//	cout << b << endl;
//}


//void foo()				// ФУНКЦИЯ
//{
//	cout << "Im FUNCTION!!11" << endl;
//}
//
//int Sum(int a, int b)
//{
//	return a + b;
//}
//
//int main()
//{
//	int q = 11;
//	int w = 12;
//	cout << Sum(q, w) << endl;
//}


//int main()				//ЗАПОЛНЕНИЕ ДВУМЕРНОГО МАССИВА И ВЫВОД
//{
//	const int ROWS = 5;
//	const int COLS = 8;
//	int arr[ROWS][COLS];
//	for (int i = 0; i < ROWS; i++)
//	{
//		for (int j = 0; j < COLS; j++)
//		{
//			arr[i][j] = rand() % 10;
//			cout << arr[i][j]<<"\t";
//		}
//		cout << endl;
//	}
//	//for (int i = 0; i < ROWS; i++)			//ВЫВОД ОТДЕЛЬНО
//	//{
//	//	for (int j = 0; j < COLS; j++)
//	//	{
//	//		cout << arr[i][j] << endl;
//	//	}
//	//	cout << endl;
//	//}
//}


//int main()				//ГЕНЕРАТОР РАНДОМА БЕЗ ПОВТОРЕНИЯ С ВЫВОДОМ НАИМЕНЬШЕГО ЧИСЛА - МОЙ
//{
//	srand(time(NULL));		//#include <ctime>
//	int const SIZE = 10;
//	int arr[SIZE];
//	int a = 0;
//	int b, c = 9999;
//	link:		//GOTO
//	for (; a < SIZE; a++)
//	{
//		arr[a] = rand() % 100 + 1;
//		for (int i = 0; i < a; i++)
//		{
//			if(arr[i] == arr[a])
//			{
//				goto link;
//			}
//		}
//		cout << arr[a] << endl;
//	}
//	for (int i = 0; i < SIZE; i++)
//	{
//		b = arr[i];
//		if (b < c)
//		{
//			c = b;
//		}
//	}
//	cout << "Smaller number - " << c << endl;
//}



//int main()					//ГЕНЕРАТОР РАНДОМА БЕЗ ПОВТОРЕНИЯ + С ВЫВОДОМ НАИМЕНЬШЕГО ЧИСЛА - УРОК
//{
//	srand(time(NULL));
//	const int SIZE = 10;
//	int arr[SIZE];
//	bool alreadyThere;
//	for (int i = 0; i < SIZE; )
//	{
//		alreadyThere = false;
//		int newRandomValue = rand() % 20 + 1;
//
//		for (int j = 0; j < i; j++)
//		{
//			if (arr[j] == newRandomValue)
//			{
//				alreadyThere = true;
//				break;
//			}
//		}
//		if (!alreadyThere)
//		{
//			arr[i] = newRandomValue;
//			i++;
//		}
//	}
//	for (int i = 0; i < SIZE; i++)
//	{
//		cout << arr[i] << endl;
//	}
//	// ВЫВОД НАИМЕНЬШЕГО ЧИСЛА
//	int minValue = arr[0];
//	for (int i = 1; i < SIZE; i++)
//	{
//		if (arr[i] < minValue)
//		{
//			minValue = arr[i];
//		}
//	}
//	cout<< "Min value - " << minValue << endl;
//}


//int main() {				//ГЕНЕРАТОР РАНДОМА БЕЗ ПОВТОРЕНИЯ - КАМЕНТ
//	setlocale(LC_ALL, "ru");
//	srand(time(NULL));
//
//	int arr[10];
//
//	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
//		arr[i] = rand() % 10;
//
//		for (int j = 0; j < i; j++) {
//			if (arr[i] == arr[j]) {
//				i--;
//				break;
//			}
//		}
//	}
//
//	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
//		cout << arr[i] << endl;
//	}
//}


//int main()				//ГЕНЕРАТОР РАНДОМА БЕЗ ПОВТОРЕНИЯ - МОЙ
//{
//	srand(time(NULL));		//#include <ctime>
//	int const SIZE = 10;
//	int arr[SIZE];
//	int a = 0;
//	link:		//GOTO
//	for (; a < SIZE; a++)
//	{
//		arr[a] = rand() % 100 + 1;
//		for (int i = 0; i < a; i++)
//		{
//			if(arr[i] == arr[a])
//			{
//				goto link;
//			}
//		}
//		cout << arr[a] << endl;
//	}
//}





//int main()				// SIZE МАССИВА
//{
//
//	double arr[]{1,6,1,5,7,0,4,1};
//
//	cout << size(arr) << endl;
//
//	for (int i = 0; i < size(arr); i++)
//	{
//		cout<<arr[i]<<endl;
//	}
//
//}


//int main()				// МАССИВЫ
//{
//	int arr[4] = { 1,3,77,1111 };
//	cout << arr[2] << endl;
//
//}

//void main() {				// МАССИВЫ
//	int number_list;
//	const int size = 5;
//	string arr[size] = { "After Effects", "Cinema 4d", "Nuke", "Houdini", "Realflow" };
//	cout << "Choose your best program:" << endl;
//	for (int i = 0; i < size; i++) {
//		cout << i << ":Your best program is " << arr[i] << endl;
//	}...




//int main()			// ВЛОЖЕННЫЙ ЦИКЛ
//{
//	for (int i = 1; i < 5; i++)
//	{
//		cout << "First cycle " << i << endl;
//		for (int j = 0; j < 5; j++)
//		{
//			cout << "\tSecond cycle " << j << endl;
//		}
//	}
//
//}




//int main()			// НАРИСУЙ СИМВОЛ МОЙ
//{
//	while (true)
//	{
//		int a, b;
//		char c;
//		cout << "enter height:" << endl;
//		cin >> a;
//		cout << "enter length:" << endl;
//		cin >> b;
//		cout << "enter symbol:" << endl;
//		cin >> c;
//		for (int i = 0; i < a; i++)
//		{
//			for (int i = 0; i < b; i++)
//			{
//				cout << c;
//			}
//			cout << endl;
//		}
//	}
//}



//void main()			// НАРИСУЙ СИМВОЛ
//{
//	setlocale(LC_ALL, "ru");
//	int a, b;
//	cout << "Введите высоту:\n";
//	cin >> a;
//	cout << "Введите ширену:\n";
//	cin >> b;
//	for (int i = 0; i < a; i++)
//	{
//
//		for (int j = 0; j < b; j++)
//		{
//			cout << "*";
//		}
//		cout << endl;
//	}
//
//	system("pause");
//}









//int main()		// GOTO 
//{
//	cout << "One" << endl;
//
//	goto link;
//
//	cout << "Two" << endl;
//
//	cout << "Three" << endl;
//
//	link:
//
//	cout << "Four" << endl;
//
//	cout << "Five" << endl;
//}




//int main()				// continue
//{
//	for (int i = 0; i < 5; i++)
//	{
//		if (i==2)
//		{
//			continue;
//		}
//		cout << "variable i = " << i << endl;
//	}
//
//	int a;
//	cout << "Enter any key to exit..." << endl;
//	cin >> a;
//	system("pause");
//}





//int main()			// BREAK
//{
	//cout << "Start" << endl;
	//for (int i = 0; i < 10 ; i++)
	//{
	//	cout << "variable i = " << i << endl;

	//	if (i == 5)
	//	{
	//		break;
	//	}
	//}



//	int i = 0;			// BREAK
//	while (true)
//	{
//
//		cout << "variable i = " << i << endl;
//		if (i == 6)
//		{
//			break;
//		}
//		i++;
//	}
//	
//	cout << "End" << endl;
//}




/*int main()			// СУММА НЕЧЕТНЫХ ИЗ УРОКА
{
	int e = 0;
	int a, b;

	cout << "Enter number range" << endl;
	cin >> a >> b;

	do
	{
		if (a % 2 != 0)
		{
			e += a;
		}

		a++;
	} while (a <= b);
	cout << "sum of odd numbers : " << e;
}



/*int main()				// СУММА НЕЧЁТНЫХ ЧИСЕЛ МОЙ
{
	int a,b,d,e;
	int index;

	cout << "Enter number range: " << endl;
	cin >> a >> b;

	if (a % 2 == false)
	{
		a++;
	}
	if (b % 2 == false)
	{
		b--;
	}
	index = (b - a) / 2;
	e = a;
	do {
		d = e;
		if (a != b)
		{
			a += 2;
			e = a + d;
			index--;
		}
	}   while (index>0);
	cout << "sum of odd numbers : " << e;
}



/*int main()
{
	int sCount;
	char s;
	int line;
	int index = 0;

	cout << "enter symbol: " << endl;
	cin >> s;

	cout << "enter symbol count: " << endl;
	cin >> sCount;

	cout << "Enter line: " << endl
		<< "1 - vertical" << endl
		<< "2 - horizontal" << endl;
	cin >> line;

	if (line < 1 || line > 2 || sCount <= 0)
	{
		cout << "DOLBOEB" << endl;
		return 1;
	}

	switch (line)
	{
	case 1: while (index < sCount)
	{
		cout << s;
		index++;
	} break;

	case 2: while (index < sCount)
	{
		cout << s << endl;
		index++;
	} break;
	}
}


// КОРОВА!!!!!!!!
/*#include <iostream>

using namespace std;

int main()
{
	setlocale(0, " ");
	int d;
	int m;
	int y;
	bool x = false;
	cout << "Введи день\n";
	cin >> d;
	cout << "Введи месяц\n";
	cin >> m;
	cout << "Введи год\n";
	cin >> y;
	if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
		x = true;
	}
	if (m == 1 || m == 3 || m == 5 || m == 7 || m == 8 || m == 10) {
		if (d > 31) {
			d = d % 31;
			m++;
		}
	} else if (m == 2) {
		if (x) {
			if (d > 29) {
				d = d % 29;
				m++;
			}
		} else {
			if (d > 28) {
				d = d % 28;
				m++;
			}
		}
	} else if (m == 12) {
		if (d > 31) {
			d = d % 31;
			m = 1;
			y++;
		}
	}	else {
		if (d > 30) {
			d = d % 30;
			m++;
		}
	}
	cout << d << " " << m << " " << y;
}



/*#include <iostream>
#include <windows.h>

using namespace std;


int main()
{
	int a=0;
	while (a<10)
	{
		Sleep(5000);
		cout << "a = " << a << endl;
		a++;
	}

}

	/*	int a, b, math;
	cout << "Enter two numbers: \n";
	cin >> a >> b;

	cout << "Select math operation: \n" <<
		"1. +" << endl <<
		"2. -" << endl <<
		"3. *" << endl <<
		"4. /" << endl;
	cin >> math;
	switch (math)
	{
	case 1: cout << "Result +: " << a + b;
		break;
	case 2: cout << "Result -: " << a - b;
		break;
	case 3: cout << "Result *: " << a * b;
		break;
	case 4: if (b)
	{
		cout << "Result /: " << (float)a / b;
	}
			else
	{
		cout << "Error!";
	}
		break;

	default: cout << "Error!";
		break;
	}

}

	/*setlocale(LC_ALL, "rus");
	int num;
	cin >> num;
	printf("Число %i %s\n", num, ((num % 2) > 0) ? "не чётное" : "чётное");
}
	/*setlocale(LC_ALL, "rus");
	int a;
	while (true)
	{
		cout << "enter number :";
		cin >> a;
		if (a % 2 == false)
		{
			cout << "4islo " << a << " 4etnoe\n";
		}
		else
		{
			cout << "4islo " << a << " ne 4etnoe\n";
		}
	}

}


/*	int a;
	cout << "Enter number: ";
	cin >> a;
	if (a > 5)
	{
		cout << "Number > 5" << endl;
	}
	else if(a==5)
	{
		cout << "Nember = 5" << endl;
	}
	else
	{
		cout << "Nember < 5" << endl;
	}
}

/*void main()
{
	double a, b, c;
	char P;

	cout << "Enter Number one: " << endl;
	cin >> a;
	cout << "Enter Number two: " << endl;
	cin >> b;
	cout << "Enter Number three: " << endl;
	cin >> c;
		cout << "Press 1 for +" << "\nPress 2 for -" << "\nPress 3 for *" << "\nPress 4 for /" << "\nPress 5 for % a b" << endl;
		cin >> P;

		switch (P)
		{
		case '1': printf("Сумма чисел=%g", a + b + c); break;
		case '2': printf("=%g", a - b - c); break;
		case '3': printf("=%g", a*b*c); break;
		case '4': if (b) printf("=%g", a / b / c); break;
		case '5': printf("=%g", fmod(a, b)); break;
		default: printf("Input error!!!");
		}
	}


/*void main()
{
	setlocale(LC_ALL, "RUS");
	int Var, Var2;
	cout << "Enter two numbers: ";
	cin >> Var >> Var2;
	cout << "Number one = " << Var << endl << "Number two = " << Var2 << endl;
}

/*void main()
{
	setlocale(LC_ALL, "RUS");

	const int COUNT_DAYS_IN_WEEK = 7;

	const char TAB = '\t';

	cout << COUNT_DAYS_IN_WEEK << NEW_LINE;
}

/*int main()
{
	setlocale(LC_ALL, "RUS");
	double a = 1.2221;
	int b = 22;
	bool c = false;
	char d = 'F';   // can escape
	cout << a << "\n" << b << "\n" << c << "\n" << d << endl;
}

/*
ESCAPE:
\n новая строка
\t табуляция
\" ковычки

*/
