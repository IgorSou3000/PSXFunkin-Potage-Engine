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

Events event_speed;

static void Events_Check(Event* event)
{
	//Events
	switch(event->event & EVENTS_FLAG_VARIANT)
	{
		case EVENTS_FLAG_SPEED: //Scroll Speed!!
		{
			event_speed.value1 = event->value1;
			event_speed.value2 = event->value2;
			break;
		}
		case EVENTS_FLAG_GF: //Set GF Speed!!
		{
			stage.gf_speed = (event->value1 >> FIXED_SHIFT) * 4;
			break;
		}
		case EVENTS_FLAG_CAMZOOM: //Add Camera Zoom!!
		{
			if (stage.save.canbump == true)
			{
				stage.charbump += event->value1;
				stage.bump += event->value2;
			}
			break;
		}
		default: //nothing lol
		break;
	}
}

void Events_Tick(void)
{
	//Scroll Speed!
	stage.speed += (FIXED_MUL(stage.ogspeed, event_speed.value1) - stage.speed) / (((event_speed.value2 / 60) + 1));
}

void Events_StartEvents(void)
{
	for (Event *event = stage.cur_event; event->pos != 0xFFFF; event++)
	{
		//Update event pointer
		if (event->pos > (stage.note_scroll >> FIXED_SHIFT))
			break;

		else
			stage.cur_event++;

		if (event->event & EVENTS_FLAG_PLAYED)
			continue;

		Events_Check(event);
		event->event |= EVENTS_FLAG_PLAYED;
	}

	//Same thing but for event.json
	if (stage.stage_def->exist_event_json == true)
	{
		for (Event *event = stage.event_cur_event; event->pos != 0xFFFF; event++)
		{
			//Update event pointer
			if (event->pos > (stage.note_scroll >> FIXED_SHIFT))
				break;

			else
				stage.event_cur_event++;

			if (event->event & EVENTS_FLAG_PLAYED)
				continue;

			Events_Check(event);
			event->event |= EVENTS_FLAG_PLAYED;
		}
	}

	Events_Tick();
}

//Initialize some stuffs
void Events_Load(void)
{
	//Scroll Speed
	event_speed.value1 = FIXED_UNIT;
	event_speed.value2 = 0;
}
