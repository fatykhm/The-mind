#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <time.h>
#include <stdbool.h>

#define PORT 4242

typedef struct
{
    SOCKET socket;
    int carte[5];
    int nbCarte;
    bool jeu;
} Joueur;

Joueur joueurs[4];
int numJoueur = 0;
int manche = 1;
int carteJeu[100];
int cartesJouees = 0;

void Client(void *clientSocket);
void distribuer();
void envoyerCartes();
bool verifOrdre();

clock_t tempsDebut, tempsFin;

void debutTemps()
{
    tempsDebut = clock();
}

void finTemps()
{
    tempsFin = clock();
    int tempsReponse = (tempsFin - tempsDebut) * 1000 / CLOCKS_PER_SEC;

    FILE *fich = fopen("temps_manches.txt", "a");

    fprintf(fich, "La duree de la manche %d : %d ms\n", manche, tempsReponse);

    fclose(fich);
}

void resultatManche(bool res)
{
    FILE *fich = fopen("temps_manches.txt", "a");

    fprintf(fich, "Cartes jouees : \n");
    for (int i = 0; i < cartesJouees; i++)
    {
        fprintf(fich, "%d ", carteJeu[i]);
    }

    if (res)
    {
        fprintf(fich, "\nRésultat : Gagnée\n");
    }
    else
    {
        fprintf(fich, "\nRésultat : Perdue\n");
    }

    fprintf(fich, "\n");
    fclose(fich);
}

int main()
{
    WSADATA wsaData;

    // Initialisation de Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Erreur WSAStartup\n");
        return 1;
    }

    struct sockaddr_in addrServeur, addrClient;
    int lg_addr_client = sizeof(struct sockaddr_in);
    SOCKET descServ, descClient;

    // Création du socket
    descServ = socket(AF_INET, SOCK_STREAM, 0);
    if (descServ == INVALID_SOCKET)
    {
        printf("Erreur socket\n");
        WSACleanup();
        return 1;
    }

    addrServeur.sin_family = AF_INET;
    addrServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    addrServeur.sin_port = htons(PORT);

    // Bind du socket
    if (bind(descServ, (struct sockaddr *)&addrServeur, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        printf("Erreur bind\n");
        closesocket(descServ);
        WSACleanup();
        return 1;
    }

    while (1)
    {
        // Écoute sur le socket
        if (listen(descServ, 4) == SOCKET_ERROR)
        {
            printf("Erreur listen\n");
            closesocket(descServ);
            WSACleanup();
            return 1;
        }

        printf("En attente\n");

        // Acceptation d'une connexion
        descClient = accept(descServ, (struct sockaddr *)&addrClient, &lg_addr_client);
        if (descClient == INVALID_SOCKET)
        {
            printf("Erreur accept\n");
            continue;
        }

        printf("Nouvelle connexion !\n");

        joueurs[numJoueur].socket = descClient;
        joueurs[numJoueur].nbCarte = 0;
        numJoueur++;

        printf("Nombre de joueurs connectes : %d\n", numJoueur);

        // Démarrer un thread pour gérer ce client
        _beginthread(Client, 0, (void *)&descClient);
    }

    closesocket(descServ);
    WSACleanup();
    return 0;
}

void Client(void *clientSocket)
{
    SOCKET client = *(SOCKET *)clientSocket;
    char buffer[128];

    send(client, "Pret a jouer oui ou non ?\n", strlen("Pret a jouer oui ou non ?\n"), 0);
    memset(buffer, 0, sizeof(buffer));
    recv(client, buffer, sizeof(buffer), 0);

    if (strcmp(buffer, "oui") == 0)
    {
        for (int i = 0; i < numJoueur; i++)
        {
            if (joueurs[i].socket == client)
            {
                joueurs[i].jeu = true;
            }
        }
    }

    int joueursPrets = 0;
    for (int i = 0; i < numJoueur; i++)
    {
        if (joueurs[i].jeu)
        {
            joueursPrets++;
        }
    }

    if (joueursPrets >= 2)
    {
        printf("Le jeu peut commencer !\n");
        distribuer();
        envoyerCartes();
        debutTemps();
    }

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            for (int i = 0; i < numJoueur; i++)
            {
                if (joueurs[i].socket == client)
                {
                    closesocket(joueurs[i].socket);

                    for (int j = i; j < numJoueur - 1; j++)
                    {
                        joueurs[j] = joueurs[j + 1];
                    }
                    cartesJouees = 0;
                    memset(carteJeu, 0, sizeof(carteJeu));
                    for (int i = 0; i < numJoueur; i++)
                    {
                        memset(joueurs[i].carte, 0, sizeof(joueurs[i].carte)); // Réinitialiser toutes les cartes
                        joueurs[i].nbCarte = 0;                                // Remettre à zéro le compteur de cartes
                    }
                    manche = 1;
                    numJoueur--;
                    break;
                }
                printf("Client deconnecte (socket %d).\n", client);
                printf("Nombre de joueurs connectes : %d\n", numJoueur - 1);
                closesocket(client);
                return;
            }
        }

        int carte = atoi(buffer);
        for (int i = 0; i < numJoueur; i++)
        {
            for (int j = 0; j < manche; j++)
            {
                if (joueurs[i].carte[j] == carte)
                    printf("Le joueur %d a joue : %d\n", i + 1, carte);
            }
        }
        carteJeu[cartesJouees++] = carte;

        if (cartesJouees == manche * numJoueur)
        {
            if (verifOrdre())
            {
                printf("Manche gagnee !\n");
                resultatManche(true);
                cartesJouees = 0;
                memset(carteJeu, 0, sizeof(carteJeu));
                for (int i = 0; i < numJoueur; i++)
                {
                    send(joueurs[i].socket, "Manche gagnee !", strlen("Manche gagnee !"), 0);
                    memset(joueurs[i].carte, 0, sizeof(joueurs[i].carte));
                    joueurs[i].nbCarte = 0;
                }
                finTemps();
                manche++;
            }
            else
            {
                printf("Manche perdue !\n");
                resultatManche(false);
                cartesJouees = 0;
                memset(carteJeu, 0, sizeof(carteJeu));
                for (int i = 0; i < numJoueur; i++)
                {
                    send(joueurs[i].socket, "Manche perdue !", strlen("Manche perdue !"), 0);
                    memset(joueurs[i].carte, 0, sizeof(joueurs[i].carte));
                    joueurs[i].nbCarte = 0;
                }
                finTemps();
                manche = 1;
            }
            distribuer();
            envoyerCartes();
            debutTemps();
        }
    }
}

void distribuer()
{
    srand(time(NULL));
    memset(carteJeu, 0, sizeof(carteJeu));
    for (int i = 0; i < numJoueur; i++)
    {
        memset(joueurs[i].carte, 0, sizeof(joueurs[i].carte));
        joueurs[i].nbCarte = 0;
    }
    for (int i = 0; i < numJoueur; i++)
    {
        for (int j = 0; j < manche; j++)
        {
            joueurs[i].carte[j] = rand() % 100 + 1;
            printf("Le joueur %d recoit la carte : %d\n", i + 1, joueurs[i].carte[j]);
        }
    }
}

void envoyerCartes()
{
    char buffer[512];
    for (int i = 0; i < numJoueur; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        for (int j = 0; j < manche; j++)
        {
            char carteBuffer[16];
            snprintf(carteBuffer, sizeof(carteBuffer), "%d ", joueurs[i].carte[j]);
            strncat(buffer, carteBuffer, sizeof(buffer) - strlen(buffer) - 1);
        }

        send(joueurs[i].socket, buffer, strlen(buffer), 0);
    }
}

bool verifOrdre()
{
    for (int i = 1; i < cartesJouees; i++)
    {
        if (carteJeu[i - 1] > carteJeu[i])
            return false;
    }
    return true;
}
