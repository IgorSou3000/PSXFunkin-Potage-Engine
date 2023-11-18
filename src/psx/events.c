/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "events.h"
#include "stage.h"
#include "psx/timer.h"
#include "psx/random.h"
#include "psx/mutil.h"

static Events event_speed;

#define GF_SPEED_FACTOR 4
#define EVENTS_TICK_DIVISOR 60
#define EVENTS_POS_END 0xFFFF

static void Events_GetValues(Event* event)
{
	//Update variables based on found event flags
	switch(event->event & EVENTS_FLAG_VARIANT)
	{
		case EVENTS_FLAG_SPEED:
		{
			event_speed.value1 = event->value1;
			event_speed.value2 = event->value2;
			break;
		}
		case EVENTS_FLAG_GF:
		{
			stage.gf_speed = (event->value1 >> FIXED_SHIFT) * GF_SPEED_FACTOR;
			break;
		}
		case EVENTS_FLAG_CAMZOOM:
		{
			if (stage.save.canbump == true)
			{
				stage.character_bump += event->value1;
				stage.bump += event->value2;
			}
			break;
		}
		default: //No updates to perform
		break;
	}
}

static void Events_CheckValues(Chart* chart)
{
	for (Event *event = chart->cur_event; event->pos != EVENTS_POS_END; event++)
	{
		//Update event pointer
		if (event->pos > (stage.note_scroll >> FIXED_SHIFT))
			break;

		else
			chart->cur_event++;

		if (event->event & EVENTS_FLAG_PLAYED)
			continue;

		Events_GetValues(event);

		event->event |= EVENTS_FLAG_PLAYED;
	}
}

void Events_Tick(void)
{
	//Scroll Speed!
	stage.speed += (FIXED_MUL(stage.previous_speed, event_speed.value1) - stage.speed) / (((event_speed.value2 / 60) + 1));

	Events_CheckValues(&stage.chart);

	if (stage.event_chart.data != NULL)
		Events_CheckValues(&stage.event_chart);
}

//Initialize some stuffs
void Events_Load(void)
{
	//Scroll Speed
	event_speed.value1 = FIXED_UNIT;
	event_speed.value2 = 0;
}
