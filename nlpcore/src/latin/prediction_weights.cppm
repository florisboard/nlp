/*
 * Copyright 2023 Patrick Goldinger
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

module;

#include <nlohmann/json.hpp>

export module fl.nlp.core.latin:prediction_weights;

namespace fl::nlp {

struct LookupWeights {
    double max_cost_sum_;
    double cost_is_equal_;
    double cost_is_equal_ignoring_case_;
    double cost_insert_;
    double cost_insert_start_of_str_;
    double cost_delete_;
    double cost_delete_start_of_str_;
    double cost_substitute_;
    double cost_substitute_in_proximity_;
    double cost_substitute_start_of_str_;
    double cost_transpose_;
};

void to_json(nlohmann::json& j, const LookupWeights& lw) {
    j = nlohmann::json {
        {"maxCostSum", lw.max_cost_sum_},
        {"costIsEqual", lw.cost_is_equal_},
        {"costIsEqualIgnoringCase", lw.cost_is_equal_ignoring_case_},
        {"costInsert", lw.cost_insert_},
        {"costInsertStartOfStr", lw.cost_insert_start_of_str_},
        {"costDelete", lw.cost_delete_},
        {"costDeleteStartOfStr", lw.cost_delete_start_of_str_},
        {"costSubstitute", lw.cost_substitute_},
        {"costSubstituteInProximity", lw.cost_substitute_in_proximity_},
        {"costSubstituteStartOfStr", lw.cost_substitute_start_of_str_},
        {"costTranspose", lw.cost_transpose_}};
}

void from_json(const nlohmann::json& j, LookupWeights& lw) {
    j.at("maxCostSum").get_to(lw.max_cost_sum_);
    j.at("costIsEqual").get_to(lw.cost_is_equal_);
    j.at("costIsEqualIgnoringCase").get_to(lw.cost_is_equal_ignoring_case_);
    j.at("costInsert").get_to(lw.cost_insert_);
    j.at("costInsertStartOfStr").get_to(lw.cost_insert_start_of_str_);
    j.at("costDelete").get_to(lw.cost_delete_);
    j.at("costDeleteStartOfStr").get_to(lw.cost_delete_start_of_str_);
    j.at("costSubstitute").get_to(lw.cost_substitute_);
    j.at("costSubstituteInProximity").get_to(lw.cost_substitute_in_proximity_);
    j.at("costSubstituteStartOfStr").get_to(lw.cost_substitute_start_of_str_);
    j.at("costTranspose").get_to(lw.cost_transpose_);
}

struct TrainingWeights {
    int usage_bonus_;
    int usage_reduction_others_;
};

void to_json(nlohmann::json& j, const TrainingWeights& tw) {
    j = nlohmann::json {{"usageBonus", tw.usage_bonus_}, {"usageReductionOthers", tw.usage_reduction_others_}};
}

void from_json(const nlohmann::json& j, TrainingWeights& tw) {
    j.at("usageBonus").get_to(tw.usage_bonus_);
    j.at("usageReductionOthers").get_to(tw.usage_reduction_others_);
}

export struct LatinPredictionWeights {
    LookupWeights lookup_;
    TrainingWeights training_;
};

export void to_json(nlohmann::json& j, const LatinPredictionWeights& lpw) {
    j = nlohmann::json {{"lookup", lpw.lookup_}, {"training", lpw.training_}};
}

export void from_json(const nlohmann::json& j, LatinPredictionWeights& lpw) {
    j.at("lookup").get_to(lpw.lookup_);
    j.at("training").get_to(lpw.training_);
}

} // namespace fl::nlp
