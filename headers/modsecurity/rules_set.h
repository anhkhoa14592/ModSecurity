/*
 * ModSecurity, http://www.modsecurity.org/
 * Copyright (c) 2015 Trustwave Holdings, Inc. (http://www.trustwave.com/)
 *
 * You may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * If any of the files related to licensing are missing or if you have any
 * other questions related to licensing please contact Trustwave Holdings, Inc.
 * directly using the email address security@modsecurity.org.
 *
 */

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#endif


#ifndef HEADERS_MODSECURITY_RULES_SET_H_
#define HEADERS_MODSECURITY_RULES_SET_H_

#include "modsecurity/rules_set_properties.h"
#include "modsecurity/modsecurity.h"
#include "modsecurity/transaction.h"
#include "modsecurity/rule.h"
#include "modsecurity/rules.h"

#ifdef __cplusplus

namespace modsecurity {
class Rule;
namespace Parser {
class Driver;
}

class RulesSetPhases {
 public:

    ~RulesSetPhases() {
        /** Cleanup the rules */
        for (int i = 0; i < modsecurity::Phases::NUMBER_OF_PHASES; i++) {
            Rules *rules = &m_rules[i];
            while (rules->empty() == false) {
                Rule *rule = rules->back();
                rules->pop_back();
                if (rule->refCountDecreaseAndCheck()) {
                    rule = NULL;
                }
            }
        }
    }

    bool insert(Rule *rule) {
        if (rule->m_phase >= modsecurity::Phases::NUMBER_OF_PHASES) {
            return false;
        }
        m_rules[rule->m_phase].push_back(rule);

        return true;
    }

    int append(RulesSetPhases *from, std::ostringstream *err) {
        int amount_of_rules = 0;
        std::vector<int64_t> v;

        for (int i = 0; i < modsecurity::Phases::NUMBER_OF_PHASES; i++) {
            v.reserve(m_rules[i].size());
            for (size_t z = 0; z < m_rules[i].size(); z++) {
                Rule *rule_ckc = m_rules[i].at(z);
                if (rule_ckc->m_secMarker == true) {
                    continue;
                }
                v.push_back(rule_ckc->m_ruleId);
            }
        }
        std::sort (v.begin(), v.end());

        for (int i = 0; i < modsecurity::Phases::NUMBER_OF_PHASES; i++) {
            for (size_t j = 0; j < from->at(i)->size(); j++) {
                Rule *rule = from->at(i)->at(j);
                if (std::binary_search(v.begin(), v.end(), rule->m_ruleId)) {
                    if (err != NULL) {
                        *err << "Rule id: " << std::to_string(rule->m_ruleId) \
                            << " is duplicated" << std::endl;
                    }
                    return -1;
                }
                amount_of_rules++;
                rule->refCountIncrease();
                m_rules[i].push_back(rule);
            }
        }

        return amount_of_rules;
    }

    void dump() {
        for (int i = 0; i <= modsecurity::Phases::NUMBER_OF_PHASES; i++) {
            std::vector<Rule *> rules = m_rules[i];
            std::cout << "Phase: " << std::to_string(i);
            std::cout << " (" << std::to_string(rules.size());
            std::cout << " rules)" << std::endl;
            for (int j = 0; j < rules.size(); j++) {
                std::cout << "    Rule ID: " << std::to_string(rules[j]->m_ruleId);
                std::cout << "--" << rules[j] << std::endl;
            }
        }
    }

    Rules *operator[](int index) { return &m_rules[index]; }
    Rules *at(int index) { return &m_rules[index]; }

    Rules m_rules[8];
};


/** @ingroup ModSecurity_CPP_API */
class RulesSet : public RulesSetProperties {
 public:
    RulesSet()
        : RulesSetProperties(new DebugLog()),
        unicode_codepage(0),
#ifndef NO_LOGS
        m_secmarker_skipped(0),
#endif
        m_referenceCount(0) { }

    explicit RulesSet(DebugLog *customLog)
        : RulesSetProperties(customLog),
        unicode_codepage(0),
#ifndef NO_LOGS
        m_secmarker_skipped(0),
#endif
        m_referenceCount(0) { }

    ~RulesSet() { }

    void incrementReferenceCount(void);
    void decrementReferenceCount(void);

    int loadFromUri(const char *uri);
    int loadRemote(const char *key, const char *uri);
    int load(const char *rules);
    int load(const char *rules, const std::string &ref);

    void dump();

    int merge(Parser::Driver *driver);
    int merge(RulesSet *rules);

    int evaluate(int phase, Transaction *transaction);
    std::string getParserError();

    void debug(int level, const std::string &id, const std::string &uri,
        const std::string &msg);

    int64_t unicode_codepage;

    RulesSetPhases m_rulesSetPhases;
 private:
    int m_referenceCount;
#ifndef NO_LOGS
    uint8_t m_secmarker_skipped;
#endif
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

RulesSet *msc_create_rules_set(void);
void msc_rules_dump(RulesSet *rules);
int msc_rules_merge(RulesSet *rules_dst, RulesSet *rules_from, const char **error);
int msc_rules_add_remote(RulesSet *rules, const char *key, const char *uri,
    const char **error);
int msc_rules_add_file(RulesSet *rules, const char *file, const char **error);
int msc_rules_add(RulesSet *rules, const char *plain_rules, const char **error);
int msc_rules_cleanup(RulesSet *rules);

#ifdef __cplusplus
}
}  // namespace modsecurity
#endif

#endif  // HEADERS_MODSECURITY_RULES_SET_H_