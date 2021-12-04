#pragma once

#include "Base/Definitions.h"

#include "Definitions.h"
#include "SymbolMap.h"
#include "Settings.h"
#include "SimulationParameters.h"
#include "GeneralSettings.h"
#include "Descriptions.h"

struct SerializedSimulation
{
    std::string timestepAndSettings;
    std::string symbolMap;
    std::string content;
};

struct DeserializedSimulation
{
    uint64_t timestep;
    Settings settings;
    SymbolMap symbolMap;
    DataDescription content;
};

class _Serializer
{
public:
    ENGINEINTERFACE_EXPORT bool serializeSimulationToFile(string const& filename, DeserializedSimulation const& data);
    ENGINEINTERFACE_EXPORT bool deserializeSimulationFromFile(string const& filename, DeserializedSimulation& data);

private:
    void serializeDataDescription(DataDescription const data, std::ostream& stream) const;
    void serializeTimestepAndSettings(uint64_t timestep, Settings const& generalSettings, std::ostream& stream) const;
    void serializeSymbolMap(SymbolMap const symbols, std::ostream& stream) const;

    void deserializeDataDescription(DataDescription& data, std::istream& stream) const;
    void deserializeTimestepAndSettings(uint64_t& timestep, Settings& settings, std::istream& stream) const;
    void deserializeSymbolMap(SymbolMap& symbolMap, std::istream& stream);

	string serializeSymbolMap(SymbolMap const symbols) const;
    SymbolMap deserializeSymbolMap(string const& data);

    string serializeTimestepAndSettings(uint64_t timestep, Settings const& generalSettings) const;
    std::pair<uint64_t, Settings> deserializeTimestepAndSettings(std::string const& data) const;

    string serializeDataDescription(DataDescription const& desc) const;
    DataDescription deserializeDataDescription(string const& data);

    bool loadDataFromFile(std::string const& filename, std::string& data);
    bool saveDataToFile(std::string const& filename, std::string const& data);
};
