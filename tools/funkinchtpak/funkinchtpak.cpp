/*
 * funkinchtpak by Regan "CuckyDev" Green
 * Packs Friday Night Funkin' json formatted charts into a binary file for the PSX port

 *Psych Engine Event Reader By IgorSou3000
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <unordered_set>

#include "json.hpp"
using json = nlohmann::json;

#define SECTION_FLAG_OPPFOCUS (1 << 15) //Focus on opponent
#define SECTION_FLAG_BPM_MASK 0x7FFF //1/24

struct Section
{
	uint16_t end;
	uint16_t flag = 0;
};

#define NOTE_FLAG_OPPONENT    (1 << 2) //Note is opponent's
#define NOTE_FLAG_SUSTAIN     (1 << 3) //Note is a sustain note
#define NOTE_FLAG_SUSTAIN_END (1 << 4) //Is either end of sustain
#define NOTE_FLAG_ALT_ANIM    (1 << 5) //Note plays alt animation
#define NOTE_FLAG_MINE        (1 << 6) //Note is a mine
#define NOTE_FLAG_HIT         (1 << 7) //Note has been hit

struct Note
{
	uint16_t pos; //1/12 steps
	uint16_t type;
};

//EVENTS
#define EVENTS_FLAG_VARIANT 0xFFFC

#define EVENTS_FLAG_SPEED     (1 << 2) //Change Scroll Speed
#define EVENTS_FLAG_GF        (1 << 3) //Set GF Speed
#define EVENTS_FLAG_CAMZOOM   (1 << 4) //Add Camera Zoom

#define EVENTS_FLAG_PLAYED     (1 << 15) //Event has been already played

struct Event
{
	//psych engine events
	uint16_t pos; //1/12 steps
	uint16_t event;
	uint16_t value1;
	uint16_t value2;
};

typedef int32_t fixed_t;

#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)

uint16_t PosRound(double pos, double crochet)
{
	return (uint16_t)std::floor(pos / crochet + 0.5);
}

void WriteWord(std::ostream &out, uint16_t word)
{
	out.put(word >> 0);
	out.put(word >> 8);
}

void WriteLong(std::ostream &out, uint32_t word)
{
	out.put(word >> 0);
	out.put(word >> 8);
	out.put(word >> 16);
	out.put(word >> 24);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "usage: funkinchtpak in_json" << std::endl;
		return 0;
	}
	
	//Read json
	std::ifstream i(argv[1]);
	if (!i.is_open())
	{
		std::cout << "Failed to open " << argv[1] << std::endl;
		return 1;
	}
	json j;
	i >> j;
	
	auto song_info = j["song"];
	
	double bpm = song_info["bpm"];
	double crochet = (60.0 / bpm) * 1000.0;
	double step_crochet = crochet / 4;
	
	double speed = song_info["speed"];
	
	std::cout << argv[1] << " speed: " << speed << " ini bpm: " << bpm << " step_crochet: " << step_crochet << std::endl;
	
	double milli_base = 0;
	uint16_t step_base = 0;
	
	std::vector<Section> sections;
	std::vector<Note> notes;
	std::vector<Event> events;
	
	uint16_t section_end = 0;
	int score = 0, dups = 0;
	std::unordered_set<uint32_t> note_fudge;
	for (auto &i : song_info["notes"]) //Iterate through sections
	{
		bool is_opponent = i["mustHitSection"] != true; //Note: swapped
		
		//Read section
		Section new_section;
		if (i["changeBPM"] == true)
		{
			//Update BPM (THIS IS HELL!)
			milli_base += step_crochet * (section_end - step_base);
			step_base = section_end;
			
			bpm = i["bpm"];
			crochet = (60.0 / bpm) * 1000.0;
			step_crochet = crochet / 4;
			
			std::cout << "chg bpm: " << bpm << " step_crochet: " << step_crochet << " milli_base: " << milli_base << " step_base: " << step_base << std::endl;
		}
		new_section.end = (section_end += 16) * 12; //(uint16_t)i["lengthInSteps"]) * 12; //I had to do this for compatibility
		new_section.flag = PosRound(bpm, 1.0 / 24.0) & SECTION_FLAG_BPM_MASK; 
		bool is_alt = i["altAnim"] == true;
		if (is_opponent)
			new_section.flag |= SECTION_FLAG_OPPFOCUS;
		sections.push_back(new_section);
		
		//Read notes
		for (auto &j : i["sectionNotes"])
		{
			//Push main note
			Note new_note;

			//invalid type
			if (j[1] < 0)
				continue;
			
			int sustain = (int)PosRound(j[2], step_crochet) - 1;
			new_note.pos = (step_base * 12) + PosRound(((double)j[0] - milli_base) * 12.0, step_crochet);
			new_note.type = (uint8_t)j[1] & (3 | NOTE_FLAG_OPPONENT);
			if (is_opponent)
				new_note.type ^= NOTE_FLAG_OPPONENT;
			if (j[3] == true || j[3] == "Alt Animation")
				new_note.type |= NOTE_FLAG_ALT_ANIM;
			else if ((new_note.type & NOTE_FLAG_OPPONENT) && is_alt)
				new_note.type |= NOTE_FLAG_ALT_ANIM;
			if (sustain >= 0)
				new_note.type |= NOTE_FLAG_SUSTAIN_END;
			if (((uint8_t)j[1]) & 8 || j[3] == "Hurt Note")
				new_note.type |= NOTE_FLAG_MINE;
			
			if (note_fudge.count(*((uint32_t*)&new_note)))
			{
				dups += 1;
				continue;
			}
			note_fudge.insert(*((uint32_t*)&new_note));
				
			notes.push_back(new_note);
			if (!(new_note.type & NOTE_FLAG_OPPONENT))
				score += 350;
			
			//Push sustain notes
			for (int k = 0; k <= sustain; k++)
			{
				Note sustain_note;
				sustain_note.pos = new_note.pos + ((k + 1) * 12);
				sustain_note.type = new_note.type | NOTE_FLAG_SUSTAIN;
				if (k != sustain)
					sustain_note.type &= ~NOTE_FLAG_SUSTAIN_END; //sustain didn't end yet
				notes.push_back(sustain_note);
			}
		}
	}
	std::cout << "max score: " << score << " dups excluded: " << dups << std::endl;
	
	//Sort notes
	std::sort(notes.begin(), notes.end(), [](Note a, Note b) {
		if (a.pos == b.pos)
			return (b.type & NOTE_FLAG_SUSTAIN) && !(a.type & NOTE_FLAG_SUSTAIN);
		else
			return a.pos < b.pos;
	});

	//Read Events lol
	for (auto &i : song_info["events"]) //Iterate through sections
	{
		for (auto &j : i[1])
		{
			Event new_event;

			//Start with 0 for avoid bugs
			new_event.event = 0;

			new_event.pos = (step_base * 12) + PosRound(((double)i[0] - milli_base) * 12.0, step_crochet);

			if (j[0] == "Change Scroll Speed")
				new_event.event |= EVENTS_FLAG_SPEED;

			if (j[0] == "Set GF Speed")
				new_event.event |= EVENTS_FLAG_GF;

			if (j[0] == "Add Camera Zoom")
				new_event.event |= EVENTS_FLAG_CAMZOOM;

			if (new_event.event & EVENTS_FLAG_VARIANT)
			{
				if (new_event.event & EVENTS_FLAG_SPEED)
				{
					//Default values
					if (j[1] == "")
						j[1] = "1";

					if (j[2] == "")
						j[2] = "0";
				}

				if (new_event.event & EVENTS_FLAG_GF)
				{
					//Default values
					if (j[1] == "")
						j[1] = "1";

					if (j[2] == "")
						j[2] = "0";
				}
				if (new_event.event & EVENTS_FLAG_CAMZOOM)
				{
					//Default values
					if (j[1] == "")
						j[1] = "0.015"; //cam zoom

					if (j[2] == "")
						j[2] = "0.03"; //hud zoom
				}

				//Get values information
				std::string value1 =  j[1];
				std::string value2 =  j[2];

				//fixed values by 1024
				new_event.value1 = std::stof(value1) * FIXED_UNIT;
				new_event.value2 = std::stof(value2) * FIXED_UNIT;
				std::cout << "founded event!: " << j[0] << '\n';

				events.push_back(new_event);
			}
		}
	}
	
	//Push dummy section and note
	Section dum_section;
	dum_section.end = 0xFFFF;
	dum_section.flag = sections[sections.size() - 1].flag;
	sections.push_back(dum_section);
	
	Note dum_note;
	dum_note.pos = 0xFFFF;
	dum_note.type = NOTE_FLAG_HIT;
	notes.push_back(dum_note);

	Event dum_event;
	dum_event.pos = 0xFFFF;
	dum_event.event = EVENTS_FLAG_PLAYED;
	dum_event.value1 = dum_event.value2 = 0;
	events.push_back(dum_event);
	
	//Write to output
	std::ofstream out(std::string(argv[2]), std::ostream::binary);
	if (!out.is_open())
	{
		std::cout << "Failed to open " << argv[2] << std::endl;
		return 1;
	}
	
	//Write header
	WriteLong(out, (fixed_t)(speed * FIXED_UNIT)); //first 4 bytes
	WriteWord(out, 6 + (sections.size() * 4)); //skip speed bytes and that bytes (2 bytes) + section size * 4
	
	//Write sections
	for (auto &i : sections)
	{
		WriteWord(out, i.end);
		WriteWord(out, i.flag);
	}
	
	//Write notes
	for (auto &i : notes)
	{
		WriteWord(out, i.pos);
		WriteWord(out, i.type);
	}

	//Write events
	for (auto &i : events)
	{
		WriteWord(out, i.pos);
		WriteWord(out, i.event);
		WriteWord(out,i.value1);
		WriteWord(out,i.value2);
	}

	return 0;
}
