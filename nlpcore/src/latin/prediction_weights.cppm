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

export module fl.nlp.core.latin.prediction_weights;

namespace fl::nlp {

struct LPW_words_lookup_ {
    int max_cost = 0;
    int cost_is_equal = 0;
    int cost_is_opposite_case = 0;
    int cost_insert = 0;
    int cost_delete = 0;
    int cost_substitute = 0;
    int cost_substitute_in_proximity = 0;
    int cost_transpose = 0;
    int penalty_default = 0;
    int penalty_start_of_str = 0;
};

struct LPW_words_training_ {
    int usage_bonus = 0;
    int usage_reduction_others = 0;
};

struct LPW_words_ {
    LPW_words_lookup_ lookup;
    LPW_words_training_ training;
};

struct LPW_ngrams_lookup_ {
    int max_cost = 0;
};

struct LPW_ngrams_training_ {
    int usage_bonus = 0;
    int usage_reduction_others = 0;
};

struct LPW_ngrams_ {
    LPW_ngrams_lookup_ lookup;
    LPW_ngrams_training_ training;
};

export struct LatinPredictionWeights {
    LPW_words_ words;
    LPW_ngrams_ ngrams;
};

} // namespace fl::nlp
