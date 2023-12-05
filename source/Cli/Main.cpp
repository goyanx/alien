#include <algorithm>
#include <iostream>

#include "CLI/CLI.hpp"

#include "Base/GlobalSettings.h"
#include "Base/LoggingService.h"
#include "Base/Resources.h"
#include "Base/StringHelper.h"
#include "Base/FileLogger.h"
#include "EngineInterface/ExportService.h"
#include "EngineInterface/SerializerService.h"
#include "EngineImpl/SimulationControllerImpl.h"

int main(int argc, char** argv)
{
    try {
        FileLogger fileLogger = std::make_shared<_FileLogger>();

        CLI::App app{"Command-line interface for ALIEN v" + Const::ProgramVersion};

        //parse command line arguments
        std::string inputFilename;
        std::string outputFilename;
        std::string statisticsFilename;
        int timesteps = 0;
        app.add_option(
            "-i", inputFilename, "Specifies the name of the input file for the simulation to run. The corresponding .settings.json should also be available.");
        app.add_option("-o", outputFilename, "Specifies the name of the output file for the simulation.");
        app.add_option("-t", timesteps, "The number of time steps to be calculated.");
        app.add_option("-s", statisticsFilename, "Specifies the name of the csv-file containing the statistics.");
        CLI11_PARSE(app, argc, argv);

        //read input
        std::cout << "Reading input" << std::endl;
        if (inputFilename.empty()) {
            std::cout << "No input file given." << std::endl;
            return 1;
        }
        DeserializedSimulation simData;
        if (!SerializerService::deserializeSimulationFromFiles(simData, inputFilename)) {
            std::cout << "Could not read from input files." << std::endl;
            return 1;
        }

        //run simulation
        auto startTimepoint = std::chrono::steady_clock::now();

        auto simController = std::make_shared<_SimulationControllerImpl>();
        simController->newSimulation(simData.auxiliaryData.timestep, simData.auxiliaryData.generalSettings, simData.auxiliaryData.simulationParameters);
        simController->setClusteredSimulationData(simData.mainData);
        simController->setStatisticsHistory(simData.statisticsHistory);
        std::cout << "Device: " << simController->getGpuName() << std::endl;
        std::cout << "Start simulation" << std::endl;

        simController->calcTimesteps(timesteps);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTimepoint).count();
        auto tps = ms != 0 ? 1000.0f * toFloat(timesteps) / toFloat(ms) : 0.0f; 
        std::cout << "Simulation finished: " << StringHelper::format(timesteps) << " time steps, " << StringHelper::format(ms) << " ms, "
                  << StringHelper::format(tps, 1) << " TPS" << std::endl;
        

        //write output simulation file
        std::cout << "Writing output" << std::endl;
        simData.auxiliaryData.timestep = static_cast<uint32_t>(simController->getCurrentTimestep());
        simData.mainData = simController->getClusteredSimulationData();
        simData.auxiliaryData.simulationParameters = simController->getSimulationParameters();
        simData.statisticsHistory = simController->getStatisticsHistory().getCopiedData();
        if (outputFilename.empty()) {
            std::cout << "No output file given." << std::endl;
            return 1;
        }
        if (!SerializerService::serializeSimulationToFiles(outputFilename, simData)) {
            std::cout << "Could not write to output files." << std::endl;
            return 1;
        }

        //write output statistics file
        if (!statisticsFilename.empty()) {
            auto timestep = simController->getCurrentTimestep();
            auto statistics = simController->getRawStatistics();
            if (!ExportService::exportStatistics(timestep, statistics, statisticsFilename)) {
                std::cout << "Could not write to statistics file." << std::endl;
                return 1;
            }
        }

        std::cout << "Finished" << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "An uncaught exception occurred: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "An unknown exception occurred." << std::endl;
    }
    return 0;
}
