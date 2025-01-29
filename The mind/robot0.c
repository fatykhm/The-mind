#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#define PORT 4242

int carte[100];
int manche = 0;

void trierCarte(int carte[], int manche);

int main()
{
    struct sockaddr_in addrServeur;
    SOCKET descRobot;
    char buffer[128];

    // Initialisation de Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Erreur WSAStartup\n");
        return 1;
    }

    // Création du socket
    descRobot = socket(AF_INET, SOCK_STREAM, 0);
    if (descRobot == INVALID_SOCKET)
    {
        printf("Erreur socket\n");
        WSACleanup();
        return 1;
    }

    addrServeur.sin_family = AF_INET;
    addrServeur.sin_port = htons(PORT);
    addrServeur.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connexion au serveur
    if (connect(descRobot, (struct sockaddr *)&addrServeur, sizeof(addrServeur)) == SOCKET_ERROR)
    {
        printf("Erreur connexion\n");
        closesocket(descRobot);
        WSACleanup();
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    recv(descRobot, buffer, sizeof(buffer), 0);
    send(descRobot, "oui", strlen("oui"), 0);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(descRobot, buffer, sizeof(buffer), 0);
        printf("Cartes : %s\n", buffer);

        char *token = strtok(buffer, " ");

        while (token != NULL)
        {
            // Vérifier si le token est un nombre
            char *endptr;
            strtol(token, &endptr, 10); // entative de conversion en nombreT

            int valeur = strtol(token, &endptr, 10); // Convertir en entier
            // Si endptr pointe sur la fin du token cela signifie que c'était un nombre
            if (*endptr == '\0')
            {
                carte[manche] = valeur;
                manche++; // Incrémenter si c'est un nombre
            }

            printf("%s\n", token);

            token = strtok(NULL, " "); // on recupere le prochain token
        }

        int c = manche;
        printf("Nombre de cartes : %d\n", c);
        trierCarte(carte, manche);
        printf("Debut de la manche %d\n", manche);

        while (c > 0)
        {
            printf("Entrez une carte a jouer\n");
            srand(time(NULL));
            int delai = (rand() % 6 + 5) * 1000;
            Sleep(delai);

            snprintf(buffer, sizeof(buffer), "%d", carte[0]);

            printf("Carte jouee : %d\n", carte[0]);

            // Décaler les cartes restantes vers la gauche
            for (int i = 0; i < c - 1; i++)
            {
                carte[i] = carte[i + 1];
            }

            send(descRobot, buffer, strlen(buffer), 0);

            c--;

            printf("Carte envoyee. Il reste %d cartes a jouer.\n", c);
        }
        manche = 0;
        c = 0;
        memset(buffer, 0, sizeof(buffer));
        recv(descRobot, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
    }

    closesocket(descRobot);
    WSACleanup();
    return 0;
}

void trierCarte(int carte[], int manche)
{
    for (int i = 0; i < manche - 1; i++)
    {
        for (int j = 0; j < manche - 1; j++)
            if (carte[j] > carte[j + 1])
            {
                int temp = carte[j];
                carte[j] = carte[j + 1];
                carte[j + 1] = temp;
            }
    }
}
