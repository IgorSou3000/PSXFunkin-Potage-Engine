/*
 * funkinchrpak by IgorSou3000 (based on the UNSTOP4BLE version)
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

#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

#define CHAR_FLAGS_IS_PLAYER 	(1 << 0)	//Character is the player
#define CHAR_FLAGS_MISS_ANIM 	(1 << 1)	//Has miss animations
#define CHAR_FLAGS_GF_DANCE   	(1 << 2)	//Has girlfriend dance
#define CHAR_FLAGS_SPOOKY_DANCE (1 << 3)	//Has spooky month dance

typedef int32_t fixed_t;

#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)

typedef struct
{
	//Animation data and indices
	uint8_t spd;
	uint8_t indices[128] = {0};
} Animation;

typedef struct
{
	uint32_t frames_address;
	uint32_t animations_address;
	uint32_t texture_paths_size;

} CharFileHeader;

typedef struct
{
	uint8_t flags;
	uint8_t health_i;
	uint32_t health_b;

	fixed_t focus_x;
	fixed_t	focus_y;
	fixed_t focus_zoom;

	fixed_t scale;

	char archive_path[32];

} CharFile;

typedef struct
{
	uint16_t tex;
	uint16_t src[4];
	int16_t off[2];
} CharFrame;

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

	CharFileHeader header;
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

	strncpy(character.archive_path, static_cast<std::string>(json_data["archive_path"]).c_str(), 32);

	if (json_data["flags"]["is_player"] == true)
		character.flags |= CHAR_FLAGS_IS_PLAYER;

	if (json_data["flags"]["miss_animation"] == true)
		character.flags |= CHAR_FLAGS_MISS_ANIM;

	if (json_data["flags"]["gf_dance"] == true)
		character.flags |= CHAR_FLAGS_GF_DANCE;

	if (json_data["flags"]["spooky_dance"] == true)
		character.flags |= CHAR_FLAGS_SPOOKY_DANCE;

	for (auto& i : json_data["frames"]) //Iterate through frames
	{
		//Read frames
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

	for (auto& i : json_data["animations"]) //Iterate through animations
	{
		//Read animations
		int animation_index = 0;
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

			new_animation.indices[animation_index] = frame;
			animation_index++;
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
   	header.frames_address = sizeof(CharFileHeader) + sizeof(CharFile) + paths.size() * 12;
    header.animations_address = header.frames_address + frames.size() * sizeof(CharFrame);
    header.texture_paths_size = paths.size();

    out.write(reinterpret_cast<const char*>(&header), sizeof(CharFileHeader));
    out.write(reinterpret_cast<const char*>(&character), sizeof(CharFile));

	for (auto& i : paths)
		out.write(i.c_str(), 12);

	for (auto& i : frames)
	{
		out.write(reinterpret_cast<const char*>(&i), sizeof(CharFrame));
	}

	for (auto& i : animations)
	{
		out.write(reinterpret_cast<const char*>(&i), sizeof(Animation));
	}

    return 0;
}