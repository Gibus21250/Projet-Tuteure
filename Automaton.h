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

        void addState(const State& state) {
            this->m_states.push_back(state);
        }

        /**
         *
         * @param nbIteration number of iteration
         * @return list of all transformations calculated in order by a traversal into to automaton
         */
        std::vector<glm::mat4> compute(uint32_t nbIteration) const;

        /**
         *
         * @param nbIteration number of iteration
         * @return list of all transformations calculated in order by a traversal into to automaton
         */
        std::vector<glm::mat4> computeTest(uint32_t nbIteration) const;

        uint32_t numberInstances(uint32_t nbIteration) const;

        /**
         *
         * @param nbIteration number of iteration
         * @return list of arithmetic encoded path for each instance
         */
        std::vector<float> getCode(uint32_t nbIteration) const;

    private:
        std::vector<State> m_states;

    };

}

#endif //AUTOMATON_H
