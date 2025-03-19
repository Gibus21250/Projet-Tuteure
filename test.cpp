#include <iostream>
#include <cstdio>
#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

#include "Automaton.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#define M_PIl          3.141592653589793238462643383279502884L

template <typename T>
void genererCSV(const std::vector<T>& data, int nbBuckets, const std::string& filename) {
    // Créer un vecteur pour stocker le comptage des valeurs dans chaque "bucket"
    std::vector<int> histogramme(nbBuckets, 0);

    // Remplir l'histogramme
    for (T valeur : data) {
        int index = static_cast<int>(valeur * nbBuckets);  // Trouver le "bucket" approprié
        if (index == nbBuckets) {
            index--;  // Assurer que les valeurs égales à 1 vont dans le dernier "bucket"
        }
        histogramme[index]++;
    }

    // Créer et ouvrir le fichier CSV
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Erreur d'ouverture du fichier." << std::endl;
        return;
    }

    // Écrire les entêtes (optionnel, pour plus de clarté)
    outFile << "Intervalle,Fréquence\n";

    // Écrire les données de l'histogramme dans le fichier CSV
    for (int i = 0; i < nbBuckets; ++i) {
        T lowerBound = i / (T)nbBuckets;
        T upperBound = (i + 1) / (T)nbBuckets;
        outFile << lowerBound << "-" << upperBound << "," << histogramme[i] << "\n";
    }

    outFile.close();
    std::cout << "Fichier CSV généré : " << filename << std::endl;
}

void testUniform()
{
    automaton::Automaton automate;

    {
        automaton::State S;

        const automaton::Transition S1(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1),
                0             //Next state
        );

        const automaton::Transition S2(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          1.0 / 4, 0.5, 0, 1),
                0
        );

        const automaton::Transition S3(
                glm::mat4(0.5, 0, 0, 0.0,
                          0, 0.5, 0, 0.0,
                          0, 0, 1, 0,
                          .5, 0, 0, 1),
                0
        );



        S.addTransition(S1);
        S.addTransition(S2);
        S.addTransition(S3);

        automate.addState(S);

    }

    int i = 13;

    auto encoded = automate.encode2<float>(i);
    //auto test = automate.decode<double>(i, encoded);
    //auto type = automate.compute(i);

    //std::cout << "i=" << i << (test== type?" OK":" NONOK") << " CodeSize:" << test.size() << "\n";

    auto name = std::string("uniform" + std::to_string(i) + ".csv");
    genererCSV(encoded, 1024, name);

}

void testCycleNonUni()
{
    automaton::Automaton automate;

    {
        automaton::State S;

        const automaton::Transition S1(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1),
                0             //Next state
        );

        const automaton::Transition S2(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          1.0 / 4, 0.5, 0, 1),
                0
        );

        const automaton::Transition S3(
                glm::mat4(0.5, 0, 0, 0.0,
                          0, 0.5, 0, 0.0,
                          0, 0, 1, 0,
                          .5, 0, 0, 1),
                0
        );

        const automaton::Transition S4(
                glm::mat4(1, 0, 0, 0.0,
                          0, 1, 0, 0.0,
                          0, 0, 1, 0,
                          1, 0, 0, 1),
                1
        );

        S.addTransition(S1);
        S.addTransition(S2);
        S.addTransition(S3);
        S.addTransition(S4);

        automate.addState(S);

        automaton::State C;

        glm::mat4 tmp(1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, -.2, 1, 0,
                      1.0 / 4, 0.5, -.8, 1);

        const automaton::Transition C1(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                1
        );

        tmp = (1, 0, 0, 0,
                0, 1, 0, 0,
                0, -.2, 1, 0,
                1.0 / 4, 0.5, .8, 1);

        const automaton::Transition C2(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                0
        );

        C.addTransition(C1);
        C.addTransition(C2);

        automate.addState(C);
    }

    for (int i = 0; i < 17; ++i) {
        auto encoded = automate.encode<float>(i);
        auto test = automate.decode<float>(i, encoded);
        auto type = automate.compute(i);

        std::cout << "i=" << i << (test== type?" OK":" NONOK") << " CodeSize:" << test.size() << "\n";
    }

}
int main(int argc, char **argv)
{
    testUniform();

    return 0;
}



