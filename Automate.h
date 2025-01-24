#ifndef AUTOMATE_H
#define AUTOMATE_H
#include <glm.hpp>
#include <vector>

namespace automate {

    class Transition {
    public:
        Transition(const glm::mat4& transform) {
            this->transform = transform;
        }

    private:
        glm::mat4 transform;

    };

    class Noeud {

    public:
        Noeud() = default;

        void addTransition(const Transition& trans) {
            transitions.push_back(trans);
        }

        std::vector<Transition>& getTransitions() {
            return transitions;
        }

        unsigned getDegree() const {
            return transitions.size();
        };

    private:
        std::vector<Transition> transitions;
    };

    class Graphe {


    };

}

#endif //AUTOMATE_H
