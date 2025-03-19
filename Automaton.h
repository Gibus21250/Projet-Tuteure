#ifndef AUTOMATON_H
#define AUTOMATON_H
#include <glm/glm.hpp>
#include <vector>

namespace automaton {

    //Classe Transition
    class Transition {
    public:
        Transition(const glm::mat4& transform, const uint32_t nextState)
            : m_transform(transform), m_nextState(nextState)
        {}

        uint32_t getNextState() const { return m_nextState; }

        const glm::mat4& getTransform() const { return m_transform; }

    private:
        glm::mat4 m_transform;
        uint32_t m_nextState; //State's indice after tranform

    };

    //Classe State
    class State {

    public:
        State() = default;

        void addTransition(const Transition& trans) {
            m_transitions.push_back(trans);
        }

        const std::vector<Transition>& getTransitions() const{
            return m_transitions;
        }

        unsigned getNumberTransform() const {
            return m_transitions.size();
        };

    private:

        std::vector<Transition> m_transitions;

    };

    //Class Automaton
    class Automaton {

    public:

        Automaton() = default;

        /**
         * Compute and return all matrices for the automate (transposed for the GPU)
         * @param nbIteration number of iteration
         * @return list of all transformations calculated in order by a traversal into to automaton
         */
        std::vector<glm::mat4> compute(uint32_t nbIteration) const;

        uint32_t numberInstances(uint32_t nbIteration) const;

        /**
         * Improved version of encode.
         * Compute and return all arithmetic encoded branch based on the automaton.
         * @param nbIteration
         * @return all arithmetic encoded values between 0 and 1
         */
        template <typename T>
        std::vector<T> encode2(uint32_t nbIteration) const
        {

            struct state_temp {
                uint32_t currentState;
                T amplitude;
                T code;
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

                    T step = sTmp.amplitude / currentTrans.size();
                    T center = step / 2.0f;
                    T lower = (sTmp.code - sTmp.amplitude / 2.0f);


                    //For each transitions for this specific state
                    for (int k = 0; k < currentTrans.size(); k++)
                    {
                        T new_code = lower + k * step + center;
                        temp_computed[counterNext] = {currentTrans[k].getNextState(), step, new_code};
                        counterNext++;
                    }

                }
            }

            computed = (nbIteration%2)?pong:ping;

            //Gather code for each leaf
            std::vector<T> result(nbInstance);

            for (uint32_t i = 0; i < nbInstance; i++)
                result[i] = computed[i].code;

            delete[] ping;
            delete[] pong;

            return result;
        }

        /**
         * Compute and return all arithmetic encoded branch based on the automaton.
         * @param nbIteration
         * @return all arithmetic encoded values between 0 and 1
         */
        template <typename T>
        std::vector<T> encode(uint32_t nbIteration) const
        {

            struct state_temp {
                uint32_t currentState;
                T amplitude;
                T code;

                state_temp(uint32_t id, T a, T c)
                        : currentState(id), amplitude(a), code(c) {}
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

                    T step = amplitude / currentTrans.size();
                    T center = step / 2.0f;
                    T lower = (code - amplitude / 2.0f);


                    //For each transitions for this specific state
                    for (int j = 0; j < currentTrans.size(); j++)
                    {
                        T new_code = lower + j * step + center;
                        temp_computed.emplace_back(currentTrans[j].getNextState(), step, new_code);
                    }

                }
                //Heavy copy!
                computed = temp_computed;
            }

            //Gather code for each leaf
            std::vector<T> result(computed.size());

            for (uint32_t i = 0; i < computed.size(); i++)
                result[i] = computed[i].code;

            return result;

        }

        /**
         * Compute and return all matrices decoded
         * @param nbIteration Number of iteration
         * @param encodedValues vector of all encoded values
         * @return vector of glm::mat4 matrices
         */
        template <typename T>
        std::vector<glm::mat4> decode(uint32_t nbIteration, const std::vector<T>& encodedValues) const
        {
            auto result = std::vector<glm::mat4>(encodedValues.size());

            for (uint32_t i = 0; i < encodedValues.size(); i++) {

                glm::mat4 acc(1.0f); // Eye matrix
                T code = encodedValues[i];
                uint32_t currentState = 0; //Start state 0
                for (uint32_t j = 0; j < nbIteration; j++) {

                    const T stateStep = 1.0f / static_cast<T>(m_states[currentState].getTransitions().size());
                    const auto transformIndex = static_cast<uint32_t>(std::floor(code / stateStep));

                    acc = m_states[currentState].getTransitions()[transformIndex].getTransform() * acc;
                    currentState = m_states[currentState].getTransitions()[transformIndex].getNextState();

                    //Rescale encoded value for the next decoding iteration
                    const T lower = static_cast<T>(transformIndex) * stateStep;
                    const T upper = static_cast<T>(transformIndex + 1) * stateStep;
                    code = (code - lower) / (upper - lower);

                }

                result[i] = glm::transpose(acc);

            }

            return result;
        }

        //***** GETTER & SETTER ***** //

        void addState(const State& state) {
            this->m_states.push_back(state);
        }

        std::vector<State>& getStates() { return m_states; }

    private:
        std::vector<State> m_states;

    };

}

#endif //AUTOMATON_H
