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

    std::vector<float> Automaton::encode2(uint32_t nbIteration) const {

        struct state_temp {
            uint32_t currentState;
            float amplitude;
            float code;
        };

        auto nbInstance = numberInstances(nbIteration);

        //Array of custom struct to keep traces of the current state
        //Preallocated to improve efficiency
        auto ping = new state_temp[nbInstance];
        auto pong = new state_temp[nbInstance];

        auto computed = ping;
        auto temp_computed = pong;

        ping[0] = {0, 1.0, 0.5}; //init ID = 0, 0.0f for centered encode value

        uint32_t nbPing = 1;
        uint32_t nbPong = 0;

        for (uint32_t i = 0; i < nbIteration; i++)
        {

            //Ping pong generator (try to avoid heavy copy, see encode)
            //2 buffers with the same size

            computed = (i%2)?pong:ping;
            temp_computed = (i%2)?ping:pong;

            auto& counterComputed = (i%2)?nbPong:nbPing;
            auto& counterNext = (i%2)?nbPing:nbPong;

            counterNext = 0;

            //For each leaf already computed
            for(int j = 0; j < counterComputed; j++)
            {
                state_temp& sTmp = computed[j];

                auto& currentTrans = m_states[sTmp.currentState].getTransitions();

                float step = sTmp.amplitude / currentTrans.size();
                float center = step / 2.0f;
                float lower = (sTmp.code - sTmp.amplitude / 2.0f);


                //For each transforms for this specific state
                for (int k = 0; k < currentTrans.size(); k++)
                {
                    float new_code = lower + k * step + center;
                    temp_computed[counterNext] = {currentTrans[k].getNextState(), step, new_code};
                    counterNext++;
                }

            }
        }

        computed = (nbIteration%2)?pong:ping;

        //Gather code for each leaf
        std::vector<float> result(nbInstance);

        for (uint32_t i = 0; i < nbInstance; i++)
            result[i] = computed[i].code;

        delete[] ping;
        delete[] pong;

        return result;
    }

    std::vector<float> Automaton::encode(uint32_t nbIteration) const {

        struct state_temp {
            uint32_t currentState;
            float amplitude;
            float code;
        };

        //List of custom struct to keep traces of the current state
        std::vector<state_temp> computed;
        std::vector<state_temp> temp_computed;
        computed.emplace_back(0, 1.0, 0.5); //init ID = 0, 0.0f for centered encode value

        for (uint32_t i = 0; i < nbIteration; i++)
        {

            temp_computed.clear();
            //For each leaf at state stateID and matrix acc
            for (auto&[currentState, amplitude, code] : computed)
            {
                auto& currentTrans = m_states[currentState].getTransitions();

                float step = amplitude / currentTrans.size();
                float center = step / 2.0f;
                float lower = (code - amplitude / 2.0f);


                //For each transforms for this specific state
                for (int j = 0; j < currentTrans.size(); j++)
                {
                    float new_code = lower + j * step + center;
                    temp_computed.emplace_back(currentTrans[j].getNextState(), step, new_code);
                }

            }
            //Heavy copy!
            computed = temp_computed;
        }

        //Gather code for each leaf
        std::vector<float> result(computed.size());

        for (uint32_t i = 0; i < computed.size(); i++)
            result[i] = computed[i].code;

        return result;

    }

    std::vector<glm::mat4> Automaton::decode(uint32_t nbIteration, const std::vector<float>& encodedValues) const {

        auto result = std::vector<glm::mat4>(encodedValues.size());

        for (uint32_t i = 0; i < encodedValues.size(); i++) {

            glm::mat4 acc(1.0f); // Eye matrix
            float code = encodedValues[i];
            uint32_t currentState = 0; //Start state 0
            for (uint32_t j = 0; j < nbIteration; j++) {

                const float stateStep = 1.0f / static_cast<float>(m_states[currentState].getTransitions().size());
                const auto transformIndex = static_cast<uint32_t>(std::floor(code / stateStep));

                acc = m_states[currentState].getTransitions()[transformIndex].getTransform() * acc;
                currentState = m_states[currentState].getTransitions()[transformIndex].getNextState();

                //Rescale encoded value for the next decoding iteration
                const float lower = static_cast<float>(transformIndex) * stateStep;
                const float upper = static_cast<float>(transformIndex + 1) * stateStep;
                code = (code - lower) / (upper - lower);

            }

            result[i] = glm::transpose(acc);

        }

        return result;
    }
}
