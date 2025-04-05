#include <string>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include "Helpers.h"
#include "diva.h"
#include "nc_log.h"
#include "save_data.h"

// HEAVILY MODELED AFTER:
//   https://github.com/blueskythlikesclouds/DivaModLoader/blob/master/Source/DivaModLoader/SaveData.cpp

constexpr int32_t MaxHeaderSize = 128;
constexpr uint8_t CurrentFileVersion[2] = { 1, 0 };
static const char* SaveFileName = "NewClassics.dat";

static std::unordered_map<int32_t, ConfigSet> config_sets;

ConfigSet* nc::FindConfigSet(int32_t id, bool create_if_missing)
{
	if (auto it = config_sets.find(id); it != config_sets.end())
		return &it->second;
	else if (create_if_missing)
	{
		config_sets[id] = { };
		return &config_sets[id];
	}

	return nullptr;
}

// NOTE: Handle reading of save data file
//
struct SaveDataFile
{
	struct Header
	{
		char signature[8];
		uint8_t version_major;
		uint8_t version_minor;
		uint16_t flags;
		int32_t header_size;
		size_t config_set_count;

		Header()
		{
			memcpy(signature, "NCSAVFIL", 8);
			version_major = CurrentFileVersion[0];
			version_minor = CurrentFileVersion[1];
			flags = 0;
			header_size = MaxHeaderSize;
			config_set_count = 0;
		}
	} header;

	static_assert(sizeof(Header) <= MaxHeaderSize);
	char _header_padding[MaxHeaderSize - sizeof(Header)];

	SaveDataFile()
	{
		memset(_header_padding, 0, sizeof(_header_padding));
	}

	inline bool IsValid() const
	{
		return memcmp(header.signature, "NCSAVFIL", 8) == 0;
	}

	inline bool IsVersionUnsupported() const
	{
		return header.version_major > CurrentFileVersion[0] ||
			(header.version_major == CurrentFileVersion[0] && header.version_minor > CurrentFileVersion[1]);
	}

	inline const ConfigSet* GetConfigSets() const
	{
		return reinterpret_cast<const ConfigSet*>(reinterpret_cast<const uint8_t*>(this) + MaxHeaderSize);
	}
};

static FUNCTION_PTR(void, __fastcall, GetSaveDataPath, 0x1401D70D0, prj::string* out, const prj::string& filename);
HOOK(void, __fastcall, LoadSaveData, 0x1401D7FB0, void* a1)
{
	originalLoadSaveData(a1);

	prj::string path;
	GetSaveDataPath(&path, SaveFileName);

	std::vector<uint8_t> file_data;
	if (FILE* file = fopen(path.c_str(), "rb"); file != nullptr)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);

		file_data.resize(size);
		fread(file_data.data(), size, 0, file);
		fclose(file);
	}

	if (file_data.size() == 0)
	{
		nc::Print("Unable to read save data file.\n");
		return;
	}

	const SaveDataFile* data = reinterpret_cast<const SaveDataFile*>(file_data.data());
	if (!data->IsValid() || data->IsVersionUnsupported())
	{
		nc::Print("Unable to load save data file. The file may be corrupted or made by a newer version of this mod.\n");
		return;
	}

	for (size_t i = 0; i < data->header.config_set_count; i++)
	{
		const ConfigSet& set = data->GetConfigSets()[i];
		config_sets[set.id] = set;
	}
}

// NOTE: Handle writing of save data file
//
struct MemoryWriter
{
	std::vector<uint8_t> data;
	size_t pos = 0;

	void Resize(size_t size) { data.resize(size); }
	bool Write(const void* buffer, size_t size)
	{
		if (pos + size > data.size())
			return false;

		memcpy(&data[pos], buffer, size);
		pos += size;
	}

	bool WriteNulls(size_t count)
	{
		if (pos + count > data.size())
			return false;

		memset(&data[pos], 0, count);
		pos += count;
	}

	bool Dump(const char* path)
	{
		if (FILE* file = fopen(path, "wb"); file != nullptr)
		{
			size_t count = fwrite(data.data(), data.size(), 1, file); 
			fclose(file);
			return count == 1;
		}

		return false;
	}
};

HOOK(void, __fastcall, SaveSaveData, 0x1401D8280, void* a1)
{
	originalSaveSaveData(a1);

	if (config_sets.empty())
		return;

	SaveDataFile::Header header;
	header.config_set_count = config_sets.size();

	MemoryWriter writer = { };
	writer.Resize(MaxHeaderSize + config_sets.size() * sizeof(ConfigSet));
	
	// NOTE: Write header
	writer.Write(&header, sizeof(SaveDataFile::Header));
	writer.WriteNulls(MaxHeaderSize - sizeof(SaveDataFile::Header));

	// NOTE: Write config sets
	for (const auto& [id, set] : config_sets)
	{
		ConfigSet setw = set;
		setw.id = id;
		writer.Write(&setw, sizeof(ConfigSet));
	}

	// NOTE: Dump data to file
	prj::string path;
	GetSaveDataPath(&path, SaveFileName);
	if (!writer.Dump(path.c_str()))
	{
		// TODO: Add "failed to save" warning
	}
}

void nc::CreateDefaultSaveData()
{
	config_sets[-1] = { }; // NOTE: Shared Set A
	config_sets[-2] = { }; // NOTE: Shared Set B
	config_sets[-3] = { }; // NOTE: Shared Set C
}

void nc::InstallSaveDataHooks()
{
	INSTALL_HOOK(LoadSaveData);
	INSTALL_HOOK(SaveSaveData);
}