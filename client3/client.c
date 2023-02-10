#include <stdio.h> /* These are the usual header files */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include <openssl/md5.h>
#include "linklist.h"
#include "graphics.h"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5550
#define P2P_SERVER_PORT 5551
#define DOWN_FIRST_LINE 16
#define UP_FIRST_LINE 3

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

DisplayQueue *downMsg;
DisplayQueue *upMsg;

int line_count = 3;
int down_line_cout = DOWN_FIRST_LINE;
int upload_line_count = UP_FIRST_LINE;
int select_item = 0;
int quit = 0;
int main_quit = 0;

int p2p_port;

int listen_sock;

pthread_t thread_id;
int client_sock;
struct sockaddr_in server_addr;
ClientLst *clientReqConn = NULL;
char fileLst[MAX_LEN][BUFF_SIZE];
char fileName[BUFF_SIZE];
int countFileLst = 0;
bool waitForReplyFromOtherClient = false;
pthread_t threadOfClientRequestConn, threadOfClientReceiveConn;
struct sockaddr_in clientLst[MAX_LEN];
int clientCount = 0;
Status st = ONLINE;

char currentClient[BUFF_SIZE];

SDL_Texture *select_texture;
SDL_Texture *col_line;
SDL_Texture *row_line;

SDL_MessageBoxButtonData buttons[] = {
    {/* .flags, .buttonid, .text */ 0, 0, "no"},
    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes"},
    {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "cancel"},
};
SDL_MessageBoxColorScheme colorScheme = {
    {/* .colors (.r, .g, .b) */
     /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
     {255, 0, 0},
     /* [SDL_MESSAGEBOX_COLOR_TEXT] */
     {0, 255, 0},
     /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
     {255, 255, 0},
     /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
     {0, 0, 255},
     /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
     {255, 0, 255}}};
SDL_MessageBoxData messageboxdata = {
    SDL_MESSAGEBOX_INFORMATION,                      /* .flags */
    NULL,                                            /* .window */
    "warning",                                       /* .title */
    "file already exists! Do you want to override?", /* .message */
    SDL_arraysize(buttons),                          /* .numbuttons */
    buttons,                                         /* .buttons */
    &colorScheme                                     /* .colorScheme */
};

void createMainWindow()
{
    SDL_Rect dest;
    SDL_Surface *rect_surface = SDL_GetWindowSurface(window);
    SDL_FillRect(rect_surface, NULL, SDL_MapRGB(rect_surface->format, 196, 36, 36));
    select_texture = SDL_CreateTextureFromSurface(renderer, rect_surface);

    dest.x = 120;
    dest.y = 90;
    dest.w = 200;
    dest.h = 3;
    SDL_RenderCopy(renderer, select_texture, NULL, &dest);

    col_line = SDL_CreateTextureFromSurface(renderer, rect_surface);

    dest.x = 0;
    dest.y = WIN_H / 2;
    dest.w = WIN_W / 2;
    dest.h = 2;
    SDL_RenderCopy(renderer, col_line, NULL, &dest);

    row_line = SDL_CreateTextureFromSurface(renderer, rect_surface);

    dest.x = WIN_W / 2;
    dest.y = 60;
    dest.w = 2;
    dest.h = WIN_H - 60;
    SDL_RenderCopy(renderer, row_line, NULL, &dest);
    SDL_FreeSurface(rect_surface);

    // SDL_FreeSurface(rect_surface);

    // drawRect(0, WIN_H / 2, WIN_W / 2, 2);
    // drawRect(WIN_W / 2, 60, 2, WIN_H - 60);
}
void destroyAllTexture()
{
    SDL_DestroyTexture(select_texture);
    SDL_DestroyTexture(col_line);
    SDL_DestroyTexture(row_line);
}

void reDrawMain()
{
    SDL_Rect dest;
    if (select_item == 0)
    {
        dest.x = 120;
        dest.y = 90;
        dest.w = 200;
        dest.h = 3;
        SDL_RenderCopy(renderer, select_texture, NULL, &dest);
    }
    else
    {
        dest.x = 20;
        dest.y = (4 + select_item) * 30;
        dest.w = 200;
        dest.h = 25;
        SDL_RenderCopy(renderer, select_texture, NULL, &dest);
    }
    dest.x = 0;
    dest.y = WIN_H / 2;
    dest.w = WIN_W / 2;
    dest.h = 2;
    SDL_RenderCopy(renderer, col_line, NULL, &dest);

    dest.x = WIN_W / 2;
    dest.y = 60;
    dest.w = 2;
    dest.h = WIN_H - 60;
    SDL_RenderCopy(renderer, row_line, NULL, &dest);
}

void drawString(char *string, int x, int y)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x, y, 0, 0};
    dstrect.h = surface->h;
    dstrect.w = surface->w;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}

void drawStringln(char *string, int x)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x, LINE_HIGHT * line_count, 0, 0};
    line_count++;
    dstrect.h = surface->h;
    dstrect.w = surface->w;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}
void drawRightStringln(char *string, int x)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x + WIN_W / 2, LINE_HIGHT * upload_line_count, 0, 0};
    upload_line_count++;
    dstrect.h = surface->h;
    dstrect.w = surface->w;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}

void drawStringlnBottom(char *string, int x)
{
    SDL_Color color = {255, 255, 255};
    SDL_Surface *surface = NULL;
    surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstrect = {x, LINE_HIGHT * down_line_cout, 0, 0};
    down_line_cout++;
    dstrect.h = surface->h;
    dstrect.w = surface->w;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    SDL_DestroyTexture(texture);
}

void hashFile(char *filename, char *c)
{
    int i;
    FILE *inFile = fopen(filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL)
    {
        printf("%s can't be opened.\n", filename);
        exit(0);
    }

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(c, &mdContext);
    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%c", c[i]);
    printf(" - %ld - %s\n", strlen(c), filename);
    fclose(inFile);
}

// chỉ cho phép 2 client kết nối 2 phút với nhau,
// và phải end connect trước khi thời điểm trên,
// vì luồng chính của từng client sleep 2 phút
// nếu không end connect thì lúc này 2 client sẽ là trạng thái BUSY
// sẽ không thể nhận được bất kì thông điệp nào từ server.

void *clientSendReqDown(void *argv)
{
    printf("\nYou have 2 minutes to connect, you must disconnect before the above time.\n");

    int choice, choiceInside;
    int client_sock_p2p;
    struct sockaddr_in *recv_addr = (struct sockaddr_in *)argv;
    struct sockaddr_in address;
    int size;
    int loop = 1;
    Message send_msg, recv_msg;
    FileTranferMsg recv_file;
    int bytes_sent, bytes_received;
    char fileListClientB[MAX_LEN][BUFF_SIZE];
    char nameFile[BUFF_SIZE];
    char hashFileReturn[BUFF_SIZE];
    char buff[BUFF_SIZE];
    struct timeval tv;

    FILE *fp;

    // Creating socket file descriptor
    if ((client_sock_p2p = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port

    address.sin_family = AF_INET;
    // address.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    // address.sin_port = htons(P2P_SERVER_PORT);

    address.sin_addr.s_addr = recv_addr->sin_addr.s_addr;
    address.sin_port = recv_addr->sin_port;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_sock_p2p, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
    if (connect(client_sock_p2p, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("\nConnection Failed \n");
        send_msg.ms_type = END_CONNECT;
        send_msg.dt_type = NONE;
        bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
        waitForReplyFromOtherClient = false;

        return NULL;
    }

    bytes_received = recv(client_sock_p2p, &recv_msg, sizeof(recv_msg), 0); // blocking
    if (bytes_received <= 0)
    {
        perror("recv");
        send_msg.ms_type = END_CONNECT;
        send_msg.dt_type = NONE;
        bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
        waitForReplyFromOtherClient = false;
        return NULL;
    }

    if (recv_msg.ms_type == SEND_FILE_NAME)
    {
        // printf("%s\n", recv_msg.value.filelst[0]);
        size = atoi(recv_msg.value.filelst[0]);
        for (int i = 0; i < size; i++)
        {
            strcpy(fileListClientB[i], recv_msg.value.filelst[i + 1]);
            // printf("%s:\n", recv_msg.value.filelst[i + 1]);
        }

        // memset(&send_msg, 0, sizeof(Message));
        // printf("-------------Menu Request Other Client-------------\n");
        // printf("1. Select file to download.\n");
        // printf("0. End and disconnecting.\n");
        // printf("--------------------------------------------------\n\n");

        // printf("(Send Other Client) Please enter your choice: ");
        // scanf("%d", &choice);
        sprintf(buff, "Start download...");
        addToQueue(downMsg, buff);
        memset(&send_msg, 0, sizeof(Message));
        send_msg.ms_type = SELECT_FILE;
        send_msg.dt_type = STRING_LIST;

        strcpy(send_msg.value.buff, fileName);
        printf("\nRequest to dowload %s\n", fileName);

        bytes_sent = send(client_sock_p2p, &send_msg, sizeof(send_msg), 0);
        memset(nameFile, '\0', strlen(nameFile) + 1);
        strcpy(nameFile, "doc/");
        strcat(nameFile, fileName);
        fp = fopen(nameFile, "wb");
        int test_count = 0;
        while (loop)
        {
            memset(&recv_file, 0, sizeof(FileTranferMsg));
            bzero(recv_file.buff, FILE_PK_SIZE);

            for (size_t nbuffer = 0; nbuffer < sizeof(FileTranferMsg); nbuffer = MAX(nbuffer, sizeof(FileTranferMsg)))
            { /* Watch out for buffer overflow */
                int len = recv(client_sock_p2p, &recv_file, sizeof(FileTranferMsg), 0);
                if (len <= 0)
                    perror("\nRecv pk loss");

                /* FIXME: Error checking */

                nbuffer += len;
            }

            switch (recv_file.ms_type)
            {
            case SEND_FILE:

                // fprintf(fp, "%s", buffer);
                fwrite(&recv_file.buff, recv_file.size, 1, fp);
                // printf("%d\n", ++test_count);
                break;

            // nhận được mã hash từ client kia kèm theo tín hiệu end file
            // lúc này sẽ mã hóa file mới nhận
            // và so sánh
            // nếu giống nhau thì thông báo thành công và ngược lại
            case END_FILE:
                fclose(fp);
                hashFile(nameFile, hashFileReturn);
                if (!strcmp(recv_file.buff, hashFileReturn))
                {
                    printf("\nSuccessfully sent.\n");
                    sprintf(buff, "Download successfull");
                    addToQueue(downMsg, buff);
                }
                else
                {
                    printf("\nAn error occurred on the transmission line.\n");
                    sprintf(buff, "Download unsuccessfull");
                    addToQueue(downMsg, buff);
                }
                printf("\nEND CONNECT.\n");

                send_msg.ms_type = END_CONNECT;
                send_msg.dt_type = NONE;

                bytes_sent = send(client_sock_p2p, &send_msg, sizeof(send_msg), 0);
                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
                waitForReplyFromOtherClient = false;
                loop = 0;
                break;
            default:
                break;
            }
        }
    }
    else
    {
        printf("\nError when connecting.\n");
    }
    close(client_sock_p2p);
    // sleep(4);
    // clearQueue(downMsg);
    return NULL;
}
void startServer()
{
    struct sockaddr_in server;
    Message send_msg, recv_msg;
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server.sin_port = htons(p2p_port);

    // Step 4: P2P
    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) < 0)
    {
        perror("\nBind failed");
        // send_msg.ms_type = END_CONNECT;
        // send_msg.dt_type = NONE;
        // bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
        // close(listen_sock);
        return;
        // exit(EXIT_FAILURE);
    }
    if (listen(listen_sock, 100) < 0)
    {
        perror("listen");
        // exit(EXIT_FAILURE);
    }
}
void *clientRecvReqDown()
{
    printf("\nYou have 2 minutes to connect, you must disconnect before the above time.\n");
    int loop = 1;
    printf("\nList file of you:\n");
    int conn_sock;
    struct sockaddr_in client;
    int sin_size;
    char buffer[BUFF_SIZE], nameFile[BUFF_SIZE];
    char buff[BUFF_SIZE + 100];
    Message send_msg, recv_msg;
    FileTranferMsg send_file;
    int bytes_sent, bytes_received, count = 0;
    FILE *fp = NULL;
    char hashFileReturn[BUFF_SIZE];

    // accept request
    sin_size = sizeof(struct sockaddr_in);
    if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
        perror("\nError: ");
    printf("You got a connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    memset(&send_msg, 0, sizeof(Message));

    send_msg.ms_type = SEND_FILE_NAME;
    send_msg.dt_type = STRING_LIST;
    sprintf(send_msg.value.filelst[0], "%d", countFileLst);
    for (int i = 0; i < countFileLst; i++)
    {
        strcpy(send_msg.value.filelst[i + 1], fileLst[i]);
        printf("    %d.  %s\n", i + 1, send_msg.value.filelst[i + 1]);
    }
    // printf("%s\n", send_msg.value.filelst[0]);
    bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);
    if (bytes_sent <= 0)
        printf("Looix connect\n");
    while (loop)
    {
        memset(&recv_msg, 0, sizeof(Message));

        bytes_received = recv(conn_sock, &recv_msg, sizeof(recv_msg), 0); // blocking
        if (bytes_received <= 0)
        {
            continue;
        }
        else
        {
            switch (recv_msg.ms_type)
            {
                // nhận được tín hiệu yêu cầu download file từ client kia
                // đọc file và gửi đi
            case SELECT_FILE:
                count = 0;
                memset(buffer, '\0', (strlen(buffer)) + 1);
                memset(hashFileReturn, '\0', sizeof(char) * BUFF_SIZE);
                memset(nameFile, '\0', sizeof(char) * BUFF_SIZE);

                printf("The other client choose file %s to download.\n", recv_msg.value.buff);
                strcpy(nameFile, "doc/");
                strcat(nameFile, recv_msg.value.buff);

                fp = fopen(nameFile, "rb");

                while (!feof(fp))
                {
                    memset(&send_file, 0, sizeof(FileTranferMsg));
                    bzero(send_file.buff, FILE_PK_SIZE);
                    send_file.ms_type = SEND_FILE;
                    send_file.size = fread(send_file.buff, 1, FILE_PK_SIZE, fp);
                    bytes_sent = send(conn_sock, &send_file, sizeof(FileTranferMsg), 0);
                    if (bytes_sent <= 0)
                    {
                        perror("[-]Error in sending file.");
                        break;
                    }
                }

                // cứ đọc 10 dòng sẽ gửi 1 lần.
                // while (fgets(buffer, 1024, fp) != NULL)
                // {
                //     count++;
                //     strcat(send_msg.value.buff, buffer);
                //     if (count % 10 == 0)
                //     {
                //         bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);
                //         memset(&send_msg, 0, sizeof(Message));
                //     }
                //     if (bytes_sent <= 0)
                //     {
                //         printf("Connection closed!\n");
                //         exit(0);
                //     }
                //     memset(buffer, '\0', (strlen(buffer)) + 1);
                // }
                fclose(fp);

                // bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);

                printf("\nSent the content of file %s :\n%s \n", nameFile, send_msg.value.buff);
                sprintf(buff, "Sent the content of file %s", nameFile);
                addToQueue(upMsg, buff);
                memset(&send_file, 0, sizeof(FILE_PK_SIZE));
                bzero(send_file.buff, FILE_PK_SIZE);

                // sau khi kết thúc thì gửi hash file kèm tín hiệu end file
                // để client kia biết đã gửi xong file
                hashFile(nameFile, hashFileReturn);
                strcpy(send_file.buff, hashFileReturn);
                send_file.ms_type = END_FILE;
                bytes_sent = send(conn_sock, &send_file, sizeof(FileTranferMsg), 0);

                break;
            case END_CONNECT:
                printf("download complete\n");
                sprintf(buff, "Upload complete");
                addToQueue(upMsg, buff);
                loop = 0;
                send_msg.ms_type = END_CONNECT;
                send_msg.dt_type = NONE;

                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);

                break;
            default:
                break;
            }
        }
    }
    close(conn_sock);

    return NULL;
}

void *sendRequestToServer(void *vargp)
{
    SDL_Event event;
    char string[1000];
    char line_string[1000];
    char display_str[1000];
    char buff[1024];
    strcpy(string, "");
    strcpy(display_str, " ");
    int buttonid;

    char port[BUFF_SIZE], address[BUFF_SIZE];
    char searchString[BUFF_SIZE];
    int bytes_sent;
    Message ms;
    ClientLst *ptr = NULL;
    pthread_t pid;
    int choice, choiceOfConnect, choiceClient = 0;
    printf("\nWelcome to P2P file tranfer application\n");
    int count = 0;
    while (!main_quit)
    {
        while (!quit)
        {

            line_count = 3;
            down_line_cout = DOWN_FIRST_LINE;
            upload_line_count = UP_FIRST_LINE;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    quit = 1;
                    choice = 3;
                    // exit(0);
                    break;
                case SDL_TEXTINPUT:
                    if (select_item == 0)
                    {
                        strcat(string, event.text.text);
                        strcpy(display_str, "");
                        strcat(display_str, string);
                        strcat(display_str, "_");
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_BACKSPACE:
                        if (select_item == 0)
                        {
                            if (strlen(string) > 0)
                                string[strlen(string) - 1] = '\0';
                            // printf("%lu\n", strlen(string));
                            strcpy(display_str, "");
                            strcat(display_str, string);
                            strcat(display_str, "_");
                        }

                        break;
                    case 13:
                        if (!waitForReplyFromOtherClient)
                        {
                            if (select_item == 0)
                                choice = 1;
                            else
                            {
                                choice = 2;
                            }
                            quit = 1;
                        }

                        break;
                    // case SDLK_LEFT:
                    //     dest.x -= 50;
                    //     break;
                    // case SDLK_RIGHT:
                    //     dest.x += 50;
                    //     break;
                    case SDLK_UP:
                        select_item--;
                        if (select_item < 0)
                            select_item = clientCount;
                        break;
                    case SDLK_DOWN:
                        select_item++;
                        if (select_item > clientCount)
                            select_item = 0;
                        break;
                    }
                    break;
                }
            }

            SDL_RenderClear(renderer);

            drawString("FILE SHARING APPLICATION", 500, 0);
            drawString(currentClient, 20, 0);
            drawString("DOWNLOAD LOG", 220, 450);
            drawString("UPLOAD LOG", 830, 50);
            reDrawMain();
            drawString("Filename:", 20, 60);
            drawString(display_str, 120, 60);
            strcpy(searchString, "Search results: ");
            if (strcmp(fileName, ""))
                strcat(searchString, fileName);
            drawStringln(searchString, 20);
            drawStringln(" ", 20);
            if (clientCount > 0)
            {
                // printf("Clients list:\n");
                for (int i = 0; i < clientCount; i++)
                {
                    sprintf(line_string, "    %d. %s:%d\n", i + 1, inet_ntoa(clientLst[i].sin_addr), ntohs(clientLst[i].sin_port));
                    drawStringln(line_string, 20);
                }
            }

            if (downMsg->size > 0)
            {
                for (int i = 0; i < downMsg->size; i++)
                    drawStringlnBottom(downMsg->message[i], 20);
            }
            if (upMsg->size > 0)
            {
                for (int i = 0; i < upMsg->size; i++)
                    drawRightStringln(upMsg->message[i], 20);
            }
            SDL_RenderPresent(renderer);
            SDL_Delay(1000 / 60);
        }
        quit = 0;
        // select_item = 0;

        // printf("\n---------------Menu Request Server---------------\n");
        // printf("1. Input file name to search\n");
        // printf("2. Input IP address and Port of client to request connect\n");
        // // printf("3. Answer connection request\n");
        // printf("3. Exit\n");
        // printf("-------------------------------------------------\n\n");

        // printf("(Send server) Please enter your choice: ");
        // scanf("%d%*c", &choice);

        // // while (getchar() != '\n');
        if ((!strcmp(string, "") || strlen(string) == 0) && choice != 3)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "warning", "filename is empty", window);
        }
        else
        {

            switch (choice)
            {
            case 1:
            {
                // printf("\n(Send server)Input file name and press ENTER to search:\n");

                // memset(fileName, '\0', (strlen(fileName)) + 1);
                // fgets(fileName, BUFF_SIZE, stdin);
                // fileName[strlen(fileName) - 1] = '\0';

                strcpy(fileName, string);
                ms.ms_type = FIND;
                ms.dt_type = STRING;
                // printf("%s:\n", fileName);
                strcpy(ms.value.buff, fileName);
                bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                if (bytes_sent <= 0)
                {
                    printf("Connection closed!\n");
                    exit(0);
                }
            }
            break;

            case 2:

                if (isClientHaveFile(fileName))
                {
                    SDL_ShowMessageBox(&messageboxdata, &buttonid);
                    if (buttonid != 1)
                        break;
                }
                if (clientCount > 0)
                {
                    printf("Clients list:\n");
                    for (int i = 0; i < clientCount; i++)
                    {
                        printf("    %d. %s:%d\n", i + 1, inet_ntoa(clientLst[i].sin_addr), ntohs(clientLst[i].sin_port));
                    }
                    // printf("your choice(0 to exit):");
                    // scanf("%d%*c", &choice);
                    choice = select_item;
                    if (choice > 0 && choice <= clientCount)
                    {
                        ms.ms_type = SELECT;
                        ms.dt_type = ADDRESS;
                        ms.value.addr = clientLst[choice - 1];
                        waitForReplyFromOtherClient = true;
                        bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                        if (bytes_sent <= 0)
                        {
                            printf("Connection closed!\n");
                            exit(0);
                        }
                        if (downMsg->size >= 9)
                            clearQueue(downMsg);
                        sprintf(buff, "Send Request download to %s:%d", inet_ntoa(clientLst[choice - 1].sin_addr), ntohs(clientLst[choice - 1].sin_port));
                        addToQueue(downMsg, buff);

                        // while (waitForReplyFromOtherClient)
                        //     ;
                        // sleep(20);
                        // if (waitForReplyFromOtherClient)
                        // {
                        //     sleep(120);
                        // }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    printf("No client have file");
                }

                break;

                // cần phải trả lời đồng ý hay từ chối cho kết nối ? và kết nối với client nào, thì nhập vào từ bàn phím
                // phải trả lời cho từng trường hợp đồng ý hay từ chối

                // case 3:
                //     // port và Ip address này phải thuộc mảng các client đã gửi yêu cầu kết nối , nếu không thì ,
                //     memset(&ms, 0, sizeof(Message));
                //     ms.ms_type = RES_SELECT;

                //     if (clientReqConn == NULL)
                //     {
                //         printf("\nNo connection request from other client.\n");
                //     }
                //     else
                //     {
                //         printf("\n(Send server)Clients request connection:\n");
                //         ptr = clientReqConn;
                //         count = 0;
                //         while (ptr != NULL)
                //         {
                //             count++;
                //             printf("    %d. %s:%d\n", count, inet_ntoa(ptr->client.sin_addr), ntohs(ptr->client.sin_port));
                //             ptr = ptr->next;
                //         }

                //         printf("\nThe client you choose is: ");
                //         scanf("%d%*c", &choiceClient);

                //         if (choiceClient > count)
                //         {
                //             printf("Invalid selection.\n");
                //             continue;
                //         }
                //         else
                //         {
                //             ms.value.addr = findClientById(clientReqConn, choiceClient);
                //         }

                //         printf("\n    1. Accept\n");
                //         printf("    2. Reject\n\n");
                //         printf("Your choice is : ");
                //         scanf("%d%*c", &choiceOfConnect);

                //         if (choiceOfConnect == 1)
                //         {
                //             ms.dt_type = ADDRESS;
                //             bytes_sent = send(client_sock, &ms, sizeof(ms), 0);

                //             pthread_create(&threadOfClientReceiveConn, NULL, clientRecvReqDown, NULL);
                //             // sleep(180);
                //             // pthread_cancel(threadOfClientReceiveConn);
                //             pthread_join(threadOfClientReceiveConn, NULL);
                //         }
                //         else
                //         {
                //             ms.dt_type = NONE;
                //             bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                //         }
                //         deleteClient(&clientReqConn, ms.value.addr);
                //     }

                //     break;

            case 3:
                // Thoát nhưng chưa xóa dữ liệu bên server
                printf("Goodbye.\n");
                memset(&ms, 0, sizeof(Message));
                ms.ms_type = END;
                ms.dt_type = NONE;
                bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                main_quit = 1;
                break;

            default:
                printf("Choice does not exist.\n");
            }
        }
    }
}

int main(int argc, char **argv)
{
    struct dirent *de;
    DIR *dr;
    /* server's address information */
    char buff[BUFF_SIZE];
    int msg_len, bytes_sent, bytes_received;
    int i = 0;
    struct sockaddr_in client_addr;
    Message ms, send_msg;
    int addr_size = sizeof(client_addr);
    if (argc != 2)
    {
        printf("Invalid Parameter! \n./client portNumber\n");
        exit(0);
    }
    p2p_port = atoi(argv[1]);
    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    downMsg = createQueue();
    upMsg = createQueue();

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    font = TTF_OpenFont("sans.ttf", 18);

    window = SDL_CreateWindow("File Tranfer Client 3",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_W,
                              WIN_H, 0);
    if (!window)
        printf("loi window\n");
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        printf("loi renderer\n");
    createMainWindow();
    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        printf("\nError!Can not connect to sever! Client exit imediately! ");
        return 0;
    }
    getsockname(client_sock, (struct sockaddr *)&client_addr, &addr_size);
    sprintf(currentClient, "%s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    startServer();
    pthread_create(&thread_id, NULL, sendRequestToServer, NULL);

    while (!main_quit)
    {
        bytes_received = recv(client_sock, &ms, sizeof(ms), 0); // blocking
        if (bytes_received <= 0)
        {
            printf("\nConnection to server closed\n");
            break;
        }
        else
        {
            switch (ms.ms_type)
            {
            case UPDATE:
                memset(&send_msg, 0, sizeof(Message));
                send_msg.dt_type = STRING_LIST;
                send_msg.ms_type = RES_UPDATE;
                send_msg.size = 0;
                // strcpy(send_msg.value.filelst[0], "0");
                dr = opendir("./doc");
                i = 0;
                if (dr == NULL) // opendir returns NULL if couldn't open directory
                {
                    printf("Could not open current directory.\n");
                    break;
                }
                memset(fileLst, 0, sizeof(fileLst));

                while ((de = readdir(dr)) != NULL)
                {
                    if (de->d_type == 8)
                    {
                        strcpy(send_msg.value.filelst[i], de->d_name);
                        strcpy(fileLst[i++], de->d_name);
                    }
                    countFileLst = i;
                    send_msg.size = i;
                    if (i >= MAX_LEN)
                    {
                        printf("Out of array length\n");
                        break;
                    }
                }
                // sprintf(send_msg.value.filelst[0], "%d", countFileLst);
                closedir(dr);
                while ((bytes_sent = send(client_sock, &send_msg, sizeof(Message), 0)) != sizeof(Message))
                    ;
                if (bytes_sent <= 0)
                {
                    printf("Connection closed!\n");
                    exit(0);
                }
                break;
            case RES_FIND:
                printf("\n%d clients have file\n", ms.size);
                clientCount = ms.size;
                if (ms.size > 0)
                {
                    for (int i = 0; i < ms.size; i++)
                    {
                        clientLst[i] = ms.value.addrLst[i];
                    }
                }

                break;

            case SELECT:
                printf("\nYou get a connection request to download files from client %s:%d.\n\n",
                       inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
                sprintf(buff, "client %s:%d request to download files",
                        inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
                if (upMsg->size >= 18)
                    clearQueue(upMsg);
                addToQueue(upMsg, buff);
                addClient(&clientReqConn, ms.value.addr);
                memset(&send_msg, 0, sizeof(Message));
                send_msg.ms_type = RES_SELECT;
                send_msg.dt_type = ADDRESS_LIST;
                send_msg.value.addrLst[0] = ms.value.addr;
                send_msg.value.addrLst[1].sin_family = AF_INET;
                send_msg.value.addrLst[1].sin_addr.s_addr = inet_addr(SERVER_ADDR);
                send_msg.value.addrLst[1].sin_port = htons(p2p_port);
                pthread_create(&threadOfClientReceiveConn, NULL, clientRecvReqDown, NULL);
                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
                break;

            case RES_SELECT:
                // printf("\nres_select: %s\n",ms.value.buff);
                printf("\nResponse of request connection: ");
                if (!strcmp(ms.value.buff, "yes"))
                {
                    waitForReplyFromOtherClient = true;
                    printf("Accept.\n\n");

                    pthread_create(&threadOfClientRequestConn, NULL, clientSendReqDown, (void *)&ms.value.addr);
                    // pthread_join(threadOfClientRequestConn, NULL);
                    //  sleep(180);
                    //  pthread_cancel(threadOfClientRequestConn);
                }
                else if (!strcmp(ms.value.buff, "null"))
                {
                    addToQueue(downMsg, "Client not found!");
                    printf("Client not found!\n");
                    waitForReplyFromOtherClient = false;
                }
                else if (!strcmp(ms.value.buff, "busy"))
                {
                    addToQueue(downMsg, "Client busy!");
                    printf("Client busy!\n");
                    waitForReplyFromOtherClient = false;
                }
                else if (!strcmp(ms.value.buff, "offline"))
                {
                    addToQueue(downMsg, "Client offline!");
                    printf("Client offline!\n");
                    waitForReplyFromOtherClient = false;
                }
                else
                {
                    addToQueue(downMsg, "Error while connecting!");
                    printf("Error while connecting!\n");
                    waitForReplyFromOtherClient = false;
                }

                break;
            default:
                break;
            }
        }
    }

    destroyAllTexture();
    close(listen_sock);

    // pthread_join(thread_id, NULL);
    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    close(client_sock);
    return 0;
}

// Công việc cần làm thêm : không hiển thị thông tin của mình
// - mở cổng để kết nối, có thể kết nối theo mô hình client server
// - các client thoát phải xóa dữ liệu trong list

// tải xong thì đóng connect, và
