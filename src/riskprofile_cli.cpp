#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "riskprofile.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

ABSL_FLAG(std::string, inherent_datasource, "", "Path to the Inherent DataSource XML file");
ABSL_FLAG(std::string, kpmr_datasource, "", "Path to the KPMR DataSource XML file");
ABSL_FLAG(std::string, inherent_riskprofile, "", "Path to the Inherent Risk Profile XML file");
ABSL_FLAG(std::string, kpmr_riskprofile, "", "Path to the KPMR Risk Profile XML file");
ABSL_FLAG(std::string, output_path, "", "Path to the output file (optional)");
ABSL_FLAG(std::string, output_inherent_riskprofile, "", "Path to write the evaluated Inherent Risk Profile XML (optional)");
ABSL_FLAG(std::string, output_kpmr_riskprofile, "", "Path to write the evaluated KPMR Risk Profile XML (optional)");

int main(int argc, char* argv[]) {
    // Set program usage message
    absl::SetProgramUsageMessage(
        "Usage: riskprofile_cli [options]\n"
        "Evaluate risk profiles using inherent and KPMR data sources\n\n"
        "Required flags:\n"
        "  --inherent_datasource=<path>  Path to Inherent DataSource XML\n"
        "  --kpmr_datasource=<path>      Path to KPMR DataSource XML\n"
        "  --inherent_riskprofile=<path> Path to Inherent Risk Profile XML\n"
        "  --kpmr_riskprofile=<path>     Path to KPMR Risk Profile XML\n\n"
        "Optional flags:\n"
        "  --output_path=<path>                    Path to output file (default: riskprofile_output.txt)\n"
        "  --output_inherent_riskprofile=<path>    Path to write evaluated Inherent Risk Profile XML"
    );

    // Parse command-line flags
    std::vector<char*> remaining_args = absl::ParseCommandLine(argc, argv);

    // Get flag values
    std::string inherent_datasource = absl::GetFlag(FLAGS_inherent_datasource);
    std::string kpmr_datasource = absl::GetFlag(FLAGS_kpmr_datasource);
    std::string inherent_riskprofile = absl::GetFlag(FLAGS_inherent_riskprofile);
    std::string kpmr_riskprofile = absl::GetFlag(FLAGS_kpmr_riskprofile);
    std::string output_path = absl::GetFlag(FLAGS_output_path);
    std::string output_inherent_riskprofile = absl::GetFlag(FLAGS_output_inherent_riskprofile);
    std::string output_kpmr_riskprofile = absl::GetFlag(FLAGS_output_kpmr_riskprofile);

    // Validate required flags
    bool has_error = false;
    if (inherent_datasource.empty()) {
        std::cerr << "Error: --inherent_datasource is required\n";
        has_error = true;
    }
    if (kpmr_datasource.empty()) {
        std::cerr << "Error: --kpmr_datasource is required\n";
        has_error = true;
    }
    if (inherent_riskprofile.empty()) {
        std::cerr << "Error: --inherent_riskprofile is required\n";
        has_error = true;
    }
    if (kpmr_riskprofile.empty()) {
        std::cerr << "Error: --kpmr_riskprofile is required\n";
        has_error = true;
    }

    if (has_error) {
        std::cerr << "\nUsage: riskprofile_cli --inherent_datasource=<path> --kpmr_datasource=<path> "
                  << "--inherent_riskprofile=<path> --kpmr_riskprofile=<path> [--output_path=<path>]\n";
        return 1;
    }

    // Validate that files exist
    if (!std::filesystem::exists(inherent_datasource)) {
        std::cerr << "Error: Inherent DataSource file not found: " << inherent_datasource << "\n";
        return 1;
    }
    if (!std::filesystem::exists(kpmr_datasource)) {
        std::cerr << "Error: KPMR DataSource file not found: " << kpmr_datasource << "\n";
        return 1;
    }
    if (!std::filesystem::exists(inherent_riskprofile)) {
        std::cerr << "Error: Inherent Risk Profile file not found: " << inherent_riskprofile << "\n";
        return 1;
    }
    if (!std::filesystem::exists(kpmr_riskprofile)) {
        std::cerr << "Error: KPMR Risk Profile file not found: " << kpmr_riskprofile << "\n";
        return 1;
    }

    try {
        LOG(INFO) << "Loading Inherent DataSource from: " << inherent_datasource;
        
        
        

        // Parse XML files with correct namespaces
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources = std::move(inherent::datasource::data(inherent_datasource));
        LOG(INFO) << "Loading KPMR DataSource from: " << kpmr_datasource;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources = 
            std::move(kpmr::datasource::data(kpmr_datasource));
        LOG(INFO) << "Loading Inherent Risk Profile from: " << inherent_riskprofile;
            std::shared_ptr<RiskProfileTree> inherentRiskProfile = std::move(RiskProfileTree_(inherent_riskprofile));
        LOG(INFO) << "Loading KPMR Risk Profile from: " << kpmr_riskprofile;
            std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfileTree = 
            std::move(kpmr::riskprofile::kpmr_risk_profile_tree_(kpmr_riskprofile));

        LOG(INFO) << "All data loaded successfully";

        // Create evaluator with loaded data
        riskprofile::Evaluator evaluator(
            inherentDataSources,
            kpmrDataSources,
            inherentRiskProfile,
            kpmrRiskProfileTree
        );

        LOG(INFO) << "Starting risk profile evaluation...";

        // Run evaluation
        OperationStatus status = evaluator.evaluate();

        if (status == SuccessOperationStatus) {
            LOG(INFO) << "Risk profile evaluation completed successfully";
            std::cout << "Successfully evaluated risk profiles\n";
            std::cout << "  Inherent DataSource: " << inherent_datasource << "\n";
            std::cout << "  KPMR DataSource: " << kpmr_datasource << "\n";
            std::cout << "  Inherent Risk Profile: " << inherent_riskprofile << "\n";
            std::cout << "  KPMR Risk Profile: " << kpmr_riskprofile << "\n";

            // Evaluate individual profiles
            LOG(INFO) << "Evaluating Inherent Risk Profile...";
            OperationStatus inherent_status = evaluator.evaluateInherentRiskProfile();
            if (inherent_status == SuccessOperationStatus) {
                LOG(INFO) << "Inherent Risk Profile evaluation completed";
            } else {
                LOG(WARNING) << "Inherent Risk Profile evaluation failed";
            }

            LOG(INFO) << "Evaluating KPMR Risk Profile...";
            OperationStatus kpmr_status = evaluator.evaluateKPMRRiskProfile();
            if (kpmr_status == SuccessOperationStatus) {
                LOG(INFO) << "KPMR Risk Profile evaluation completed";
            } else {
                LOG(WARNING) << "KPMR Risk Profile evaluation failed";
            }

            // Handle output
            if (output_path.empty()) {
                output_path = "riskprofile_output.txt";
            }

            LOG(INFO) << "Writing results to: " << output_path;

            std::ofstream ofs(output_path);
            if (!ofs.is_open()) {
                LOG(ERROR) << "Failed to open output file: " << output_path;
                std::cerr << "Error: Failed to open output file: " << output_path << "\n";
                return 1;
            }

            // Write evaluation results
            ofs << "Risk Profile Evaluation Results\n";
            ofs << "================================\n\n";
            ofs << "Input Files:\n";
            ofs << "  Inherent DataSource: " << inherent_datasource << "\n";
            ofs << "  KPMR DataSource: " << kpmr_datasource << "\n";
            ofs << "  Inherent Risk Profile: " << inherent_riskprofile << "\n";
            ofs << "  KPMR Risk Profile: " << kpmr_riskprofile << "\n\n";
            ofs << "Status: SUCCESS\n";
            ofs << "Inherent Risk Profile Status: " 
                << (inherent_status == SuccessOperationStatus ? "SUCCESS" : "FAILED") << "\n";
            ofs << "KPMR Risk Profile Status: " 
                << (kpmr_status == SuccessOperationStatus ? "SUCCESS" : "FAILED") << "\n";
            
            ofs.close();

            LOG(INFO) << "Results written successfully to: " << output_path;
            std::cout << "\nResults written to: " << output_path << "\n";

            // Write evaluated Inherent Risk Profile XML if output path is specified
            if (!output_inherent_riskprofile.empty()) {
                try {
                    LOG(INFO) << "Writing evaluated Inherent Risk Profile XML to: " << output_inherent_riskprofile;
                    
                    // Create namespace map for serialization
                    xml_schema::namespace_infomap map;
                    map[""].name = "http://example.com/inherent";
                    map[""].schema = "../xsd/InherentRiskProfile.xsd";
                    
                    // Serialize to file
                    std::ofstream xml_ofs(output_inherent_riskprofile);
                    RiskProfileTree_(xml_ofs, *inherentRiskProfile, map);
                    xml_ofs.close();
                    
                    LOG(INFO) << "Evaluated Inherent Risk Profile XML written successfully";
                    std::cout << "Evaluated Inherent Risk Profile XML written to: " << output_inherent_riskprofile << "\n";
                } catch (const xml_schema::exception& e) {
                    LOG(ERROR) << "Failed to write Inherent Risk Profile XML: " << e.what();
                    std::cerr << "Error writing XML: " << e.what() << "\n";
                    // Don't fail the whole program, just log the error
                }
            }

            // Write evaluated KPMR Risk Profile XML if output path is specified
            if (!output_kpmr_riskprofile.empty()) {
                try {
                    LOG(INFO) << "Writing evaluated KPMR Risk Profile XML to: " << output_kpmr_riskprofile;
                    
                    // Create namespace map for serialization
                    xml_schema::namespace_infomap map;
                    map[""].name = "http://example.com/kpmr";
                    map[""].schema = "../xsd/KPMRRiskProfile.xsd";
                    
                    // Serialize to file
                    std::ofstream xml_ofs(output_kpmr_riskprofile);
                    kpmr::riskprofile::kpmr_risk_profile_tree_(xml_ofs, *kpmrRiskProfileTree, map);
                    xml_ofs.close();
                    
                    LOG(INFO) << "Evaluated KPMR Risk Profile XML written successfully";
                    std::cout << "Evaluated KPMR Risk Profile XML written to: " << output_kpmr_riskprofile << "\n";
                } catch (const xml_schema::exception& e) {
                    LOG(ERROR) << "Failed to write KPMR Risk Profile XML: " << e.what();
                    std::cerr << "Error writing XML: " << e.what() << "\n";
                    // Don't fail the whole program, just log the error
                }
            }

            return 0;
        } else {
            LOG(ERROR) << "Risk profile evaluation failed";
            std::cerr << "Error: Risk profile evaluation failed\n";
            return 1;
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error: " << e.what();
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
