#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define PORT 4242

char buffer[128];

int main()
{
    struct sockaddr_in addrServeur;
    SOCKET descClient;
    int carte = 0;
    int manche = 0;

    // Initialisation de Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Erreur WSAStartup\n");
        return 1;
    }

    // Création du socket
    descClient = socket(AF_INET, SOCK_STREAM, 0);
    if (descClient == INVALID_SOCKET)
    {
        printf("Erreur socket\n");
        WSACleanup();
        return 1;
    }

    addrServeur.sin_family = AF_INET;
    addrServeur.sin_port = htons(PORT);
    addrServeur.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connexion au serveur
    if (connect(descClient, (struct sockaddr *)&addrServeur, sizeof(addrServeur)) == SOCKET_ERROR)
    {
        printf("Erreur connexion\n");
        closesocket(descClient);
        WSACleanup();
        return 1;
    }

    printf("Connecte au serveur.\n");

    memset(buffer, 0, sizeof(buffer));
    recv(descClient, buffer, sizeof(buffer), 0);
    printf("Serveur : %s", buffer);

    printf("Votre reponse (oui/non) : ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    send(descClient, buffer, strlen(buffer), 0);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(descClient, buffer, sizeof(buffer), 0);
        printf("Vos cartes : %s\n", buffer);

        // Découper la chaîne pour obtenir les cartes
        char *token = strtok(buffer, " "); // Utilise un espace comme délimiteur

        // Compter les cartes
        while (token != NULL)
        {
            // Incrémenter directement pour chaque token trouvé
            carte++;
            token = strtok(NULL, " ");
        }

        manche = carte;
        printf("Nombre de cartes : %d\n", carte);
        printf("Debut de la manche %d\n", manche);

        while (carte > 0)
        {
            printf("Entrez une carte a jouer (ou 'exit' pour quitter) : ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strcmp(buffer, "exit") == 0)
            {
                printf("Deconnexion...\n");
                break;
            }

            send(descClient, buffer, strlen(buffer), 0);

            carte--;

            printf("Carte envoyee. Il reste %d cartes a jouer.\n", carte);
        }
        manche = 0;
        carte = 0;

        if (strcmp(buffer, "exit") == 0)
        {
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        recv(descClient, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
    }

    closesocket(descClient);
    WSACleanup();
    return 0;
}
