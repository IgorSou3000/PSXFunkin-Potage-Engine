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
					//So easy LOL
					stage.gf_speed = (event->value1 / FIXED_UNIT) * 4;
					break;
				}

				default: //nothing lol
					break;
			}

				event->event |= EVENTS_FLAG_PLAYED;
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
