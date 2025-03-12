#include "Automaton.h"

#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <ostream>
#include <vector>

namespace automaton {

    /**
     * Compute and return all matrices for the automate (transposed for the GPU)
     * @param nbIteration number of iteration
     * @return list of all transformations calculated in order by a traversal into to automaton
     */
    std::vector<glm::mat4> Automaton::compute(const uint32_t nbIteration) const {

        struct state_temp {
            uint32_t stateID;
            glm::mat4 acc;
        };

        //List of custom struct to keep traces of the current state
        std::vector<state_temp> computed;
        std::vector<state_temp> temp_computed;
        computed.emplace_back(0, 1.0f); //init ID = 0, eye matrix

        for (uint32_t i = 0; i < nbIteration; i++)
        {
            temp_computed.clear();
            //For each leaf at state stateID and matrix acc
            for (auto&[stateID, acc] : computed)
            {
                //For each transforms for this specific state
                for (const auto& transition : m_states[stateID].getTransitions())
                {
                    glm::mat4 new_acc = transition.getTransform() * acc;
                    temp_computed.emplace_back(transition.getNextState(), new_acc);
                }
            }
            //Heavy copy!
            computed = temp_computed;
        }

        //Transform computed to formated mat4 list
        std::vector<glm::mat4> result(computed.size());
        for (uint32_t i = 0; i < computed.size(); i++)
            result[i] = transpose(computed[i].acc);

        return result;
    }

    uint32_t Automaton::numberInstances(uint32_t nbIteration) const {

        uint32_t nbInstances = 0;
        //Keep traces of number of leaf on each state
        std::vector<uint32_t> tracesStates(m_states.size(), 0);
        tracesStates[0] = 1; // We start at the state index 0

        while (nbIteration > 0) {

            std::vector<uint32_t> tracesStatesTmp(m_states.size(), 0);

            //For each leaf count per state
            for (int i = 0; i < tracesStates.size(); i++)
            {
                //Now update leaf count per state
                for (auto transition : m_states[i].getTransitions())
                {
                    tracesStatesTmp[transition.getNextState()] += tracesStates[i];
                }
            }

            tracesStates = tracesStatesTmp;

            nbIteration--;
        }

        for (const unsigned int tracesState : tracesStates) {
            nbInstances += tracesState;
        }

        return nbInstances;
    }

    std::vector<glm::mat4> Automaton::computeTest(uint32_t nbIteration) const {
        //Adapt for centered encoded tree
        auto encodedTree = this->getCode(nbIteration);
        uint32_t nbInstances = encodedTree.size();

        std::vector<glm::mat4> result(nbInstances);

        for (int i = 0; i < nbInstances; ++i) {

            float codeAri = encodedTree[i];

            uint32_t currentState = 0;
            auto res = glm::mat4(1.0f);

            for (int j = 0; j < nbIteration; ++j)
            {
                const float stateStep = 1.0f / static_cast<float>(m_states[currentState].getTransitions().size());

                const auto transformIndex = static_cast<uint32_t>(std::floor(codeAri / stateStep));
                std::cout << " " << currentState << ":" << transformIndex;

                auto &transform = m_states[currentState].getTransitions()[transformIndex];

                res = transform.getTransform() * res;

                //Remap the value between 0 and 1 for the next iteration and save next state
                const float lower = static_cast<float>(transformIndex) * stateStep;
                const float upper = static_cast<float>(transformIndex + 1) * stateStep;
                codeAri = (codeAri - lower) / (upper - lower);

                currentState = transform.getNextState();
            }

            std::cout << "\n";

            result[i] = glm::transpose(res);

        }


        return result;


    }

    /**
     * Compute and return all arithmetic encode branch based on the automaton.
     * @param nbIteration
     * @return all arithmetic encoded values.
     */
    std::vector<float> Automaton::getCode(uint32_t nbIteration) const {

        struct state_temp {
            uint32_t stateID;
            float code;
        };

        //List of custom struct to keep traces of the current state
        std::vector<state_temp> computed;
        std::vector<state_temp> temp_computed;
        computed.emplace_back(0, 0.5f); //init ID = 0, 0.5f for centered encode value

        for (uint32_t i = 0; i < nbIteration; i++)
        {
            temp_computed.clear();
            //For each leaf at state stateID and matrix acc
            for (auto&[stateID, code] : computed)
            {
                auto transitions = m_states[stateID].getTransitions();
                float sizeInterval = 1.0f / transitions.size();
                //                                middle        Iterator factor
                float middle = sizeInterval * (1.0f / 2.0f) * (1.0f / (i + 1.0f));

                uint32_t parity = transitions.size() % 2;
                int center = transitions.size() / 2; //Is under-rounded if Odd

                if(parity == 0)
                {
                    //For each transforms for this specific state
                    for (int j = 0; j < transitions.size(); j++)
                    {
                        if (j == center)
                            center--;

                        float offset = (float) (j - center) * middle;
                        float new_code = code + offset;
                        temp_computed.emplace_back(transitions[j].getNextState(), new_code);
                    }
                }
                else //Odd
                {
                    //For each transforms for this specific state
                    for (int j = 0; j < transitions.size(); j++)
                    {
                        float offset = (float) (j - center) * middle;
                        float new_code = code + offset;
                        temp_computed.emplace_back(transitions[j].getNextState(), new_code);
                    }
                }
            }
            //Heavy copy!
            computed = temp_computed;
        }

        //Transform computed to formated mat4 list
        std::vector<float> result(computed.size());

        for (uint32_t i = 0; i < computed.size(); i++)
            result[i] = computed[i].code;

        return result;

    }
}
