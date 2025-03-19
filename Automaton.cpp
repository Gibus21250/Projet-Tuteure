#include "Automaton.h"

#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <ostream>
#include <vector>

namespace automaton {

    std::vector<glm::mat4> Automaton::compute(const uint32_t nbIteration) const {

        struct state_temp {
            uint32_t stateID;
            glm::mat4 acc;

            state_temp(uint32_t id, const glm::mat4& m)
                    : stateID(id), acc(m) {}
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
                //For each transitions for this specific state
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

        uint32_t nbInstances = 0;

        for (const unsigned int tracesState : tracesStates) {
            nbInstances += tracesState;
        }

        return nbInstances;
    }

}
