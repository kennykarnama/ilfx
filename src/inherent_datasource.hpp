#ifndef ILFX_INHERENT_DATASOURCE_HPP
#define ILFX_INHERENT_DATASOURCE_HPP

#include "allheaders.hpp"
#include "InherentDataSource.hxx"
#include "chaicli.hpp"
#include "chaiscript/extras/math.hpp"
#include "absl/log/log.h"
#include "Weight.hxx"

typedef std::vector<int> int_vector;
typedef std::map<std::string, int_vector> map_type;
typedef std::pair<const std::string, int_vector> map_pair;

namespace inherent::datasource
{
    class Evaluator
    {
    private:
        /* data */
        std::shared_ptr<DataType> datasources;
        std::shared_ptr<CompanyWeights> companyWeights;
        

        // Store computed input values for companies
        // this will be populated when we are evaluating the FSI rules
        std::unordered_map<std::string, double> computedInputs;

        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);

    public:
        Evaluator(const std::string &configPath, const std::string &weightPath);
        ~Evaluator();

        std::unordered_map<std::string, double> getComputedInputs() const
        {
            return computedInputs;
        };

        std::shared_ptr<DataType> getDataSources() const
        {
            return datasources;
        };

        OperationStatus evaluate();

        OperationStatus riskProfileEvaluation();

        // Return a map of company names to their corresponding vector of values for a given code
        chaiscript::Boxed_Value companyValuesByCode(const std::string &code);

        chaiscript::Boxed_Value companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName);

        // Update the computed input for a given company based on provided values
        bool updateCompanyInputByValues(const std::string &code, const std::string &companyName, double value);

        // weight lookup
        // this function looks up the weight for a given company from the CompanyWeights data
        double findWeightByCompany(const std::string &company);

        // weight lookup
        // this function looks up the weight for a given company type from the CompanyWeights data
        double findWeightByCompanyType(const std::string &companyType);

        // weight lookup
        // this function looks up the bank and financing weight for a given company from the CompanyWeights data
        double findBankAndFinancingWeightByCompany(const std::string &company);
    };

    Evaluator::Evaluator(const std::string &configPath, const std::string &weightPath)
    {
        datasources = data(configPath);
        companyWeights = CompanyWeights_(weightPath);
        computedInputs = std::unordered_map<std::string, double>{};
    }

    Evaluator::~Evaluator()
    {
    }

    inline void Evaluator::setupChaiScriptEvaluator(chaiscript::ChaiScript& chai)
    {
        auto mathlib = chaiscript::extras::math::bootstrap();
        chai.add(mathlib);
        
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->companyValuesByCode(code);
                     }),
                 "companyValuesByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code, const std::string &companyName, double values)
                     {
                         return this->updateCompanyInputByValues(code, companyName, values);
                     }),
                 "updateCompanyInputByValues");

        chai.add(chaiscript::fun(
                     [this](const std::string &code, const std::string &companyName)
                     {
                         return this->companyValuesByCodeAndCompanyName(code, companyName);
                     }),
                 "companyValuesByCodeAndCompanyName");
        
        chai.add(chaiscript::fun(
                     [this](const std::string &company)
                     {
                         return this->findWeightByCompany(company);
                     }),
                 "findWeightByCompany");

        chai.add(chaiscript::fun(
                     [this](const std::string &companyType)
                     {
                         return this->findWeightByCompanyType(companyType);
                     }),
                 "findWeightByCompanyType");

        chai.add(chaiscript::fun(
                     [this](const std::string &company)
                     {
                         return this->findBankAndFinancingWeightByCompany(company);
                     }),
                 "findBankAndFinancingWeightByCompany");
    }

    inline OperationStatus Evaluator::evaluate()
    {
        // register built in functions for chaiscript if needed

        // std::shared_ptr<chaiscript::ChaiScript> chai = cli->getChai();
        

        // first step, we need to evaluate all FSI rules and update the values accordingly
        for (auto &item : datasources->list().item())
        {


            std::string code = item.code();

            std::string fsiRule = "";

            if (item.fsiRule().present())
            {
                fsiRule = item.fsiRule().get();
            }

            if (item.detail().present())
            {
                for (auto &row : item.detail()->row())
                {
                    
                    std::string companyName = row.companyName();
                    std::string companyType = "";
                    if (row.companyType().present())
                    {
                        companyType = row.companyType().get();
                    }

                    double value = row.value();

                    if (!fsiRule.empty())
                    {
                        DLOG(INFO) << "Evaluating FSI Rule for code " << code << ": " << fsiRule;

                        // TODO: this can we made more flexible by providing an opaque object
                        // that can be queried for data instead of injecting individual variables
                        chaiscript::ChaiScript chai;
                        setupChaiScriptEvaluator(chai);
                        
                        chai.add(chaiscript::var(code), "code");
                        chai.add(chaiscript::var(companyName), "companyName");
                        chai.add(chaiscript::var(companyType), "companyType");
                        chai.add(chaiscript::var(value), "value");

                        chaiscript::Boxed_Value v =  chai.eval(fsiRule);
                        double computedValue = chai.boxed_cast<double>(v);

                        // Update the row's value with the computed value
                        row.value(computedValue);

                        DLOG(INFO) << "Computed value for company " << companyName << " and code " << code << ": " << computedValue;
                    }
                }
            }
        }

        // If we reach here, evaluation was successful
        // then we need to evaluate the consolidation rules
        for (auto &item : datasources->list().item())
        {
            std::string consolidationRule = "";

            if (item.consolidationRule().present())
            {
                consolidationRule = item.consolidationRule().get();
            }

            if (!consolidationRule.empty())
            {
                DLOG(INFO) << "Evaluating Consolidation Rule for code " << item.code() << ": " << consolidationRule;

                chaiscript::ChaiScript chai;
                setupChaiScriptEvaluator(chai);
                chaiscript::Boxed_Value v = chai.eval(consolidationRule);
                double consolidatedValue = chai.boxed_cast<double>(v);

                // Set the consolidated value somewhere
                // For this example, we'll just log it
                DLOG(INFO) << "Consolidated value for code " << item.code() << ": " << consolidatedValue;

                item.consolidate(consolidatedValue);
            }
        }

        return SuccessOperationStatus;
    }

    // Return a map of company names to their corresponding vector of values for a given code
    inline chaiscript::Boxed_Value Evaluator::companyValuesByCode(const std::string &code)
    {

        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                std::map<std::string, std::vector<double>> raw;

                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        LOG(INFO) << "Found row for company " << row.companyName() << " with value " << row.value();
                        raw[row.companyName()] = {row.value()};
                    }
                }

                std::map<std::string, chaiscript::Boxed_Value> converted;

                for (auto &[company, vec] : raw)
                {
                    std::vector<chaiscript::Boxed_Value> boxed_vec;
                    boxed_vec.reserve(vec.size());

                    for (double v : vec)
                        boxed_vec.push_back(chaiscript::var(v));

                    converted[company] = chaiscript::var(boxed_vec);
                }

                return chaiscript::var(converted);
            }
        }

        return chaiscript::var(std::map<std::string, chaiscript::Boxed_Value>{});
    }

    inline chaiscript::Boxed_Value Evaluator::companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        if (row.companyName() == companyName)
                        {
                            return chaiscript::var(row.value());
                        }
                    }
                }
            }
        }

        return chaiscript::Boxed_Value();
    }

    inline bool Evaluator::updateCompanyInputByValues(const std::string &code, const std::string &companyName, double value)
    {
        LOG(INFO) << "Updating computed input for company " << companyName << " with values for code " << code;

        this->computedInputs[companyName] = value;

        return true;
    }

    inline double Evaluator::findWeightByCompany(const std::string &company)
    {
        for (const auto &category : companyWeights->Category())
        {
            for (const auto &comp : category.Company())
            {
                if (comp.Code() == company)
                {
                    if (comp.BobotPerPUJK().present()) {
                        return comp.BobotPerPUJK().get(); 
                    }
                }
            }
        }

        return 1.0;
    }

    inline double Evaluator::findWeightByCompanyType(const std::string &companyType)
    {
        for (const auto &category : companyWeights->Category())
        {
            if (category.name() == companyType)
            {
                for (const auto &comp : category.Company())
                {
                    if (comp.BobotBidangUsaha().present()) {
                        return comp.BobotBidangUsaha().get(); 
                    }
                }
            }
        }

        return 1.0;
    }

    inline double Evaluator::findBankAndFinancingWeightByCompany(const std::string &company)
    {
        for (const auto &category : companyWeights->Category())
        {
            for (const auto &comp : category.Company())
            {
                if (comp.Code() == company)
                {
                   if (comp.BobotBankPembiayaan().present()) {
                        return comp.BobotBankPembiayaan().get(); 
                    }
                }
            }
        }

        return 1.0;
    }

} // namespace inherent::datasource

#endif // ILFX_INHERENT_DATASOURCE_HPP