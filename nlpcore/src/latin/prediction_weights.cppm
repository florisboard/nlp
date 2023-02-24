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

export module fl.nlp.core.latin.prediction_weights;

namespace fl::nlp {

using json = nlohmann::json;

struct LookupWeights {
    int max_cost;
    int cost_is_equal;
    int cost_is_opposite_case;
    int cost_insert;
    int cost_delete;
    int cost_substitute;
    int cost_substitute_in_proximity;
    int cost_transpose;
    int penalty_default;
    int penalty_start_of_str;
};

void to_json(json& j, const LookupWeights& lw) {
    j = json{
        {"maxCost", lw.max_cost},
        {"costIsEqual", lw.cost_is_equal},
        {"costIsOppositeCase", lw.cost_is_opposite_case},
        {"costInsert", lw.cost_insert},
        {"costDelete", lw.cost_delete},
        {"costSubstitute", lw.cost_substitute},
        {"costSubstituteInProximity", lw.cost_substitute_in_proximity},
        {"costTranspose", lw.cost_transpose},
        {"penaltyDefault", lw.penalty_default},
        {"penaltyStartOfStr", lw.penalty_start_of_str}
    };
}

void from_json(const json& j, LookupWeights& lw) {
    j.at("maxCost").get_to(lw.max_cost);
    j.at("costIsEqual").get_to(lw.cost_is_equal);
    j.at("costIsOppositeCase").get_to(lw.cost_is_opposite_case);
    j.at("costInsert").get_to(lw.cost_insert);
    j.at("costDelete").get_to(lw.cost_delete);
    j.at("costSubstitute").get_to(lw.cost_substitute);
    j.at("costSubstituteInProximity").get_to(lw.cost_substitute_in_proximity);
    j.at("costTranspose").get_to(lw.cost_transpose);
    j.at("penaltyDefault").get_to(lw.penalty_default);
    j.at("penaltyStartOfStr").get_to(lw.penalty_start_of_str);
}

struct TrainingWeights {
    int usage_bonus;
    int usage_reduction_others;
};

void to_json(json& j, const TrainingWeights& tw) {
    j = json{
        {"usageBonus", tw.usage_bonus},
        {"usageReductionOthers", tw.usage_reduction_others}
    };
}

void from_json(const json& j, TrainingWeights& tw) {
    j.at("usageBonus").get_to(tw.usage_bonus);
    j.at("usageReductionOthers").get_to(tw.usage_reduction_others);
}

struct PredictionWeights {
    LookupWeights lookup;
    TrainingWeights training;
};

void to_json(json& j, const PredictionWeights& pw) {
    j = json{
        {"lookup", pw.lookup},
        {"training", pw.training}
    };
}

void from_json(const json& j, PredictionWeights& pw) {
    j.at("lookup").get_to(pw.lookup);
    j.at("training").get_to(pw.training);
}

export struct LatinPredictionWeights {
    PredictionWeights words;
    PredictionWeights ngrams;
};

export void to_json(json& j, const LatinPredictionWeights& lpw) {
    j = json{
        {"words", lpw.words},
        {"ngrams", lpw.ngrams}
    };
}

export void from_json(const json& j, LatinPredictionWeights& lpw) {
    j.at("words").get_to(lpw.words);
    j.at("ngrams").get_to(lpw.ngrams);
}

} // namespace fl::nlp
