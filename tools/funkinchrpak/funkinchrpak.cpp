/*
 * funkinchrpak by IgorSou3000
 * Packs PSXFunkin' json formatted characters into a binary file for the PSX port
*/

/*
	IMPORTANT! 
	When you try display a uint8_t variable use the unsigned 
	function otherwise it will print garbage or a character like 'a'
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <unordered_set>

#include "json.hpp"
using json = nlohmann::json;

// Characters flags
#define CHAR_SPEC_ISPLAYER (1 << 0) 	//Character is the player
#define CHAR_SPEC_MISSANIM (1 << 1) 	//Has miss animations
#define CHAR_SPEC_GFANIM   (1 << 2) 	//Has girlfriend animations
#define CHAR_SPEC_FASTANIM (1 << 3) 	//Has fast animations

//Animation flags
#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

//Fixed point constants
typedef int32_t fixed_t;

#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)

struct RECT
{
    int16_t x, y, w, h;
};

struct RECT_FIXED
{
    fixed_t x, y, w, h;
};

typedef struct
{
	uint8_t tex;
	uint16_t src[4];
	int16_t off[2];
} CharFrame;

typedef struct
{
	//Animation data and script
	uint8_t spd;
	uint8_t script[128] = {0};
} Animation;

typedef struct
{
	uint8_t flags;
	uint8_t health_i;
	uint32_t health_b;

	fixed_t focus_x;
	fixed_t	focus_y;
	fixed_t focus_zoom;

	fixed_t scale;

} CharFile;

// Function to write a int16_t to the output stream
void WriteWord(std::ostream &out, uint16_t word)
{
    out.put(word >> 0);
    out.put(word >> 8);
}

// Function to write a int32_t to the output stream
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
		std::cout << "usage: funkinchrpak in_json" << std::endl;
		return 0;
	}
	
	//Read json
	std::ifstream i(argv[1]);
	if (!i.is_open())
	{
		std::cout << "Failed to open " << argv[1] << std::endl;
		return 1;
	}
	json json_data;
	i >> json_data;
	
	CharFile character;
	std::vector<CharFrame> frames;
	std::vector<Animation> animations;
	std::vector<std::string> paths;

	character.flags = 0;

	character.health_i = json_data["health_index"];
	character.health_b = std::stoul(static_cast<std::string>(json_data["healthbar_color"]), nullptr, 16); // Convert hex string to number

	character.focus_x = static_cast<fixed_t>(json_data["camera_position"]["x"]) * FIXED_UNIT;
	character.focus_y = static_cast<fixed_t>(json_data["camera_position"]["y"]) * FIXED_UNIT;
	character.focus_zoom = static_cast<double>(json_data["camera_position"]["zoom"]) * FIXED_UNIT;

	character.scale = static_cast<double>(json_data["scale"]) * FIXED_UNIT;

	if (json_data["flags"]["is_player"] == true)
		character.flags |= CHAR_SPEC_ISPLAYER;

	if (json_data["flags"]["miss_animation"] == true)
		character.flags |= CHAR_SPEC_MISSANIM;

	if (json_data["flags"]["gf_dance"] == true)
		character.flags |= CHAR_SPEC_GFANIM;

	if (json_data["flags"]["fast_dance"] == true)
		character.flags |= CHAR_SPEC_FASTANIM;

	for (auto& i : json_data["frames"])
	{
		CharFrame new_frame;

		std::vector<uint16_t>src_values = i[1];
		std::vector<uint16_t>off_values = i[2];

		new_frame.tex = i[0];

		new_frame.src[0] = src_values[0];
		new_frame.src[1] = src_values[1];
		new_frame.src[2] = src_values[2];
		new_frame.src[3] = src_values[3];

		new_frame.off[0] = off_values[0];
		new_frame.off[1] = off_values[1];

		frames.push_back(new_frame);
	}

	for (auto& i : json_data["animations"])
	{
		int current_script_indice = 0;
		Animation new_animation;
		new_animation.spd = i[0];

		std::vector<uint8_t> frame_values;

		for (auto& j : i[1])
		{
			uint8_t frame;

			if (j == "back") frame = ASCR_BACK;
			else if (j == "chgani") frame = ASCR_CHGANI;
			else if (j == "repeat") frame = ASCR_REPEAT;
			else frame = j;

			new_animation.script[current_script_indice] = frame;
			current_script_indice++;
		}

		animations.push_back(new_animation);
	}

	for (auto& i : json_data["path"])
	{
		std::string tim_path = i;
		
		if (tim_path.length() > 12)
			std::cerr << "The path " << tim_path << " has more character than the max allowed " << "12";

		tim_path.append(12 - tim_path.length(), '\0');

		paths.push_back(tim_path);
	}

	//Write to output binary file
    std::ofstream out(std::string(argv[2]), std::ostream::binary);
    if (!out.is_open())
    {
        std::cout << "Failed to open " << argv[2] << std::endl;
        return 1;
    }

    //Write header
    out.write(reinterpret_cast<const char*>(&character), sizeof(character));

	for (auto& i : paths)
		out.write(i.c_str(), 12);
	out.write("\0", 12);

    return 0;
}