#ifndef ILFX_RISKPROFILE_HPP
#define ILFX_RISKPROFILE_HPP

#include "allheaders.hpp"
#include "InherentDataSource.hxx"
#include "KPMRDataSource.hxx"
#include "InherentRiskProfile.hxx"
#include "KPMRRiskProfile.hxx"
#include <chaiscript/chaiscript.hpp>
#include <antlr4-runtime.h>
#include "ThresholdLexer.h"
#include "ThresholdParser.h"
#include "EvalVisitor.hpp"
#include "scoreratingparser.hpp"
#include <sstream>

// Forward declaration
bool evalExprWithX(double x, const std::string& expr);

namespace riskprofile {
    class Evaluator
    {
    private:
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> datasources;
        std::shared_ptr<RiskProfileTree> inherentRiskProfile;
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile;
        

    public:
        Evaluator(std::shared_ptr<inherent::datasource::DataType> inherentDataSources,
                  std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources, std::shared_ptr<RiskProfileTree> inherentRiskProfile, std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile)
            : inherentDataSources(inherentDataSources), datasources(kpmrDataSources), inherentRiskProfile(inherentRiskProfile), kpmrRiskProfile(kpmrRiskProfile) {};
        ~Evaluator() = default;

        OperationStatus evaluate();
        OperationStatus evaluateInherentRiskProfile();
        OperationStatus evaluateKPMRRiskProfile();

        double findConsolidateByCode(const std::string& code) {
         for (auto item : inherentDataSources->list().item()) {
                if (item.code() == code) {
                    return item.consolidate();
                }
         }

         return 0.0;
        };

        double computedScoreByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.computed_score().present()) {
                            return std::strtod(node.computed_score().get().c_str(), nullptr);
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double computedValueByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.computed_value().present()) {
                            return std::strtod(node.computed_value().get().c_str(), nullptr);
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double computedWeightedScoreByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.computed_weighted_score().present()) {
                            return std::strtod(node.computed_weighted_score().get().c_str(), nullptr);
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        int computedRatingByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<int(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> int {
                    if (node.Profile_ID() == code) {
                        if (node.computed_rating().present()) {
                            return std::stoi(node.computed_rating().get());
                        }
                        return 0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        int result = searchNode(child);
                        if (result != 0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                int result = searchNode(item);
                if (result != 0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0;
        };

        double findConsolidateKPMRByCode(const std::string& code) {
            // Check if datasources is valid
            if (!datasources) {
                return 0.0;
            }
            
            // Helper function to recursively search through RiskItemType children
            std::function<double(const kpmr::datasource::RiskItemType&)> searchInItem = 
                [&](const kpmr::datasource::RiskItemType& item) -> double {
                    // Check if current item matches the code
                    if (item.code() == code) {
                        if (item.consolidate().present()) {
                            return static_cast<double>(item.consolidate().get());
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children if present
                    if (item.children().present()) {
                        for (const auto& child : item.children()->item()) {
                            double result = searchInItem(child);
                            if (result != 0.0 || child.code() == code) {
                                return result;
                            }
                        }
                    }
                    
                    return 0.0;
                };
            
            // Iterate through the top-level list
            for (const auto& riskGroup : datasources->list()) {
                // Check if the risk group itself matches
                if (riskGroup.code() == code) {
                    return static_cast<double>(riskGroup.value());
                }
                
                // Search in children if present
                if (riskGroup.children().present()) {
                    for (const auto& item : riskGroup.children()->item()) {
                        double result = searchInItem(item);
                        if (result != 0.0 || item.code() == code) {
                            return result;
                        }
                    }
                }
            }
            
            return 0.0;
        };
        
        int ratingByThreshold(const std::string& threshold, double value) {
            // Split threshold by \n
            std::istringstream stream(threshold);
            std::string line;
            
            while (std::getline(stream, line)) {
                if (line.empty()) continue;
                
                try {
                    // Parse with ANTLR
                    antlr4::ANTLRInputStream inputStream(line);
                    ThresholdLexer lexer(&inputStream);
                    antlr4::CommonTokenStream tokens(&lexer);
                    ThresholdParser parser(&tokens);
                    
                    ThresholdParser::RuleFileContext* tree = parser.ruleFile();
                    
                    // Visit the tree to extract rating and expression
                    EvalVisitor visitor;
                    visitor.visit(tree);
                    
                    // Evaluate the expression with the given value
                    if (evalExprWithX(value, visitor.evalExpr)) {
                        // Return the rating if the expression matches
                        return std::stoi(visitor.rating);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing threshold line: " << line << " - " << e.what() << std::endl;
                    continue;
                }
            }
            
            // No matching rule found
            return -1;
        };

        double ratingToScore(const std::string& ratingToScoreStr, int rating) {
            std::cout << "Mapping rating to score using string: " << ratingToScoreStr << " for rating: " << rating << std::endl;
            
            // Parse the rating to score mapping (format: "1:92\n2:76\n3:60\n4:44\n5:28")
            std::istringstream stream(ratingToScoreStr);
            std::string line;
            
            while (std::getline(stream, line)) {
                if (line.empty()) continue;
                
                // Split by ':'
                size_t colonPos = line.find(':');
                if (colonPos == std::string::npos) continue;
                
                try {
                    int lineRating = std::stoi(line.substr(0, colonPos));
                    double score = std::stod(line.substr(colonPos + 1));
                    
                    if (lineRating == rating) {
                        std::cout << "Found matching rating " << rating << " -> score " << score << std::endl;
                        return score;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing rating-to-score line: " << line << " - " << e.what() << std::endl;
                    continue;
                }
            }
            
            // No matching rating found
            std::cout << "No matching rating found for: " << rating << std::endl;
            return 0.0;
        };
        
    private:
        void processInherentRiskNode(RiskProfileNodeType& node, int depth = 0);
        void processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth = 0);
    };

    inline OperationStatus Evaluator::evaluate()
    {
        // Dummy implementation
        std::cout << "Evaluating risk profile..." << std::endl;

            evaluateInherentRiskProfile();
           // evaluateKPMRRiskProfile();
        return SuccessOperationStatus;
    }
    
    inline void Evaluator::processInherentRiskNode(RiskProfileNodeType& node, int depth)
    {
        // Recursively process children first
        for (auto& childNode : node.RiskProfileNode()) {
            processInherentRiskNode(childNode, depth + 1);
        }
        
        std::string indent(depth * 2, ' ');
        std::cout << indent << "Processing Inherent Risk Profile Node: " << node.Profile_ID() << std::endl;
        
        // Only print risiko_name if it's present (it's optional)
        if (node.assessment_factor().present()) {
            std::cout << indent << "  Assessment Name: " << node.assessment_factor().get() << std::endl;
        }

        chaiscript::ChaiScript chai;

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->findConsolidateByCode(code);
        }), "findConsolidateByCode");

        chai.add(chaiscript::fun([this](const std::string& threshold, double value) {
            return this->ratingByThreshold(threshold, value);
        }), "ratingByThreshold");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->findConsolidateKPMRByCode(code);
        }), "findConsolidateKPMRByCode");

        chai.add(chaiscript::fun([this](const std::string& ratingToScoreStr, int rating) {
            return this->ratingToScore(ratingToScoreStr, rating);
        }), "scoreByRating");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedScoreByCode(code);
        }), "computedScoreByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedValueByCode(code);
        }), "computedValueByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedWeightedScoreByCode(code);
        }), "computedWeightedScoreByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedRatingByCode(code);
        }), "computedRatingByCode");

        
        if (node.threshold().present()) {
            //std::cout << indent << "  Threshold: " << node.threshold().get() << std::endl;
            chai.add(chaiscript::var(std::string(node.threshold().get())), "threshold");
        }

        if (node.rating_to_score().present()) {
            //std::cout << indent << "  Rating to Score: " << node.rating_to_score().get() << std::endl;
            chai.add(chaiscript::var(std::string(node.rating_to_score().get())), "rating_to_score");
        }

        // calculate the rules first
        if (node.value_rule().present()) {
            std::cout << indent << "  Value Rule: " << node.value_rule().get() << std::endl;

            
            auto result = chai.eval<double>(node.value_rule().get());

            std::cout << indent << "    Evaluated Value Rule Result: " << result << std::endl;

            node.computed_value(std::to_string(result));

            chai.add(chaiscript::var(result), "value");
        }

        if (node.rating_rule().present()) {
            std::cout << indent << "  Rating Rule: " << node.rating_rule().get() << std::endl;

            auto result = chai.eval<int>(node.rating_rule().get());
            std::cout << indent << "    Evaluated Rating Rule Result: " << result << std::endl;

            node.computed_rating(std::to_string(result));

            chai.add(chaiscript::var(result), "rating");
        }

        if (node.score_rule().present()) {
            std::cout << indent << "  Score Rule: " << node.score_rule().get() << std::endl;

            auto result = chai.eval<double>(node.score_rule().get());

            std::cout << indent << "    Evaluated Score Rule Result: " << result << std::endl;

            node.computed_score(std::to_string(result));

            double weight = 1.0;

            if (node.weight().present()) {
                weight = node.weight().get();
            }

            double weightedScore = result * weight;
            std::cout << indent << "    Weighted Score: " << weightedScore << std::endl;

            node.computed_weighted_score(std::to_string(weightedScore));
        }

        
        
        // Add evaluation logic here
    }
    
    inline OperationStatus Evaluator::evaluateInherentRiskProfile()
    {
        for (auto &node : inherentRiskProfile->RiskProfileNode()) {
            processInherentRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
    
    inline void Evaluator::processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth)
    {
        chaiscript::ChaiScript chai;

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->findConsolidateByCode(code);
        }), "findConsolidateByCode");

        chai.add(chaiscript::fun([this](const std::string& threshold, double value) {
            return this->ratingByThreshold(threshold, value);
        }), "ratingByThreshold");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->findConsolidateKPMRByCode(code);
        }), "findConsolidateKPMRByCode");

        chai.add(chaiscript::fun([this](const std::string& ratingToScoreStr, int rating) {
            return this->ratingToScore(ratingToScoreStr, rating);
        }), "scoreByRating");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedScoreByCode(code);
        }), "computedScoreByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedValueByCode(code);
        }), "computedValueByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedWeightedScoreByCode(code);
        }), "computedWeightedScoreByCode");

        chai.add(chaiscript::fun([this](const std::string& code) {
            return this->computedRatingByCode(code);
        }), "computedRatingByCode");

        // Recursively process children first
        if (node.children().present()) {
            for (auto& childNode : node.children()->node()) {
                processKPMRRiskNode(childNode, depth + 1);

                if (childNode.rating_rule().present()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    std::cout << indentChild << "  Child Rating Rule: " << childNode.rating_rule().get() << std::endl;

                    auto computed_rating = chai.eval<int>(childNode.rating_rule().get());

                    std::cout << indentChild << "Computed Child Rating: " << computed_rating << std::endl;

                    childNode.computed_rating(std::to_string(computed_rating));
                }

                if (childNode.score_rule().present()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    std::cout << indentChild << "  Child Score Rule: " << childNode.score_rule().get() << std::endl;

                    auto computed_score = chai.eval<double>(childNode.score_rule().get());

                    std::cout << indentChild << "Computed Child Score: " << computed_score << std::endl;

                    childNode.computed_score(std::to_string(computed_score));

                    double weight = 1.0;

                    if (childNode.weight().present()) {
                        weight = childNode.weight().get();
                    }

                    double weightedScore = computed_score * weight;
                    std::cout << indentChild << "    Child Weighted Score: " << weightedScore << std::endl;

                    childNode.computed_weighted_score(std::to_string(weightedScore));

                }
            }
        }
        
        std::string indent(depth * 2, ' ');
        std::cout << indent << "Processing KPMR Risk Profile Node: " << node.profile_id() << std::endl;
        std::cout << indent << "  Risk Name: " << node.risiko_name() << std::endl;
        
        // Add evaluation logic here
    }

    inline OperationStatus Evaluator::evaluateKPMRRiskProfile()
    {
        for (auto &node : kpmrRiskProfile->node()) {
            processKPMRRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
}

#endif // ILFX_RISKPROFILE_HPP