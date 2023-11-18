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

// Constants for section flags
#define SECTION_FLAG_OPPFOCUS (1 << 15)
#define SECTION_FLAG_BPM_MASK 0x7FFF

// Constants for note flags
#define NOTE_FLAG_OPPONENT    (1 << 2)
#define NOTE_FLAG_SUSTAIN     (1 << 3)
#define NOTE_FLAG_SUSTAIN_END (1 << 4)
#define NOTE_FLAG_ALT_ANIM    (1 << 5)
#define NOTE_FLAG_MINE        (1 << 6)
#define NOTE_FLAG_HIT         (1 << 7)

// Constants for event flags
#define EVENTS_FLAG_VARIANT   0xFFFC
#define EVENTS_FLAG_SPEED     (1 << 2)
#define EVENTS_FLAG_GF        (1 << 3)
#define EVENTS_FLAG_CAMZOOM   (1 << 4)
#define EVENTS_FLAG_PLAYED    (1 << 15)

// Structure to represent a section
struct Section
{
    uint16_t end;
    uint16_t flag = 0;
};

// Structure to represent a note
struct Note
{
    uint16_t pos; // 1/12 steps
    uint16_t type;
};

// Structure to represent an event
struct Event
{
    uint16_t pos = 0; // 1/12 steps
    uint16_t event = 0;
    uint16_t value1 = 0;
    uint16_t value2 = 0;
};

typedef int32_t fixed_t;

#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)

// Function to round a position to the nearest multiple of a given crochet
uint16_t PosRound(double pos, double crochet)
{
    return static_cast<uint16_t>(std::floor(pos / crochet + 0.5));
}

// Function to write a 16-bit word to the output stream
void WriteWord(std::ostream &out, uint16_t word)
{
    out.put(word >> 0);
    out.put(word >> 8);
}

// Function to write a 32-bit word to the output stream
void WriteLong(std::ostream &out, uint32_t word)
{
    out.put(word >> 0);
    out.put(word >> 8);
    out.put(word >> 16);
    out.put(word >> 24);
}

// Function to read events from the input json and populate the event_target vector
void Events_Read(json& i, Event& event_src, std::vector<Event>& event_target)
{
    if (i[0] == "Change Scroll Speed")
		event_src.event |= EVENTS_FLAG_SPEED;

	if (i[0] == "Set GF Speed")
		event_src.event |= EVENTS_FLAG_GF;

	if (i[0] == "Add Camera Zoom")
		event_src.event |= EVENTS_FLAG_CAMZOOM;
	
	if (event_src.event & EVENTS_FLAG_VARIANT)
	{
		// Check if Change speed event flag is set
		if (event_src.event & EVENTS_FLAG_SPEED)
		{
			// Set default values for the first and second values if they are empty strings
		    i[1] = (i[1] == "") ? "1" : i[1];
		    i[2] = (i[2] == "") ? "0" : i[2];
		}

		// Check if GF event flag is set
		if (event_src.event & EVENTS_FLAG_GF)
		{
		    // Set default values for the first and second values if they are empty strings
		    i[1] = (i[1] == "") ? "1" : i[1];
		    i[2] = (i[2] == "") ? "0" : i[2];
		}

		// Check if CAMZOOM event flag is set
		if (event_src.event & EVENTS_FLAG_CAMZOOM)
		{
		    // Set default values for the first and second values if they are empty strings
		    i[1] = (i[1] == "") ? "0.015" : i[1]; // cam zoom
		    i[2] = (i[2] == "") ? "0.03" : i[2]; // hud zoom
		}

		//Get values information
		std::string value1 =  i[1];
		std::string value2 =  i[2];

		//fixed values by 1024
		event_src.value1 = static_cast<uint16_t>(std::stof(value1) * FIXED_UNIT);
		event_src.value2 = static_cast<uint16_t>(std::stof(value2) * FIXED_UNIT);
		std::cout << "Found event!: " << i[0] << '\n';

		event_target.push_back(event_src);
	}
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

    // Process the song_info section to extract bpm and speed information
    double bpm = song_info.value("bpm", 0.0);
    double crochet = (60.0 / bpm) * 1000;
    double step_crochet = crochet / 4;

    double speed = song_info.value("speed", 0.0);

	std::cout << argv[1] << " speed: " << speed << " ini bpm: " << bpm << " step_crochet: " << step_crochet << std::endl;
	
	double milli_base = 0;
	uint16_t step_base = 0;

    std::vector<Section> sections;
    std::vector<Note> notes;
    std::vector<Event> events;

    uint16_t section_end = 0;
	int score = 0, dups = 0;
	std::unordered_set<uint32_t> note_fudge;

    // Iterate through the song_info to process sections and notes
    for (auto &i : song_info["notes"])
    {
        bool is_opponent = i["mustHitSection"] != true; //Note: swapped
		
		// Read section
		Section new_section;
		if (i["changeBPM"] == true)
		{
			// Update BPM (THIS IS HELL!)
			milli_base += step_crochet * (section_end - step_base);
			step_base = section_end;
			
			bpm = i["bpm"];
			crochet = (60.0 / bpm) * 1000.0;
			step_crochet = crochet / 4;
			
			std::cout << "chg bpm: " << bpm << " step_crochet: " << step_crochet << " milli_base: " << milli_base << " step_base: " << step_base << std::endl;
		}
		new_section.end = (section_end += 16) * 12; // (uint16_t)i["lengthInSteps"]) * 12; //I had to do this for compatibility
		new_section.flag = PosRound(bpm, 1.0 / 24.0) & SECTION_FLAG_BPM_MASK; 
		bool is_alt = i["altAnim"] == true;
		if (is_opponent)
			new_section.flag |= SECTION_FLAG_OPPFOCUS;
		sections.push_back(new_section);

        // Read notes
        for (auto &j : i["sectionNotes"])
        {
            //Push main note
			Note new_note;

			//Event type
			if (j[1] == -1)
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
			
			// Process notes data and add them to the notes vector
			if (note_fudge.count(*((uint32_t*)&new_note)))
			{
				dups += 1;
				continue;
			}
			note_fudge.insert(*((uint32_t*)&new_note));
				
			notes.push_back(new_note);
			if (!(new_note.type & NOTE_FLAG_OPPONENT))
				score += 350;
			
			// Push sustain notes
			for (int k = 0; k <= sustain; k++)
			{
				Note sustain_note;
				sustain_note.pos = new_note.pos + ((k + 1) * 12);
				sustain_note.type = new_note.type | NOTE_FLAG_SUSTAIN;
				if (k != sustain)
					sustain_note.type &= ~NOTE_FLAG_SUSTAIN_END; // sustain didn't end yet
				notes.push_back(sustain_note);
        	}
        }
    }

    std::cout << "max score: " << score << " dups excluded: " << dups << std::endl;
	
	// Sort notes
	std::sort(notes.begin(), notes.end(), [](Note a, Note b) {
		if (a.pos == b.pos)
			return (b.type & NOTE_FLAG_SUSTAIN) && !(a.type & NOTE_FLAG_SUSTAIN);
		else
			return a.pos < b.pos;
	});

	// Read Events
	for (auto &i : song_info["events"]) //Iterate through sections
	{
		for (auto &j : i[1])
		{
			//Push main event
			Event new_event;

			new_event.pos = (step_base * 12) + PosRound(((double)i[0] - milli_base) * 12.0, step_crochet);
			//Newer psych engine events version
			Events_Read(j, new_event, events);
		}
	}

    // Push dummy section and note
    Section dummy_section;
    dummy_section.end = 0xFFFF;
    dummy_section.flag = (sections.empty() ? 0 : sections.back().flag);
    sections.push_back(dummy_section);

    Note dummy_note;
    dummy_note.pos = 0xFFFF;
    dummy_note.type = NOTE_FLAG_HIT;
    notes.push_back(dummy_note);

    Event dummy_event;
    dummy_event.pos = 0xFFFF;
    dummy_event.event = EVENTS_FLAG_PLAYED;
    events.push_back(dummy_event);

    // Write to output binary file
    std::ofstream out(std::string(argv[2]), std::ostream::binary);
    if (!out.is_open())
    {
        std::cout << "Failed to open " << argv[2] << std::endl;
        return 1;
    }

    // Write header
    WriteLong(out, static_cast<fixed_t>(speed * FIXED_UNIT));
    WriteWord(out, 8 + (sections.size() * 4));
    WriteWord(out, static_cast<uint16_t>(notes.size() * 4));

    // Write sections
    for (auto &section : sections)
    {
        WriteWord(out, section.end);
        WriteWord(out, section.flag);
    }

    // Write notes
    for (auto &note : notes)
    {
        WriteWord(out, note.pos);
        WriteWord(out, note.type);
    }

    // Write events
    for (auto &event : events)
    {
        WriteWord(out, event.pos);
        WriteWord(out, event.event);
        WriteWord(out, event.value1);
        WriteWord(out, event.value2);
    }

    return 0;
}